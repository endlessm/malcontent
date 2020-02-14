/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 *
 * Copyright © 2019 Endless Mobile, Inc.
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
 *  - Philip Withnall <withnall@endlessm.com>
 */

#include "config.h"

#include <act/act.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <libmalcontent-ui/malcontent-ui.h>
#include <polkit/polkit.h>

#include "application.h"
#include "user-selector.h"


static void user_selector_notify_user_cb (GObject    *obj,
                                          GParamSpec *pspec,
                                          gpointer    user_data);
static void user_manager_notify_is_loaded_cb (GObject    *obj,
                                              GParamSpec *pspec,
                                              gpointer    user_data);
static void permission_new_cb (GObject      *source_object,
                               GAsyncResult *result,
                               gpointer      user_data);
static void permission_notify_allowed_cb (GObject    *obj,
                                          GParamSpec *pspec,
                                          gpointer    user_data);
static void user_accounts_panel_button_clicked_cb (GtkButton *button,
                                                   gpointer   user_data);


/**
 * MctApplication:
 *
 * #MctApplication is a top-level object representing the parental controls
 * application.
 *
 * Since: 0.5.0
 */
struct _MctApplication
{
  GtkApplication parent_instance;

  GCancellable *cancellable;  /* (owned) */

  ActUserManager *user_manager;  /* (owned) */

  GPermission *permission;  /* (owned) */
  GError *permission_error;  /* (nullable) (owned) */

  MctUserSelector *user_selector;
  MctUserControls *user_controls;
  GtkStack *main_stack;
  GtkLabel *error_title;
  GtkLabel *error_message;
  GtkLockButton *lock_button;
  GtkButton *user_accounts_panel_button;
};

G_DEFINE_TYPE (MctApplication, mct_application, GTK_TYPE_APPLICATION)

static void
mct_application_init (MctApplication *self)
{
  self->cancellable = g_cancellable_new ();
}

static void
mct_application_constructed (GObject *object)
{
  GApplication *application = G_APPLICATION (object);

  g_application_set_application_id (application, "org.freedesktop.MalcontentControl");

  /* Localisation */
  bindtextdomain ("malcontent", PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset ("malcontent", "UTF-8");
  textdomain ("malcontent");

  g_set_application_name (_("Parental Controls"));
  gtk_window_set_default_icon_name ("org.freedesktop.MalcontentControl");

  G_OBJECT_CLASS (mct_application_parent_class)->constructed (object);
}

static void
mct_application_dispose (GObject *object)
{
  MctApplication *self = MCT_APPLICATION (object);

  g_cancellable_cancel (self->cancellable);

  if (self->user_manager != NULL)
    {
      g_signal_handlers_disconnect_by_func (self->user_manager,
                                            user_manager_notify_is_loaded_cb, self);
      g_clear_object (&self->user_manager);
    }

  if (self->permission != NULL)
    {
      g_signal_handlers_disconnect_by_func (self->permission,
                                            permission_notify_allowed_cb, self);
      g_clear_object (&self->permission);
    }

  g_clear_error (&self->permission_error);
  g_clear_object (&self->cancellable);

  G_OBJECT_CLASS (mct_application_parent_class)->dispose (object);
}

static GtkWindow *
mct_application_get_main_window (MctApplication *self)
{
  return gtk_application_get_active_window (GTK_APPLICATION (self));
}

static void
mct_application_activate (GApplication *application)
{
  MctApplication *self = MCT_APPLICATION (application);
  GtkWindow *window = NULL;

  window = mct_application_get_main_window (self);

  if (window == NULL)
    {
      g_autoptr(GtkBuilder) builder = NULL;
      g_autoptr(GError) local_error = NULL;

      /* Ensure the types used in the UI are registered. */
      g_type_ensure (MCT_TYPE_USER_CONTROLS);
      g_type_ensure (MCT_TYPE_USER_SELECTOR);

      /* Start loading the permission */
      polkit_permission_new ("org.freedesktop.MalcontentControl.administration",
                             NULL, self->cancellable,
                             permission_new_cb, self);

      builder = gtk_builder_new ();

      g_assert (self->user_manager == NULL);
      self->user_manager = g_object_ref (act_user_manager_get_default ());

      gtk_builder_set_translation_domain (builder, "malcontent");
      gtk_builder_expose_object (builder, "user_manager", G_OBJECT (self->user_manager));

      gtk_builder_add_from_resource (builder, "/org/freedesktop/MalcontentControl/ui/main.ui", &local_error);
      g_assert (local_error == NULL);

      /* Set up the main window. */
      window = GTK_WINDOW (gtk_builder_get_object (builder, "main_window"));
      gtk_window_set_application (window, GTK_APPLICATION (application));

      self->main_stack = GTK_STACK (gtk_builder_get_object (builder, "main_stack"));
      self->user_selector = MCT_USER_SELECTOR (gtk_builder_get_object (builder, "user_selector"));
      self->user_controls = MCT_USER_CONTROLS (gtk_builder_get_object (builder, "user_controls"));
      self->error_title = GTK_LABEL (gtk_builder_get_object (builder, "error_title"));
      self->error_message = GTK_LABEL (gtk_builder_get_object (builder, "error_message"));
      self->lock_button = GTK_LOCK_BUTTON (gtk_builder_get_object (builder, "lock_button"));
      self->user_accounts_panel_button = GTK_BUTTON (gtk_builder_get_object (builder, "user_accounts_panel_button"));

      /* Connect signals. */
      g_signal_connect_object (self->user_selector, "notify::user",
                               G_CALLBACK (user_selector_notify_user_cb),
                               self, 0  /* flags */);
      g_signal_connect_object (self->user_accounts_panel_button, "clicked",
                               G_CALLBACK (user_accounts_panel_button_clicked_cb),
                               self, 0  /* flags */);
      g_signal_connect (self->user_manager, "notify::is-loaded",
                        G_CALLBACK (user_manager_notify_is_loaded_cb), self);

      /* Work out whether to show the loading page or the main page, and show
       * the controls for the initially selected user. */
      user_selector_notify_user_cb (G_OBJECT (self->user_selector), NULL, self);
      user_manager_notify_is_loaded_cb (G_OBJECT (self->user_manager), NULL, self);

      gtk_widget_show (GTK_WIDGET (window));
    }

  /* Bring the window to the front. */
  gtk_window_present (window);
}

static void
mct_application_class_init (MctApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

  object_class->constructed = mct_application_constructed;
  object_class->dispose = mct_application_dispose;

  application_class->activate = mct_application_activate;
}

static void
update_main_stack (MctApplication *self)
{
  gboolean is_user_manager_loaded, is_permission_loaded, has_permission;
  const gchar *new_page_name, *old_page_name;
  GtkWidget *new_focus_widget;
  ActUser *selected_user;

  /* The implementation of #ActUserManager guarantees that once is-loaded is
   * true, it is never reset to false. */
  g_object_get (self->user_manager, "is-loaded", &is_user_manager_loaded, NULL);
  is_permission_loaded = (self->permission != NULL || self->permission_error != NULL);
  has_permission = (self->permission != NULL && g_permission_get_allowed (self->permission));
  selected_user = mct_user_selector_get_user (self->user_selector);

  /* Handle any loading errors (including those from getting the permission). */
  if ((is_user_manager_loaded && act_user_manager_no_service (self->user_manager)) ||
      self->permission_error != NULL)
    {
      gtk_label_set_label (self->error_title,
                           _("Failed to load user data from the system"));
      gtk_label_set_label (self->error_message,
                           _("Please make sure that the AccountsService is installed and enabled."));

      new_page_name = "error";
      new_focus_widget = NULL;
    }
  else if (is_user_manager_loaded && selected_user == NULL)
    {
      new_page_name = "no-other-users";
      new_focus_widget = GTK_WIDGET (self->user_accounts_panel_button);
    }
  else if (is_permission_loaded && !has_permission)
    {
      gtk_lock_button_set_permission (self->lock_button, self->permission);
      mct_user_controls_set_permission (self->user_controls, self->permission);

      new_page_name = "unlock";
      new_focus_widget = GTK_WIDGET (self->lock_button);
    }
  else if (is_permission_loaded && is_user_manager_loaded)
    {
      mct_user_controls_set_user (self->user_controls, selected_user);

      new_page_name = "controls";
      new_focus_widget = GTK_WIDGET (self->user_selector);
    }
  else
    {
      new_page_name = "loading";
      new_focus_widget = NULL;
    }

  old_page_name = gtk_stack_get_visible_child_name (self->main_stack);
  gtk_stack_set_visible_child_name (self->main_stack, new_page_name);

  if (new_focus_widget != NULL && !g_str_equal (old_page_name, new_page_name))
    gtk_widget_grab_focus (new_focus_widget);
}

static void
user_selector_notify_user_cb (GObject    *obj,
                              GParamSpec *pspec,
                              gpointer    user_data)
{
  MctApplication *self = MCT_APPLICATION (user_data);

  update_main_stack (self);
}

static void
user_manager_notify_is_loaded_cb (GObject    *obj,
                                  GParamSpec *pspec,
                                  gpointer    user_data)
{
  MctApplication *self = MCT_APPLICATION (user_data);

  update_main_stack (self);
}

static void
permission_new_cb (GObject      *source_object,
                   GAsyncResult *result,
                   gpointer      user_data)
{
  MctApplication *self = MCT_APPLICATION (user_data);
  g_autoptr(GPermission) permission = NULL;
  g_autoptr(GError) local_error = NULL;

  permission = polkit_permission_new_finish (result, &local_error);
  if (permission == NULL)
    {
      g_assert (self->permission_error == NULL);
      self->permission_error = g_steal_pointer (&local_error);
      g_debug ("Error getting permission: %s", self->permission_error->message);
    }
  else
    {
      g_assert (self->permission == NULL);
      self->permission = g_steal_pointer (&permission);

      g_signal_connect (self->permission, "notify::allowed",
                        G_CALLBACK (permission_notify_allowed_cb), self);
    }

  /* Recalculate the UI. */
  update_main_stack (self);
}

static void
permission_notify_allowed_cb (GObject    *obj,
                              GParamSpec *pspec,
                              gpointer    user_data)
{
  MctApplication *self = MCT_APPLICATION (user_data);

  update_main_stack (self);
}

static void
user_accounts_panel_button_clicked_cb (GtkButton *button,
                                       gpointer   user_data)
{
  g_autoptr(GError) local_error = NULL;

  if (!g_spawn_command_line_async ("gnome-control-center user-accounts", &local_error))
    {
      g_warning ("Error opening GNOME Control Center: %s",
                 local_error->message);
      return;
    }
}

/**
 * mct_application_new:
 *
 * Create a new #MctApplication.
 *
 * Returns: (transfer full): a new #MctApplication
 * Since: 0.5.0
 */
MctApplication *
mct_application_new (void)
{
  return g_object_new (MCT_TYPE_APPLICATION, NULL);
}