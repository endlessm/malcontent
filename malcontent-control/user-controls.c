/* cc-app-permissions.c
 *
 * Copyright 2018, 2019 Endless, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <libmalcontent/malcontent.h>
#include <flatpak.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>
#include <strings.h>

#include "gs-content-rating.h"

#include "cc-app-permissions.h"

#define WEB_BROWSERS_CONTENT_TYPE "x-scheme-handler/http"

/* The value which we store as an age to indicate that OARS filtering is disabled. */
static const guint32 oars_disabled_age = (guint32) -1;

struct _CcAppPermissions
{
  GtkGrid     parent_instance;

  GMenu      *age_menu;
  GtkSwitch  *allow_system_installation_switch;
  GtkSwitch  *allow_user_installation_switch;
  GtkSwitch  *allow_web_browsers_switch;
  GtkListBox *listbox;
  GtkButton  *restriction_button;
  GtkPopover *restriction_popover;

  FlatpakInstallation *system_installation; /* (owned) */
  FlatpakInstallation *user_installation; /* (owned) */

  GSimpleActionGroup *action_group; /* (owned) */

  ActUser    *user; /* (owned) */

  GPermission *permission;  /* (owned) (nullable) */
  gulong permission_allowed_id;

  GAppInfoMonitor *app_info_monitor;  /* (owned) */

  GHashTable *blacklisted_apps; /* (owned) */
  GListStore *apps; /* (owned) */

  GCancellable *cancellable; /* (owned) */
  MctManager   *manager; /* (owned) */
  MctAppFilter *filter; /* (owned) */
  guint         selected_age; /* @oars_disabled_age to disable OARS */

  guint         blacklist_apps_source_id;
};

static gboolean blacklist_apps_cb (gpointer data);
static void app_info_changed_cb (GAppInfoMonitor *monitor,
                                 gpointer         user_data);

static gint compare_app_info_cb (gconstpointer a,
                                 gconstpointer b,
                                 gpointer      user_data);

static void on_allow_installation_switch_active_changed_cb (GtkSwitch        *s,
                                                            GParamSpec       *pspec,
                                                            CcAppPermissions *self);

static void on_allow_web_browsers_switch_active_changed_cb (GtkSwitch        *s,
                                                            GParamSpec       *pspec,
                                                            CcAppPermissions *self);

static void on_set_age_action_activated (GSimpleAction *action,
                                         GVariant      *param,
                                         gpointer       user_data);

static void on_permission_allowed_cb (GObject    *obj,
                                      GParamSpec *pspec,
                                      gpointer    user_data);

G_DEFINE_TYPE (CcAppPermissions, cc_app_permissions, GTK_TYPE_GRID)

enum
{
  PROP_USER = 1,
  PROP_PERMISSION,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

static const GActionEntry actions[] = {
  { "set-age", on_set_age_action_activated, "u", NULL, NULL, { 0, }}
};

/* FIXME: Factor this out and rely on code from libappstream-glib or gnome-software
 * to do it. See: https://phabricator.endlessm.com/T24986 */
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

static gint
app_compare_id_length_cb (gconstpointer a,
                          gconstpointer b)
{
  GAppInfo *info_a = (GAppInfo *) a, *info_b = (GAppInfo *) b;
  const gchar *id_a, *id_b;

  id_a = g_app_info_get_id (info_a);
  id_b = g_app_info_get_id (info_b);

  if (id_a == NULL && id_b == NULL)
    return 0;
  else if (id_a == NULL)
    return -1;
  else if (id_b == NULL)
    return 1;

  return strlen (id_a) - strlen (id_b);
}

static void
reload_apps (CcAppPermissions *self)
{
  GList *iter, *apps;
  g_autoptr(GHashTable) seen_flatpak_ids = NULL;
  g_autoptr(GHashTable) seen_executables = NULL;

  apps = g_app_info_get_all ();

  /* Sort the apps by increasing length of #GAppInfo ID. When coupled with the
   * deduplication of flatpak IDs and executable paths, below, this should ensure that we
   * pick the ‘base’ app out of any set with matching prefixes and identical app IDs (in
   * case of flatpak apps) or executables (for non-flatpak apps), and show only that.
   *
   * This is designed to avoid listing all the components of LibreOffice for example,
   * which all share an app ID and hence have the same entry in the parental controls
   * app filter. */
  apps = g_list_sort (apps, app_compare_id_length_cb);
  seen_flatpak_ids = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  seen_executables = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  g_list_store_remove_all (self->apps);

  for (iter = apps; iter; iter = iter->next)
    {
      GAppInfo *app;
      const gchar *app_name;
      const gchar * const *supported_types;

      app = iter->data;
      app_name = g_app_info_get_name (app);

      supported_types = g_app_info_get_supported_types (app);

      if (!G_IS_DESKTOP_APP_INFO (app) ||
          !g_app_info_should_show (app) ||
          app_name[0] == '\0' ||
          /* Endless' link apps have the "eos-link" prefix, and should be ignored too */
          g_str_has_prefix (g_app_info_get_id (app), "eos-link") ||
          /* FIXME: Only list flatpak apps and apps with X-Parental-Controls
           * key set for now; we really need a system-wide MAC to be able to
           * reliably support blacklisting system programs. See
           * https://phabricator.endlessm.com/T25080. */
          (!g_desktop_app_info_has_key (G_DESKTOP_APP_INFO (app), "X-Flatpak") &&
           !g_desktop_app_info_has_key (G_DESKTOP_APP_INFO (app), "X-Parental-Controls")) ||
          /* Web browsers are special cased */
          (supported_types && g_strv_contains (supported_types, WEB_BROWSERS_CONTENT_TYPE)))
        {
          continue;
        }

      if (g_desktop_app_info_has_key (G_DESKTOP_APP_INFO (app), "X-Flatpak"))
        {
          g_autofree gchar *flatpak_id = NULL;

          flatpak_id = g_desktop_app_info_get_string (G_DESKTOP_APP_INFO (app), "X-Flatpak");
          g_debug ("Processing app ‘%s’ (Exec=%s, X-Flatpak=%s)",
                   g_app_info_get_id (app),
                   g_app_info_get_executable (app),
                   flatpak_id);

          /* Have we seen this flatpak ID before? */
          if (!g_hash_table_add (seen_flatpak_ids, g_steal_pointer (&flatpak_id)))
            {
              g_debug (" → Skipping ‘%s’ due to seeing its flatpak ID already",
                       g_app_info_get_id (app));
              continue;
            }
        }
      else if (g_desktop_app_info_has_key (G_DESKTOP_APP_INFO (app), "X-Parental-Controls"))
        {
          g_autofree gchar *parental_controls_type = NULL;
          g_autofree gchar *executable = NULL;

          parental_controls_type = g_desktop_app_info_get_string (G_DESKTOP_APP_INFO (app),
                                                                  "X-Parental-Controls");
          /* Ignore X-Parental-Controls=none */
          if (g_strcmp0 (parental_controls_type, "none") == 0)
            continue;

          executable = g_strdup (g_app_info_get_executable (app));
          g_debug ("Processing app ‘%s’ (Exec=%s, X-Parental-Controls=%s)",
                   g_app_info_get_id (app),
                   executable,
                   parental_controls_type);

          /* Have we seen this executable before? */
          if (!g_hash_table_add (seen_executables, g_steal_pointer (&executable)))
            {
              g_debug (" → Skipping ‘%s’ due to seeing its executable already",
                       g_app_info_get_id (app));
              continue;
            }
        }

      g_list_store_insert_sorted (self->apps,
                                  app,
                                  compare_app_info_cb,
                                  self);
    }

  g_list_free_full (apps, g_object_unref);
}

static void
app_info_changed_cb (GAppInfoMonitor *monitor,
                     gpointer         user_data)
{
  CcAppPermissions *self = CC_APP_PERMISSIONS (user_data);

  reload_apps (self);
}

static GsContentRatingSystem
get_content_rating_system (ActUser *user)
{
  const gchar *user_language;

  user_language = act_user_get_language (user);

  return gs_utils_content_rating_system_from_locale (user_language);
}

static void
schedule_update_blacklisted_apps (CcAppPermissions *self)
{
  if (self->blacklist_apps_source_id > 0)
    return;

  /* Use a timeout to batch multiple quick changes into a single
   * update. 1 second is an arbitrary sufficiently small number */
  self->blacklist_apps_source_id = g_timeout_add_seconds (1, blacklist_apps_cb, self);
}

static void
flush_update_blacklisted_apps (CcAppPermissions *self)
{
  if (self->blacklist_apps_source_id > 0)
    {
      blacklist_apps_cb (self);
      g_source_remove (self->blacklist_apps_source_id);
      self->blacklist_apps_source_id = 0;
    }
}

static void
update_app_filter (CcAppPermissions *self)
{
  g_autoptr(GError) error = NULL;

  g_clear_pointer (&self->filter, mct_app_filter_unref);

  /* FIXME: make it asynchronous */
  self->filter = mct_manager_get_app_filter (self->manager,
                                             act_user_get_uid (self->user),
                                             MCT_GET_APP_FILTER_FLAGS_NONE,
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
update_categories_from_language (CcAppPermissions *self)
{
  GsContentRatingSystem rating_system;
  const gchar * const * entries;
  const gchar *rating_system_str;
  const guint *ages;
  gsize i;
  g_autofree gchar *disabled_action = NULL;

  rating_system = get_content_rating_system (self->user);
  rating_system_str = gs_content_rating_system_to_str (rating_system);

  g_debug ("Using rating system %s", rating_system_str);

  entries = gs_utils_content_rating_get_values (rating_system);
  ages = gs_utils_content_rating_get_ages (rating_system);

  /* Fill in the age menu */
  g_menu_remove_all (self->age_menu);

  disabled_action = g_strdup_printf ("permissions.set-age(uint32 %u)", oars_disabled_age);
  g_menu_append (self->age_menu, _("No Restriction"), disabled_action);

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
static const gchar*
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
    }
  return "";
}

static void
update_oars_level (CcAppPermissions *self)
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

  rating_system = get_content_rating_system (self->user);
  rating_age_category = gs_utils_content_rating_age_to_str (rating_system, maximum_age);

  /* Unrestricted? */
  if (rating_age_category == NULL || all_categories_unset)
    rating_age_category = _("No Restriction");

  gtk_button_set_label (self->restriction_button, rating_age_category);
}

static void
update_allow_app_installation (CcAppPermissions *self)
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
update_allow_web_browsers (CcAppPermissions *self)
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
setup_parental_control_settings (CcAppPermissions *self)
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
    is_authorized = TRUE;

  gtk_widget_set_sensitive (GTK_WIDGET (self), is_authorized);

  g_hash_table_remove_all (self->blacklisted_apps);

  update_oars_level (self);
  update_categories_from_language (self);
  update_allow_app_installation (self);
  update_allow_web_browsers (self);
  reload_apps (self);
}

/* Will return %NULL if @flatpak_id is not installed. */
static gchar *
get_flatpak_ref_for_app_id (CcAppPermissions *self,
                            const gchar      *flatpak_id)
{
  g_autoptr(FlatpakInstalledRef) ref = NULL;
  g_autoptr(GError) error = NULL;

  g_assert (self->system_installation != NULL);
  g_assert (self->user_installation != NULL);

  ref = flatpak_installation_get_current_installed_app (self->user_installation,
                                                        flatpak_id,
                                                        self->cancellable,
                                                        &error);

  if (error &&
      !g_error_matches (error, FLATPAK_ERROR, FLATPAK_ERROR_NOT_INSTALLED))
    {
      g_warning ("Error searching for Flatpak ref: %s", error->message);
      return NULL;
    }

  g_clear_error (&error);

  if (!ref || !flatpak_installed_ref_get_is_current (ref))
    {
      ref = flatpak_installation_get_current_installed_app (self->system_installation,
                                                            flatpak_id,
                                                            self->cancellable,
                                                            &error);
      if (error)
        {
          if (!g_error_matches (error, FLATPAK_ERROR, FLATPAK_ERROR_NOT_INSTALLED))
            g_warning ("Error searching for Flatpak ref: %s", error->message);
          return NULL;
        }
    }

  return flatpak_ref_format_ref (FLATPAK_REF (ref));
}

/* Callbacks */

static gboolean
blacklist_apps_cb (gpointer data)
{
  g_auto(MctAppFilterBuilder) builder = MCT_APP_FILTER_BUILDER_INIT ();
  g_autoptr(MctAppFilter) new_filter = NULL;
  g_autoptr(GError) error = NULL;
  CcAppPermissions *self = data;
  GDesktopAppInfo *app;
  GHashTableIter iter;
  gboolean allow_web_browsers;
  gsize i;

  self->blacklist_apps_source_id = 0;

  g_debug ("Building parental controls settings…");

  /* Blacklist */

  g_debug ("\t → Blacklisting apps");

  g_hash_table_iter_init (&iter, self->blacklisted_apps);
  while (g_hash_table_iter_next (&iter, (gpointer) &app, NULL))
    {
      g_autofree gchar *flatpak_id = NULL;

      flatpak_id = g_desktop_app_info_get_string (app, "X-Flatpak");
      if (flatpak_id)
        flatpak_id = g_strstrip (flatpak_id);

      if (flatpak_id)
        {
          g_autofree gchar *flatpak_ref = get_flatpak_ref_for_app_id (self, flatpak_id);

          if (!flatpak_ref)
            {
              g_warning ("Skipping blacklisting Flatpak ID ‘%s’ due to it not being installed", flatpak_id);
              continue;
            }

          g_debug ("\t\t → Blacklisting Flatpak ref: %s", flatpak_ref);
          mct_app_filter_builder_blacklist_flatpak_ref (&builder, flatpak_ref);
        }
      else
        {
          const gchar *executable = g_app_info_get_executable (G_APP_INFO (app));
          g_autofree gchar *path = g_find_program_in_path (executable);

          if (!path)
            {
              g_warning ("Skipping blacklisting executable ‘%s’ due to it not being found", executable);
              continue;
            }

          g_debug ("\t\t → Blacklisting path: %s", path);
          mct_app_filter_builder_blacklist_path (&builder, path);
        }
    }

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
                              MCT_GET_APP_FILTER_FLAGS_INTERACTIVE,
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
                                                CcAppPermissions *self)
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
                                                CcAppPermissions *self)
{
  /* Save the changes. */
  schedule_update_blacklisted_apps (self);
}

static void
on_switch_active_changed_cb (GtkSwitch        *s,
                             GParamSpec       *pspec,
                             CcAppPermissions *self)
{
  GAppInfo *app;
  gboolean allowed;

  app = g_object_get_data (G_OBJECT (s), "GAppInfo");
  allowed = gtk_switch_get_active (s);

  if (allowed)
    {
      gboolean removed;

      g_debug ("Removing '%s' from blacklisted apps", g_app_info_get_id (app));

      removed = g_hash_table_remove (self->blacklisted_apps, app);
      g_assert (removed);
    }
  else
    {
      gboolean added;

      g_debug ("Blacklisting '%s'", g_app_info_get_id (app));

      added = g_hash_table_add (self->blacklisted_apps, g_object_ref (app));
      g_assert (added);
    }

  schedule_update_blacklisted_apps (self);
}

static GtkWidget *
create_row_for_app_cb (gpointer item,
                       gpointer user_data)
{
  g_autoptr(GIcon) icon = NULL;
  CcAppPermissions *self;
  GtkWidget *box, *w;
  GAppInfo *app;
  gboolean allowed;
  const gchar *app_name;
  gint size;

  self = CC_APP_PERMISSIONS (user_data);
  app = item;
  app_name = g_app_info_get_name (app);

  g_assert (G_IS_DESKTOP_APP_INFO (app));

  icon = g_app_info_get_icon (app);
  if (icon == NULL)
    icon = g_themed_icon_new ("application-x-executable");
  else
    g_object_ref (icon);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (box), 12);
  gtk_widget_set_margin_end (box, 12);

  /* Icon */
  w = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_DIALOG);
  gtk_icon_size_lookup (GTK_ICON_SIZE_DND, &size, NULL);
  gtk_image_set_pixel_size (GTK_IMAGE (w), size);
  gtk_container_add (GTK_CONTAINER (box), w);

  /* App name label */
  w = g_object_new (GTK_TYPE_LABEL,
                    "label", app_name,
                    "hexpand", TRUE,
                    "xalign", 0.0,
                    NULL);
  gtk_container_add (GTK_CONTAINER (box), w);

  /* Switch */
  w = g_object_new (GTK_TYPE_SWITCH,
                    "valign", GTK_ALIGN_CENTER,
                    NULL);
  gtk_container_add (GTK_CONTAINER (box), w);

  gtk_widget_show_all (box);

  /* Fetch status from AccountService */
  allowed = mct_app_filter_is_appinfo_allowed (self->filter, app);

  gtk_switch_set_active (GTK_SWITCH (w), allowed);
  g_object_set_data_full (G_OBJECT (w), "GAppInfo", g_object_ref (app), g_object_unref);

  if (allowed)
    g_hash_table_remove (self->blacklisted_apps, app);
  else if (!allowed)
    g_hash_table_add (self->blacklisted_apps, g_object_ref (app));

  g_signal_connect (w, "notify::active", G_CALLBACK (on_switch_active_changed_cb), self);

  return box;
}

static gint
compare_app_info_cb (gconstpointer a,
                     gconstpointer b,
                     gpointer      user_data)
{
  GAppInfo *app_a = (GAppInfo*) a;
  GAppInfo *app_b = (GAppInfo*) b;

  return g_utf8_collate (g_app_info_get_display_name (app_a),
                         g_app_info_get_display_name (app_b));
}

static void
on_set_age_action_activated (GSimpleAction *action,
                             GVariant      *param,
                             gpointer       user_data)
{
  GsContentRatingSystem rating_system;
  CcAppPermissions *self;
  const gchar * const * entries;
  const guint *ages;
  guint age;
  guint i;

  self = CC_APP_PERMISSIONS (user_data);
  age = g_variant_get_uint32 (param);

  rating_system = get_content_rating_system (self->user);
  entries = gs_utils_content_rating_get_values (rating_system);
  ages = gs_utils_content_rating_get_ages (rating_system);

  /* Update the button */
  if (age == oars_disabled_age)
    gtk_button_set_label (self->restriction_button, _("No Restriction"));

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
cc_app_permissions_finalize (GObject *object)
{
  CcAppPermissions *self = (CcAppPermissions *)object;

  g_assert (self->blacklist_apps_source_id == 0);

  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->action_group);
  g_clear_object (&self->apps);
  g_clear_object (&self->cancellable);
  g_clear_object (&self->system_installation);
  g_clear_object (&self->user);
  g_clear_object (&self->user_installation);

  if (self->permission != NULL && self->permission_allowed_id != 0)
    {
      g_signal_handler_disconnect (self->permission, self->permission_allowed_id);
      self->permission_allowed_id = 0;
    }
  g_clear_object (&self->permission);

  g_clear_pointer (&self->blacklisted_apps, g_hash_table_unref);
  g_clear_pointer (&self->filter, mct_app_filter_unref);
  g_clear_object (&self->manager);
  g_clear_object (&self->app_info_monitor);

  G_OBJECT_CLASS (cc_app_permissions_parent_class)->finalize (object);
}


static void
cc_app_permissions_dispose (GObject *object)
{
  CcAppPermissions *self = (CcAppPermissions *)object;

  flush_update_blacklisted_apps (self);

  G_OBJECT_CLASS (cc_app_permissions_parent_class)->dispose (object);
}

static void
cc_app_permissions_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  CcAppPermissions *self = CC_APP_PERMISSIONS (object);

  switch (prop_id)
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
cc_app_permissions_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  CcAppPermissions *self = CC_APP_PERMISSIONS (object);

  switch (prop_id)
    {
    case PROP_USER:
      cc_app_permissions_set_user (self, g_value_get_object (value));
      break;

    case PROP_PERMISSION:
      cc_app_permissions_set_permission (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
cc_app_permissions_class_init (CcAppPermissionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = cc_app_permissions_finalize;
  object_class->dispose = cc_app_permissions_dispose;
  object_class->get_property = cc_app_permissions_get_property;
  object_class->set_property = cc_app_permissions_set_property;

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

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/control-center/user-accounts/cc-app-permissions.ui");

  gtk_widget_class_bind_template_child (widget_class, CcAppPermissions, age_menu);
  gtk_widget_class_bind_template_child (widget_class, CcAppPermissions, allow_system_installation_switch);
  gtk_widget_class_bind_template_child (widget_class, CcAppPermissions, allow_user_installation_switch);
  gtk_widget_class_bind_template_child (widget_class, CcAppPermissions, allow_web_browsers_switch);
  gtk_widget_class_bind_template_child (widget_class, CcAppPermissions, restriction_button);
  gtk_widget_class_bind_template_child (widget_class, CcAppPermissions, restriction_popover);
  gtk_widget_class_bind_template_child (widget_class, CcAppPermissions, listbox);

  gtk_widget_class_bind_template_callback (widget_class, on_allow_installation_switch_active_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_allow_web_browsers_switch_active_changed_cb);
}

static void
cc_app_permissions_init (CcAppPermissions *self)
{
  g_autoptr(GDBusConnection) system_bus = NULL;
  g_autoptr(GError) error = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  self->selected_age = (guint) -1;
  self->system_installation = flatpak_installation_new_system (NULL, NULL);
  self->user_installation = flatpak_installation_new_user (NULL, NULL);

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
  self->blacklisted_apps = g_hash_table_new_full (g_direct_hash, g_direct_equal, g_object_unref, NULL);

  self->apps = g_list_store_new (G_TYPE_APP_INFO);

  self->app_info_monitor = g_app_info_monitor_get ();
  g_signal_connect_object (self->app_info_monitor, "changed",
                           (GCallback) app_info_changed_cb, self, 0);

  gtk_list_box_bind_model (self->listbox,
                           G_LIST_MODEL (self->apps),
                           create_row_for_app_cb,
                           self,
                           NULL);

  g_object_bind_property (self->allow_user_installation_switch, "active",
                          self->allow_system_installation_switch, "sensitive",
                          G_BINDING_DEFAULT);
}

ActUser*
cc_app_permissions_get_user (CcAppPermissions *self)
{
  g_return_val_if_fail (CC_IS_APP_PERMISSIONS (self), NULL);

  return self->user;
}

void
cc_app_permissions_set_user (CcAppPermissions *self,
                             ActUser          *user)
{
  g_return_if_fail (CC_IS_APP_PERMISSIONS (self));
  g_return_if_fail (ACT_IS_USER (user));

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
  CcAppPermissions *self = CC_APP_PERMISSIONS (user_data);

  setup_parental_control_settings (self);
}

GPermission *  /* (nullable) */
cc_app_permissions_get_permission (CcAppPermissions *self)
{
  g_return_val_if_fail (CC_IS_APP_PERMISSIONS (self), NULL);

  return self->permission;
}

void
cc_app_permissions_set_permission (CcAppPermissions *self,
                                   GPermission      *permission  /* (nullable) */)
{
  g_return_if_fail (CC_IS_APP_PERMISSIONS (self));
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
  setup_parental_control_settings (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PERMISSION]);
}
