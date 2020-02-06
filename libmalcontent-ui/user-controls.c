/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 *
 * Copyright © 2018, 2019, 2020 Endless Mobile, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *  - Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *  - Philip Withnall <withnall@endlessm.com>
 */

#include <libmalcontent/malcontent.h>
#include <flatpak.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>
#include <strings.h>

#include "gs-content-rating.h"
#include "restrict-applications-dialog.h"
#include "user-controls.h"


#define WEB_BROWSERS_CONTENT_TYPE "x-scheme-handler/http"

/* The value which we store as an age to indicate that OARS filtering is disabled. */
static const guint32 oars_disabled_age = (guint32) -1;

struct _MctUserControls
{
  GtkGrid     parent_instance;

  GMenu      *age_menu;
  GtkSwitch  *allow_system_installation_switch;
  GtkSwitch  *allow_user_installation_switch;
  GtkSwitch  *allow_web_browsers_switch;
  GtkButton  *restriction_button;
  GtkPopover *restriction_popover;
  MctRestrictApplicationsDialog *restrict_applications_dialog;

  GSimpleActionGroup *action_group; /* (owned) */

  ActUser    *user; /* (owned) (nullable) */

  GPermission *permission;  /* (owned) (nullable) */
  gulong permission_allowed_id;

  GCancellable *cancellable; /* (owned) */
  MctManager   *manager; /* (owned) */
  MctAppFilter *filter; /* (owned) */
  guint         selected_age; /* @oars_disabled_age to disable OARS */

  guint         blacklist_apps_source_id;
  gboolean      flushed_on_dispose;
};

static gboolean blacklist_apps_cb (gpointer data);

static void on_allow_installation_switch_active_changed_cb (GtkSwitch        *s,
                                                            GParamSpec       *pspec,
                                                            MctUserControls *self);

static void on_allow_web_browsers_switch_active_changed_cb (GtkSwitch        *s,
                                                            GParamSpec       *pspec,
                                                            MctUserControls *self);

static void on_restrict_applications_button_clicked_cb (GtkButton *button,
                                                        gpointer   user_data);

static gboolean on_restrict_applications_dialog_delete_event_cb (GtkWidget *widget,
                                                                 GdkEvent  *event,
                                                                 gpointer   user_data);

static void on_restrict_applications_dialog_response_cb (GtkDialog *dialog,
                                                         gint       response_id,
                                                         gpointer   user_data);

static void on_set_age_action_activated (GSimpleAction *action,
                                         GVariant      *param,
                                         gpointer       user_data);

static void on_permission_allowed_cb (GObject    *obj,
                                      GParamSpec *pspec,
                                      gpointer    user_data);

G_DEFINE_TYPE (MctUserControls, mct_user_controls, GTK_TYPE_GRID)

typedef enum
{
  PROP_USER = 1,
  PROP_PERMISSION,
} MctUserControlsProperty;

static GParamSpec *properties[PROP_PERMISSION + 1];

static const GActionEntry actions[] = {
  { "set-age", on_set_age_action_activated, "u", NULL, NULL, { 0, }}
};

/* FIXME: Factor this out and rely on code from libappstream-glib or gnome-software
 * to do it. See: https://gitlab.freedesktop.org/pwithnall/malcontent/issues/7 */
static const gchar * const oars_categories[] =
{
  "violence-cartoon",
  "violence-fantasy",
  "violence-realistic",
  "violence-bloodshed",
  "violence-sexual",
  "violence-desecration",
  "violence-slavery",
  "violence-worship",
  "drugs-alcohol",
  "drugs-narcotics",
  "drugs-tobacco",
  "sex-nudity",
  "sex-themes",
  "sex-homosexuality",
  "sex-prostitution",
  "sex-adultery",
  "sex-appearance",
  "language-profanity",
  "language-humor",
  "language-discrimination",
  "social-chat",
  "social-info",
  "social-audio",
  "social-location",
  "social-contacts",
  "money-purchasing",
  "money-gambling",
  NULL
};

/* Auxiliary methods */

static GsContentRatingSystem
get_content_rating_system (MctUserControls *self)
{
  const gchar *user_language;

  user_language = act_user_get_language (self->user);

  return gs_utils_content_rating_system_from_locale (user_language);
}

static const gchar *
get_user_display_name (ActUser *user)
{
  const gchar *display_name;

  g_return_val_if_fail (ACT_IS_USER (user), _("unknown"));

  display_name = act_user_get_real_name (user);
  if (display_name != NULL)
    return display_name;

  display_name = act_user_get_user_name (user);
  if (display_name != NULL)
    return display_name;

  /* Translators: this is the full name for an unknown user account. */
  return _("unknown");
}

static void
schedule_update_blacklisted_apps (MctUserControls *self)
{
  if (self->blacklist_apps_source_id > 0)
    return;

  /* Use a timeout to batch multiple quick changes into a single
   * update. 1 second is an arbitrary sufficiently small number */
  self->blacklist_apps_source_id = g_timeout_add_seconds (1, blacklist_apps_cb, self);
}

static void
flush_update_blacklisted_apps (MctUserControls *self)
{
  if (self->blacklist_apps_source_id > 0)
    {
      /* Remove the timer and forcefully call the timer callback. */
      g_source_remove (self->blacklist_apps_source_id);
      self->blacklist_apps_source_id = 0;

      blacklist_apps_cb (self);
    }
}

static void
update_app_filter (MctUserControls *self)
{
  g_autoptr(GError) error = NULL;

  g_clear_pointer (&self->filter, mct_app_filter_unref);

  if (self->user == NULL)
    return;

  /* FIXME: It’s expected that, unless authorised already, a user cannot read
   * another user’s app filter. accounts-service currently (incorrectly) ignores
   * the missing ‘interactive’ flag and prompts the user for permission if so,
   * so don’t query at all in that case. */
  if (act_user_get_uid (self->user) != getuid () &&
      (self->permission == NULL ||
       !g_permission_get_allowed (self->permission)))
    return;

  /* FIXME: make it asynchronous */
  self->filter = mct_manager_get_app_filter (self->manager,
                                             act_user_get_uid (self->user),
                                             MCT_MANAGER_GET_VALUE_FLAGS_NONE,
                                             self->cancellable,
                                             &error);

  if (error)
    {
      g_warning ("Error retrieving app filter for user '%s': %s",
                 act_user_get_user_name (self->user),
                 error->message);
      return;
    }

  g_debug ("Retrieved new app filter for user '%s'", act_user_get_user_name (self->user));
}

static void
update_categories_from_language (MctUserControls *self)
{
  GsContentRatingSystem rating_system;
  const gchar * const * entries;
  const gchar *rating_system_str;
  const guint *ages;
  gsize i;
  g_autofree gchar *disabled_action = NULL;

  rating_system = get_content_rating_system (self);
  rating_system_str = gs_content_rating_system_to_str (rating_system);

  g_debug ("Using rating system %s", rating_system_str);

  entries = gs_utils_content_rating_get_values (rating_system);
  ages = gs_utils_content_rating_get_ages (rating_system);

  /* Fill in the age menu */
  g_menu_remove_all (self->age_menu);

  disabled_action = g_strdup_printf ("permissions.set-age(uint32 %u)", oars_disabled_age);
  g_menu_append (self->age_menu, _("All Ages"), disabled_action);

  for (i = 0; entries[i] != NULL; i++)
    {
      g_autofree gchar *action = g_strdup_printf ("permissions.set-age(uint32 %u)", ages[i]);

      /* Prevent the unlikely case that one of the real ages is the same as our
       * special ‘disabled’ value. */
      g_assert (ages[i] != oars_disabled_age);

      g_menu_append (self->age_menu, entries[i], action);
    }
}

/* Returns a human-readable but untranslated string, not suitable
 * to be shown in any UI */
static const gchar *
oars_value_to_string (MctAppFilterOarsValue oars_value)
{
  switch (oars_value)
    {
    case MCT_APP_FILTER_OARS_VALUE_UNKNOWN:
      return "unknown";
    case MCT_APP_FILTER_OARS_VALUE_NONE:
      return "none";
    case MCT_APP_FILTER_OARS_VALUE_MILD:
      return "mild";
    case MCT_APP_FILTER_OARS_VALUE_MODERATE:
      return "moderate";
    case MCT_APP_FILTER_OARS_VALUE_INTENSE:
      return "intense";
    default:
      return "";
    }
}

static void
update_oars_level (MctUserControls *self)
{
  GsContentRatingSystem rating_system;
  const gchar *rating_age_category;
  guint maximum_age;
  gsize i;
  gboolean all_categories_unset;

  g_assert (self->filter != NULL);

  maximum_age = 0;
  all_categories_unset = TRUE;

  for (i = 0; oars_categories[i] != NULL; i++)
    {
      MctAppFilterOarsValue oars_value;
      guint age;

      oars_value = mct_app_filter_get_oars_value (self->filter, oars_categories[i]);
      all_categories_unset &= (oars_value == MCT_APP_FILTER_OARS_VALUE_UNKNOWN);
      age = as_content_rating_id_value_to_csm_age (oars_categories[i], oars_value);

      g_debug ("OARS value for '%s': %s", oars_categories[i], oars_value_to_string (oars_value));

      if (age > maximum_age)
        maximum_age = age;
    }

  g_debug ("Effective age for this user: %u; %s", maximum_age,
           all_categories_unset ? "all categories unset" : "some categories set");

  rating_system = get_content_rating_system (self);
  rating_age_category = gs_utils_content_rating_age_to_str (rating_system, maximum_age);

  /* Unrestricted? */
  if (rating_age_category == NULL || all_categories_unset)
    rating_age_category = _("All Ages");

  gtk_button_set_label (self->restriction_button, rating_age_category);
}

static void
update_allow_app_installation (MctUserControls *self)
{
  gboolean allow_system_installation;
  gboolean allow_user_installation;
  gboolean non_admin_user = TRUE;

  if (act_user_get_account_type (self->user) == ACT_USER_ACCOUNT_TYPE_ADMINISTRATOR)
    non_admin_user = FALSE;

  /* Admins are always allowed to install apps for all users. This behaviour is governed
   * by flatpak polkit rules. Hence, these hide these defunct switches for admins. */
  gtk_widget_set_visible (GTK_WIDGET (self->allow_system_installation_switch), non_admin_user);
  gtk_widget_set_visible (GTK_WIDGET (self->allow_user_installation_switch), non_admin_user);

  /* If user is admin, we are done here, bail out. */
  if (!non_admin_user)
    {
      g_debug ("User %s is administrator, hiding app installation controls",
               act_user_get_user_name (self->user));
      return;
    }

  allow_system_installation = mct_app_filter_is_system_installation_allowed (self->filter);
  allow_user_installation = mct_app_filter_is_user_installation_allowed (self->filter);

  /* While the underlying permissions storage allows the system and user settings
   * to be stored completely independently, force the system setting to OFF if
   * the user setting is OFF in the UI. This keeps the policy in use for most
   * people simpler. */
  if (!allow_user_installation)
    allow_system_installation = FALSE;

  g_signal_handlers_block_by_func (self->allow_system_installation_switch,
                                   on_allow_installation_switch_active_changed_cb,
                                   self);

  g_signal_handlers_block_by_func (self->allow_user_installation_switch,
                                   on_allow_installation_switch_active_changed_cb,
                                   self);

  gtk_switch_set_active (self->allow_system_installation_switch, allow_system_installation);
  gtk_switch_set_active (self->allow_user_installation_switch, allow_user_installation);

  g_debug ("Allow system installation: %s", allow_system_installation ? "yes" : "no");
  g_debug ("Allow user installation: %s", allow_user_installation ? "yes" : "no");

  g_signal_handlers_unblock_by_func (self->allow_system_installation_switch,
                                     on_allow_installation_switch_active_changed_cb,
                                     self);

  g_signal_handlers_unblock_by_func (self->allow_user_installation_switch,
                                     on_allow_installation_switch_active_changed_cb,
                                     self);
}

static void
update_allow_web_browsers (MctUserControls *self)
{
  gboolean allow_web_browsers;

  allow_web_browsers = mct_app_filter_is_content_type_allowed (self->filter,
                                                               WEB_BROWSERS_CONTENT_TYPE);

  g_signal_handlers_block_by_func (self->allow_web_browsers_switch,
                                   on_allow_web_browsers_switch_active_changed_cb,
                                   self);

  gtk_switch_set_active (self->allow_web_browsers_switch, allow_web_browsers);

  g_debug ("Allow web browsers: %s", allow_web_browsers ? "yes" : "no");

  g_signal_handlers_unblock_by_func (self->allow_web_browsers_switch,
                                     on_allow_web_browsers_switch_active_changed_cb,
                                     self);
}

static void
setup_parental_control_settings (MctUserControls *self)
{
  gboolean is_authorized;

  gtk_widget_set_visible (GTK_WIDGET (self), self->filter != NULL);

  if (!self->filter)
    return;

  /* We only want to make the controls sensitive if we have permission to save
   * changes (@is_authorized). */
  if (self->permission != NULL)
    is_authorized = g_permission_get_allowed (G_PERMISSION (self->permission));
  else
    is_authorized = FALSE;

  gtk_widget_set_sensitive (GTK_WIDGET (self), is_authorized);

  update_oars_level (self);
  update_categories_from_language (self);
  update_allow_app_installation (self);
  update_allow_web_browsers (self);
}

/* Callbacks */

static gboolean
blacklist_apps_cb (gpointer data)
{
  g_auto(MctAppFilterBuilder) builder = MCT_APP_FILTER_BUILDER_INIT ();
  g_autoptr(MctAppFilter) new_filter = NULL;
  g_autoptr(GError) error = NULL;
  MctUserControls *self = data;
  gboolean allow_web_browsers;
  gsize i;

  self->blacklist_apps_source_id = 0;

  g_debug ("Building parental controls settings…");

  /* Blacklist */

  g_debug ("\t → Blacklisting apps");

  mct_restrict_applications_dialog_build_app_filter (self->restrict_applications_dialog, &builder);

  /* Maturity level */

  g_debug ("\t → Maturity level");

  if (self->selected_age == oars_disabled_age)
    g_debug ("\t\t → Disabled");

  for (i = 0; self->selected_age != oars_disabled_age && oars_categories[i] != NULL; i++)
    {
      MctAppFilterOarsValue oars_value;
      const gchar *oars_category;

      oars_category = oars_categories[i];
      oars_value = as_content_rating_id_csm_age_to_value (oars_category, self->selected_age);

      g_debug ("\t\t → %s: %s", oars_category, oars_value_to_string (oars_value));

      mct_app_filter_builder_set_oars_value (&builder, oars_category, oars_value);
    }

  /* Web browsers */
  allow_web_browsers = gtk_switch_get_active (self->allow_web_browsers_switch);

  g_debug ("\t → %s web browsers", allow_web_browsers ? "Enabling" : "Disabling");

  if (!allow_web_browsers)
    mct_app_filter_builder_blacklist_content_type (&builder, WEB_BROWSERS_CONTENT_TYPE);

  /* App installation */
  if (act_user_get_account_type (self->user) != ACT_USER_ACCOUNT_TYPE_ADMINISTRATOR)
    {
      gboolean allow_system_installation;
      gboolean allow_user_installation;

      allow_system_installation = gtk_switch_get_active (self->allow_system_installation_switch);
      allow_user_installation = gtk_switch_get_active (self->allow_user_installation_switch);

      g_debug ("\t → %s system installation", allow_system_installation ? "Enabling" : "Disabling");
      g_debug ("\t → %s user installation", allow_user_installation ? "Enabling" : "Disabling");

      mct_app_filter_builder_set_allow_user_installation (&builder, allow_user_installation);
      mct_app_filter_builder_set_allow_system_installation (&builder, allow_system_installation);
    }

  new_filter = mct_app_filter_builder_end (&builder);

  /* FIXME: should become asynchronous */
  mct_manager_set_app_filter (self->manager,
                              act_user_get_uid (self->user),
                              new_filter,
                              MCT_MANAGER_SET_VALUE_FLAGS_INTERACTIVE,
                              self->cancellable,
                              &error);

  if (error)
    {
      g_warning ("Error updating app filter: %s", error->message);
      setup_parental_control_settings (self);
    }

  return G_SOURCE_REMOVE;
}

static void
on_allow_installation_switch_active_changed_cb (GtkSwitch        *s,
                                                GParamSpec       *pspec,
                                                MctUserControls *self)
{
  /* See the comment about policy in update_allow_app_installation(). */
  if (s == self->allow_user_installation_switch &&
      !gtk_switch_get_active (s) &&
      gtk_switch_get_active (self->allow_system_installation_switch))
    {
      g_signal_handlers_block_by_func (self->allow_system_installation_switch,
                                       on_allow_installation_switch_active_changed_cb,
                                       self);
      gtk_switch_set_active (self->allow_system_installation_switch, FALSE);
      g_signal_handlers_unblock_by_func (self->allow_system_installation_switch,
                                         on_allow_installation_switch_active_changed_cb,
                                         self);
    }

  /* Save the changes. */
  schedule_update_blacklisted_apps (self);
}

static void
on_allow_web_browsers_switch_active_changed_cb (GtkSwitch        *s,
                                                GParamSpec       *pspec,
                                                MctUserControls *self)
{
  /* Save the changes. */
  schedule_update_blacklisted_apps (self);
}

static void
on_restrict_applications_button_clicked_cb (GtkButton *button,
                                            gpointer   user_data)
{
  MctUserControls *self = MCT_USER_CONTROLS (user_data);
  GtkWidget *toplevel;

  /* Show the restrict applications dialogue modally, making sure to update its
   * state first. */
  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (self));
  if (GTK_IS_WINDOW (toplevel))
    gtk_window_set_transient_for (GTK_WINDOW (self->restrict_applications_dialog),
                                  GTK_WINDOW (toplevel));

  mct_restrict_applications_dialog_set_user_display_name (self->restrict_applications_dialog, get_user_display_name (self->user));
  mct_restrict_applications_dialog_set_app_filter (self->restrict_applications_dialog, self->filter);

  gtk_widget_show (GTK_WIDGET (self->restrict_applications_dialog));
}

static gboolean
on_restrict_applications_dialog_delete_event_cb (GtkWidget *widget,
                                                 GdkEvent  *event,
                                                 gpointer   user_data)
{
  MctUserControls *self = MCT_USER_CONTROLS (user_data);

  /* When the ‘Restrict Applications’ dialogue is closed, don’t destroy it,
   * since it contains the app filter settings which we’ll want to reuse next
   * time the dialogue is shown or the app filter is saved. */
  gtk_widget_hide (GTK_WIDGET (self->restrict_applications_dialog));

  /* Schedule an update to the saved state. */
  schedule_update_blacklisted_apps (self);

  return TRUE;
}

static void
on_restrict_applications_dialog_response_cb (GtkDialog *dialog,
                                             gint       response_id,
                                             gpointer   user_data)
{
  MctUserControls *self = MCT_USER_CONTROLS (user_data);

  on_restrict_applications_dialog_delete_event_cb (GTK_WIDGET (dialog), NULL, self);
}

static void
on_set_age_action_activated (GSimpleAction *action,
                             GVariant      *param,
                             gpointer       user_data)
{
  GsContentRatingSystem rating_system;
  MctUserControls *self;
  const gchar * const * entries;
  const guint *ages;
  guint age;
  guint i;

  self = MCT_USER_CONTROLS (user_data);
  age = g_variant_get_uint32 (param);

  rating_system = get_content_rating_system (self);
  entries = gs_utils_content_rating_get_values (rating_system);
  ages = gs_utils_content_rating_get_ages (rating_system);

  /* Update the button */
  if (age == oars_disabled_age)
    gtk_button_set_label (self->restriction_button, _("All Ages"));

  for (i = 0; age != oars_disabled_age && entries[i] != NULL; i++)
    {
      if (ages[i] == age)
        {
          gtk_button_set_label (self->restriction_button, entries[i]);
          break;
        }
    }

  g_assert (age == oars_disabled_age || entries[i] != NULL);

  if (age == oars_disabled_age)
    g_debug ("Selected to disable OARS");
  else
    g_debug ("Selected OARS age: %u", age);

  self->selected_age = age;

  schedule_update_blacklisted_apps (self);
}

/* GObject overrides */

static void
mct_user_controls_finalize (GObject *object)
{
  MctUserControls *self = (MctUserControls *)object;

  g_assert (self->blacklist_apps_source_id == 0);

  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->action_group);
  g_clear_object (&self->cancellable);
  g_clear_object (&self->user);

  if (self->permission != NULL && self->permission_allowed_id != 0)
    {
      g_signal_handler_disconnect (self->permission, self->permission_allowed_id);
      self->permission_allowed_id = 0;
    }
  g_clear_object (&self->permission);

  g_clear_pointer (&self->filter, mct_app_filter_unref);
  g_clear_object (&self->manager);

  /* Hopefully we don’t have data loss. */
  g_assert (self->flushed_on_dispose);

  G_OBJECT_CLASS (mct_user_controls_parent_class)->finalize (object);
}


static void
mct_user_controls_dispose (GObject *object)
{
  MctUserControls *self = (MctUserControls *)object;

  /* Since GTK calls g_object_run_dispose(), dispose() may be called multiple
   * times. We definitely want to save any unsaved changes, but don’t need to
   * do it multiple times, and after the first g_object_run_dispose() call,
   * none of our child widgets are still around to extract data from anyway. */
  if (!self->flushed_on_dispose)
    flush_update_blacklisted_apps (self);
  self->flushed_on_dispose = TRUE;

  G_OBJECT_CLASS (mct_user_controls_parent_class)->dispose (object);
}

static void
mct_user_controls_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  MctUserControls *self = MCT_USER_CONTROLS (object);

  switch ((MctUserControlsProperty) prop_id)
    {
    case PROP_USER:
      g_value_set_object (value, self->user);
      break;

    case PROP_PERMISSION:
      g_value_set_object (value, self->permission);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
mct_user_controls_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  MctUserControls *self = MCT_USER_CONTROLS (object);

  switch ((MctUserControlsProperty) prop_id)
    {
    case PROP_USER:
      mct_user_controls_set_user (self, g_value_get_object (value));
      break;

    case PROP_PERMISSION:
      mct_user_controls_set_permission (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
mct_user_controls_class_init (MctUserControlsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = mct_user_controls_finalize;
  object_class->dispose = mct_user_controls_dispose;
  object_class->get_property = mct_user_controls_get_property;
  object_class->set_property = mct_user_controls_set_property;

  properties[PROP_USER] = g_param_spec_object ("user",
                                               "User",
                                               "User",
                                               ACT_TYPE_USER,
                                               G_PARAM_READWRITE |
                                               G_PARAM_STATIC_STRINGS |
                                               G_PARAM_EXPLICIT_NOTIFY);

  properties[PROP_PERMISSION] = g_param_spec_object ("permission",
                                                     "Permission",
                                                     "Permission to change parental controls",
                                                     G_TYPE_PERMISSION,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_STATIC_STRINGS |
                                                     G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/freedesktop/MalcontentUi/ui/user-controls.ui");

  gtk_widget_class_bind_template_child (widget_class, MctUserControls, age_menu);
  gtk_widget_class_bind_template_child (widget_class, MctUserControls, allow_system_installation_switch);
  gtk_widget_class_bind_template_child (widget_class, MctUserControls, allow_user_installation_switch);
  gtk_widget_class_bind_template_child (widget_class, MctUserControls, allow_web_browsers_switch);
  gtk_widget_class_bind_template_child (widget_class, MctUserControls, restriction_button);
  gtk_widget_class_bind_template_child (widget_class, MctUserControls, restriction_popover);
  gtk_widget_class_bind_template_child (widget_class, MctUserControls, restrict_applications_dialog);

  gtk_widget_class_bind_template_callback (widget_class, on_allow_installation_switch_active_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_allow_web_browsers_switch_active_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_restrict_applications_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_restrict_applications_dialog_delete_event_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_restrict_applications_dialog_response_cb);
}

static void
mct_user_controls_init (MctUserControls *self)
{
  g_autoptr(GDBusConnection) system_bus = NULL;
  g_autoptr(GError) error = NULL;

  /* Ensure the types used in the UI are registered. */
  g_type_ensure (MCT_TYPE_RESTRICT_APPLICATIONS_DIALOG);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->selected_age = (guint) -1;

  self->cancellable = g_cancellable_new ();

  /* FIXME: should become asynchronous */
  system_bus = g_bus_get_sync (G_BUS_TYPE_SYSTEM, self->cancellable, &error);
  if (system_bus == NULL)
    {
      g_warning ("Error getting system bus while setting up app permissions: %s", error->message);
      return;
    }

  self->manager = mct_manager_new (system_bus);

  self->action_group = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (self->action_group),
                                   actions,
                                   G_N_ELEMENTS (actions),
                                   self);

  gtk_widget_insert_action_group (GTK_WIDGET (self),
                                  "permissions",
                                  G_ACTION_GROUP (self->action_group));

  gtk_popover_bind_model (self->restriction_popover, G_MENU_MODEL (self->age_menu), NULL);

  g_object_bind_property (self->allow_user_installation_switch, "active",
                          self->allow_system_installation_switch, "sensitive",
                          G_BINDING_DEFAULT);
}

ActUser *
mct_user_controls_get_user (MctUserControls *self)
{
  g_return_val_if_fail (MCT_IS_USER_CONTROLS (self), NULL);

  return self->user;
}

void
mct_user_controls_set_user (MctUserControls *self,
                            ActUser         *user)
{
  g_return_if_fail (MCT_IS_USER_CONTROLS (self));
  g_return_if_fail (user == NULL || ACT_IS_USER (user));

  /* If we have pending unsaved changes from the previous user, force them to be
   * saved first. */
  flush_update_blacklisted_apps (self);

  if (g_set_object (&self->user, user))
    {
      update_app_filter (self);
      setup_parental_control_settings (self);

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_USER]);
    }
}

static void
on_permission_allowed_cb (GObject    *obj,
                          GParamSpec *pspec,
                          gpointer    user_data)
{
  MctUserControls *self = MCT_USER_CONTROLS (user_data);

  update_app_filter (self);
  setup_parental_control_settings (self);
}

GPermission *  /* (nullable) */
mct_user_controls_get_permission (MctUserControls *self)
{
  g_return_val_if_fail (MCT_IS_USER_CONTROLS (self), NULL);

  return self->permission;
}

void
mct_user_controls_set_permission (MctUserControls *self,
                                  GPermission     *permission  /* (nullable) */)
{
  g_return_if_fail (MCT_IS_USER_CONTROLS (self));
  g_return_if_fail (permission == NULL || G_IS_PERMISSION (permission));

  if (self->permission == permission)
    return;

  if (self->permission != NULL && self->permission_allowed_id != 0)
    {
      g_signal_handler_disconnect (self->permission, self->permission_allowed_id);
      self->permission_allowed_id = 0;
    }

  g_clear_object (&self->permission);

  if (permission != NULL)
    {
      self->permission = g_object_ref (permission);
      self->permission_allowed_id = g_signal_connect (self->permission,
                                                      "notify::allowed",
                                                      (GCallback) on_permission_allowed_cb,
                                                      self);
    }

  /* Handle changes. */
  update_app_filter (self);
  setup_parental_control_settings (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PERMISSION]);
}
