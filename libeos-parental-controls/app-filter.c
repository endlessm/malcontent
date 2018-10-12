/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 *
 * Copyright © 2018 Endless Mobile, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors:
 *  - Philip Withnall <withnall@endlessm.com>
 */

#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include <libeos-parental-controls/app-filter.h>


G_DEFINE_QUARK (EpcAppFilterError, epc_app_filter_error)

/**
 * EpcAppFilterListType:
 * @EPC_APP_FILTER_LIST_BLACKLIST: Any program in the list is not allowed to
 *    be run.
 * @EPC_APP_FILTER_LIST_WHITELIST: Any program not in the list is not allowed
 *    to be run.
 *
 * Different semantics for interpreting an application list.
 *
 * Since: 0.1.0
 */
typedef enum
{
  EPC_APP_FILTER_LIST_BLACKLIST,
  EPC_APP_FILTER_LIST_WHITELIST,
} EpcAppFilterListType;

struct _EpcAppFilter
{
  gint ref_count;

  uid_t user_id;

  gchar **app_list;  /* (owned) (array zero-terminated=1) */
  EpcAppFilterListType app_list_type;

  GVariant *oars_ratings;  /* (type a{ss}) (owned) */
};

G_DEFINE_BOXED_TYPE (EpcAppFilter, epc_app_filter,
                     epc_app_filter_ref, epc_app_filter_unref)

/**
 * epc_app_filter_ref:
 * @filter: (transfer none): an #EpcAppFilter
 *
 * Increment the reference count of @filter, and return the same pointer to it.
 *
 * Returns: (transfer full): the same pointer as @filter
 * Since: 0.1.0
 */
EpcAppFilter *
epc_app_filter_ref (EpcAppFilter *filter)
{
  g_return_val_if_fail (filter != NULL, NULL);
  g_return_val_if_fail (filter->ref_count >= 1, NULL);
  g_return_val_if_fail (filter->ref_count <= G_MAXINT - 1, NULL);

  filter->ref_count++;
  return filter;
}

/**
 * epc_app_filter_unref:
 * @filter: (transfer full): an #EpcAppFilter
 *
 * Decrement the reference count of @filter. If the reference count reaches
 * zero, free the @filter and all its resources.
 *
 * Since: 0.1.0
 */
void
epc_app_filter_unref (EpcAppFilter *filter)
{
  g_return_if_fail (filter != NULL);
  g_return_if_fail (filter->ref_count >= 1);

  filter->ref_count--;

  if (filter->ref_count <= 0)
    {
      g_strfreev (filter->app_list);
      g_variant_unref (filter->oars_ratings);
      g_free (filter);
    }
}

/**
 * epc_app_filter_get_user_id:
 * @filter: an #EpcAppFilter
 *
 * Get the user ID of the user this #EpcAppFilter is for.
 *
 * Returns: user ID of the relevant user
 * Since: 0.1.0
 */
uid_t
epc_app_filter_get_user_id (EpcAppFilter *filter)
{
  g_return_val_if_fail (filter != NULL, FALSE);
  g_return_val_if_fail (filter->ref_count >= 1, FALSE);

  return filter->user_id;
}

/**
 * epc_app_filter_is_path_allowed:
 * @filter: an #EpcAppFilter
 * @path: absolute path of a program to check
 *
 * Check whether the program at @path is allowed to be run according to this
 * app filter. @path will be canonicalised without doing any I/O.
 *
 * Returns: %TRUE if the user this @filter corresponds to is allowed to run the
 *    program at @path according to the @filter policy; %FALSE otherwise
 * Since: 0.1.0
 */
gboolean
epc_app_filter_is_path_allowed (EpcAppFilter *filter,
                                const gchar  *path)
{
  g_return_val_if_fail (filter != NULL, FALSE);
  g_return_val_if_fail (filter->ref_count >= 1, FALSE);
  g_return_val_if_fail (path != NULL, FALSE);
  g_return_val_if_fail (g_path_is_absolute (path), FALSE);

  g_autofree gchar *canonical_path = g_canonicalize_filename (path, "/");
  gboolean path_in_list = g_strv_contains ((const gchar * const *) filter->app_list,
                                           canonical_path);

  switch (filter->app_list_type)
    {
    case EPC_APP_FILTER_LIST_BLACKLIST:
      return !path_in_list;
    case EPC_APP_FILTER_LIST_WHITELIST:
      return path_in_list;
    default:
      g_assert_not_reached ();
    }
}

/**
 * epc_app_filter_get_oars_value:
 * @filter: an #EpcAppFilter
 * @oars_section: name of the OARS section to get the value from
 *
 * Get the value assigned to the given @oars_section in the OARS filter stored
 * within @filter. If that section has no value explicitly defined,
 * %EPC_APP_FILTER_OARS_VALUE_UNKNOWN is returned.
 *
 * This value is the most intense value allowed for apps to have in this
 * section, inclusive. Any app with a more intense value for this section must
 * be hidden from the user whose @filter this is.
 *
 * Returns: an #EpcAppFilterOarsValue
 * Since: 0.1.0
 */
EpcAppFilterOarsValue
epc_app_filter_get_oars_value (EpcAppFilter *filter,
                               const gchar  *oars_section)
{
  const gchar *value_str;

  g_return_val_if_fail (filter != NULL, EPC_APP_FILTER_OARS_VALUE_UNKNOWN);
  g_return_val_if_fail (filter->ref_count >= 1,
                        EPC_APP_FILTER_OARS_VALUE_UNKNOWN);
  g_return_val_if_fail (oars_section != NULL && *oars_section != '\0',
                        EPC_APP_FILTER_OARS_VALUE_UNKNOWN);

  if (!g_variant_lookup (filter->oars_ratings, oars_section, "&s", &value_str))
    return EPC_APP_FILTER_OARS_VALUE_UNKNOWN;

  if (g_str_equal (value_str, "none"))
    return EPC_APP_FILTER_OARS_VALUE_NONE;
  else if (g_str_equal (value_str, "mild"))
    return EPC_APP_FILTER_OARS_VALUE_MILD;
  else if (g_str_equal (value_str, "moderate"))
    return EPC_APP_FILTER_OARS_VALUE_MODERATE;
  else if (g_str_equal (value_str, "intense"))
    return EPC_APP_FILTER_OARS_VALUE_INTENSE;
  else
    return EPC_APP_FILTER_OARS_VALUE_UNKNOWN;
}

/* Check if @error is a D-Bus remote error matching @expected_error_name. */
static gboolean
bus_remote_error_matches (const GError *error,
                          const gchar  *expected_error_name)
{
  g_autofree gchar *error_name = NULL;

  if (!g_dbus_error_is_remote_error (error))
    return FALSE;

  error_name = g_dbus_error_get_remote_error (error);

  return g_str_equal (error_name, expected_error_name);
}

/* Convert a #GDBusError into a #EpcAppFilter error. */
static GError *
bus_error_to_app_filter_error (const GError *bus_error,
                               uid_t         user_id)
{
  if (g_error_matches (bus_error, G_DBUS_ERROR, G_DBUS_ERROR_ACCESS_DENIED) ||
      bus_remote_error_matches (bus_error, "org.freedesktop.Accounts.Error.PermissionDenied"))
    return g_error_new (EPC_APP_FILTER_ERROR, EPC_APP_FILTER_ERROR_PERMISSION_DENIED,
                        _("Not allowed to query app filter data for user %u"),
                        user_id);
  else if (g_error_matches (bus_error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD))
    return g_error_new (EPC_APP_FILTER_ERROR, EPC_APP_FILTER_ERROR_INVALID_USER,
                        _("User %u does not exist"), user_id);
  else
    return g_error_copy (bus_error);
}

static void get_bus_cb        (GObject         *obj,
                               GAsyncResult    *result,
                               gpointer         user_data);
static void get_app_filter    (GDBusConnection *connection,
                               GTask           *task);
static void get_app_filter_cb (GObject         *obj,
                               GAsyncResult    *result,
                               gpointer         user_data);

typedef struct
{
  uid_t user_id;
  gboolean allow_interactive_authorization;
} GetAppFilterData;

static void
get_app_filter_data_free (GetAppFilterData *data)
{
  g_free (data);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GetAppFilterData, get_app_filter_data_free)

/**
 * epc_get_app_filter_async:
 * @connection: (nullable): a #GDBusConnection to the system bus, or %NULL to
 *    use the default
 * @user_id: ID of the user to query, typically coming from getuid()
 * @allow_interactive_authorization: %TRUE to allow interactive polkit
 *    authorization dialogues to be displayed during the call; %FALSE otherwise
 * @callback: a #GAsyncReadyCallback
 * @cancellable: (nullable): a #GCancellable, or %NULL
 * @user_data: user data to pass to @callback
 *
 * Asynchronously get a snapshot of the app filter settings for the given
 * @user_id.
 *
 * @connection should be a connection to the system bus, where accounts-service
 * runs. It’s provided mostly for testing purposes, or to allow an existing
 * connection to be re-used. Pass %NULL to use the default connection.
 *
 * On failure, an #EpcAppFilterError, a #GDBusError or a #GIOError will be
 * returned.
 *
 * Since: 0.1.0
 */
void
epc_get_app_filter_async  (GDBusConnection     *connection,
                           uid_t                user_id,
                           gboolean             allow_interactive_authorization,
                           GCancellable        *cancellable,
                           GAsyncReadyCallback  callback,
                           gpointer             user_data)
{
  g_autoptr(GDBusConnection) connection_owned = NULL;
  g_autoptr(GTask) task = NULL;
  g_autoptr(GetAppFilterData) data = NULL;

  g_return_if_fail (connection == NULL || G_IS_DBUS_CONNECTION (connection));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (NULL, cancellable, callback, user_data);
  g_task_set_source_tag (task, epc_get_app_filter_async);

  data = g_new0 (GetAppFilterData, 1);
  data->user_id = user_id;
  data->allow_interactive_authorization = allow_interactive_authorization;
  g_task_set_task_data (task, g_steal_pointer (&data),
                        (GDestroyNotify) get_app_filter_data_free);

  if (connection == NULL)
    g_bus_get (G_BUS_TYPE_SYSTEM, cancellable,
               get_bus_cb, g_steal_pointer (&task));
  else
    get_app_filter (connection, g_steal_pointer (&task));
}

static void
get_bus_cb (GObject      *obj,
            GAsyncResult *result,
            gpointer      user_data)
{
  g_autoptr(GTask) task = G_TASK (user_data);
  g_autoptr(GDBusConnection) connection = NULL;
  g_autoptr(GError) local_error = NULL;

  connection = g_bus_get_finish (result, &local_error);

  if (local_error != NULL)
    g_task_return_error (task, g_steal_pointer (&local_error));
  else
    get_app_filter (connection, g_steal_pointer (&task));
}

static void
get_app_filter (GDBusConnection *connection,
                GTask           *task)
{
  g_autofree gchar *object_path = NULL;
  GCancellable *cancellable;

  GetAppFilterData *data = g_task_get_task_data (task);
  cancellable = g_task_get_cancellable (task);
  object_path = g_strdup_printf ("/org/freedesktop/Accounts/User%u",
                                 data->user_id);
  g_dbus_connection_call (connection,
                          "org.freedesktop.Accounts",
                          object_path,
                          "org.freedesktop.DBus.Properties",
                          "GetAll",
                          g_variant_new ("(s)", "com.endlessm.ParentalControls.AppFilter"),
                          G_VARIANT_TYPE ("(a{sv})"),
                          data->allow_interactive_authorization
                            ? G_DBUS_CALL_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION
                            : G_DBUS_CALL_FLAGS_NONE,
                          -1,  /* timeout, ms */
                          cancellable,
                          get_app_filter_cb,
                          g_steal_pointer (&task));
}

static void
get_app_filter_cb (GObject      *obj,
                   GAsyncResult *result,
                   gpointer      user_data)
{
  GDBusConnection *connection = G_DBUS_CONNECTION (obj);
  g_autoptr(GTask) task = G_TASK (user_data);
  g_autoptr(GVariant) result_variant = NULL;
  g_autoptr(GVariant) properties = NULL;
  g_autoptr(GError) local_error = NULL;
  g_autoptr(EpcAppFilter) app_filter = NULL;
  gboolean is_whitelist;
  g_auto(GStrv) app_list = NULL;
  const gchar *content_rating_kind;
  g_autoptr(GVariant) oars_variant = NULL;
  g_autoptr(GHashTable) oars_map = NULL;

  GetAppFilterData *data = g_task_get_task_data (task);
  result_variant = g_dbus_connection_call_finish (connection, result, &local_error);

  if (local_error != NULL)
    {
      g_autoptr(GError) app_filter_error = bus_error_to_app_filter_error (local_error,
                                                                          data->user_id);
      g_task_return_error (task, g_steal_pointer (&app_filter_error));
      return;
    }

  /* Extract the properties we care about. They may be silently omitted from the
   * results if we don’t have permission to access them. */
  properties = g_variant_get_child_value (result_variant, 0);
  if (!g_variant_lookup (properties, "app-filter", "(b^as)",
                         &is_whitelist, &app_list))
    {
      g_task_return_new_error (task, EPC_APP_FILTER_ERROR,
                               EPC_APP_FILTER_ERROR_PERMISSION_DENIED,
                               _("Not allowed to query app filter data for user %u"),
                               data->user_id);
      return;
    }

  if (!g_variant_lookup (properties, "oars-filter", "(&s@a{ss})",
                         &content_rating_kind, &oars_variant))
    {
      /* Default value. */
      content_rating_kind = "oars-1.1";
      oars_variant = g_variant_new ("@a{ss} {}");
    }

  /* Check that the OARS filter is in a format we support. Currently, that’s
   * only oars-1.0 and oars-1.1. */
  if (!g_str_equal (content_rating_kind, "oars-1.0") &&
      !g_str_equal (content_rating_kind, "oars-1.1"))
    {
      g_task_return_new_error (task, EPC_APP_FILTER_ERROR,
                               EPC_APP_FILTER_ERROR_INVALID_DATA,
                               _("OARS filter for user %u has an unrecognized kind ‘%s’"),
                               data->user_id, content_rating_kind);
      return;
    }

  /* Success. Create an #EpcAppFilter object to contain the results. */
  app_filter = g_new0 (EpcAppFilter, 1);
  app_filter->ref_count = 1;
  app_filter->user_id = data->user_id;
  app_filter->app_list = g_steal_pointer (&app_list);
  app_filter->app_list_type =
    is_whitelist ? EPC_APP_FILTER_LIST_WHITELIST : EPC_APP_FILTER_LIST_BLACKLIST;
  app_filter->oars_ratings = g_steal_pointer (&oars_variant);

  g_task_return_pointer (task, g_steal_pointer (&app_filter),
                         (GDestroyNotify) epc_app_filter_unref);
}

/**
 * epc_get_app_filter_finish:
 * @result: a #GAsyncResult
 * @error: return location for a #GError, or %NULL
 *
 * Finish an asynchronous operation to get the app filter for a user, started
 * with epc_get_app_filter_async().
 *
 * Returns: (transfer full): app filter for the queried user
 * Since: 0.1.0
 */
EpcAppFilter *
epc_get_app_filter_finish (GAsyncResult  *result,
                           GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (result, NULL), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}
