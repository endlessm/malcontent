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

  GVariant *oars_ratings;  /* (type a{ss}) (owned non-floating) */
  gboolean allow_app_installation;
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
 * @path: (type filename): absolute path of a program to check
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
 * epc_app_filter_is_flatpak_ref_allowed:
 * @filter: an #EpcAppFilter
 * @app_ref: flatpak ref for the app
 *
 * Check whether the flatpak app with the given @app_ref is allowed to be run
 * according to this app filter.
 *
 * Returns: %TRUE if the user this @filter corresponds to is allowed to run the
 *    flatpak called @app_ref according to the @filter policy; %FALSE otherwise
 * Since: 0.1.0
 */
gboolean
epc_app_filter_is_flatpak_ref_allowed (EpcAppFilter *filter,
                                       const gchar  *app_ref)
{
  g_return_val_if_fail (filter != NULL, FALSE);
  g_return_val_if_fail (filter->ref_count >= 1, FALSE);
  g_return_val_if_fail (app_ref != NULL, FALSE);

  gboolean ref_in_list = g_strv_contains ((const gchar * const *) filter->app_list,
                                          app_ref);

  switch (filter->app_list_type)
    {
    case EPC_APP_FILTER_LIST_BLACKLIST:
      return !ref_in_list;
    case EPC_APP_FILTER_LIST_WHITELIST:
      return ref_in_list;
    default:
      g_assert_not_reached ();
    }
}

static gint
strcmp_cb (gconstpointer a,
           gconstpointer b)
{
  const gchar *str_a = *((const gchar * const *) a);
  const gchar *str_b = *((const gchar * const *) b);

  return g_strcmp0 (str_a, str_b);
}

/**
 * epc_app_filter_get_oars_sections:
 * @filter: an #EpcAppFilter
 *
 * List the OARS sections present in this app filter. The sections are returned
 * in lexicographic order. A section will be listed even if its stored value is
 * %EPC_APP_FILTER_OARS_VALUE_UNKNOWN. The returned list may be empty.
 *
 * Returns: (transfer container) (array zero-terminated=1): %NULL-terminated
 *    array of OARS sections
 * Since: 0.1.0
 */
const gchar **
epc_app_filter_get_oars_sections (EpcAppFilter *filter)
{
  g_autoptr(GPtrArray) sections = g_ptr_array_new_with_free_func (NULL);
  GVariantIter iter;
  const gchar *oars_section;

  g_return_val_if_fail (filter != NULL, NULL);
  g_return_val_if_fail (filter->ref_count >= 1, NULL);

  g_variant_iter_init (&iter, filter->oars_ratings);

  while (g_variant_iter_loop (&iter, "{&s&s}", &oars_section, NULL))
    g_ptr_array_add (sections, (gpointer) oars_section);

  /* Sort alphabetically for easier comparisons later. */
  g_ptr_array_sort (sections, strcmp_cb);

  g_ptr_array_add (sections, NULL);  /* NULL terminator */

  return (const gchar **) g_ptr_array_free (g_steal_pointer (&sections), FALSE);
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
 * This does not factor in epc_app_filter_is_app_installation_allowed().
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

/**
 * epc_app_filter_is_app_installation_allowed:
 * @filter: an #EpcAppFilter
 *
 * Get whether app installation is allowed at all for the user. This should be
 * queried in addition to the OARS values (epc_app_filter_get_oars_value()) — if
 * it returns %FALSE, the OARS values should be ignored and app installation
 * should be unconditionally disallowed.
 *
 * Returns: %TRUE if app installation is allowed in general for this user;
 *    %FALSE if it is unconditionally disallowed for this user
 * Since: 0.1.0
 */
gboolean
epc_app_filter_is_app_installation_allowed (EpcAppFilter *filter)
{
  g_return_val_if_fail (filter != NULL, FALSE);
  g_return_val_if_fail (filter->ref_count >= 1, FALSE);

  return filter->allow_app_installation;
}

/**
 * _epc_app_filter_build_app_filter_variant:
 * @filter: an #EpcAppFilter
 *
 * Build a #GVariant which contains the app filter from @filter, in the format
 * used for storing it in AccountsService.
 *
 * Returns: (transfer floating): a new, floating #GVariant containing the app
 *    filter
 * Since: 0.1.0
 */
static GVariant *
_epc_app_filter_build_app_filter_variant (EpcAppFilter *filter)
{
  g_auto(GVariantBuilder) builder = G_VARIANT_BUILDER_INIT (G_VARIANT_TYPE ("(bas)"));

  g_return_val_if_fail (filter != NULL, NULL);
  g_return_val_if_fail (filter->ref_count >= 1, NULL);

  g_variant_builder_add (&builder, "b",
                         (filter->app_list_type == EPC_APP_FILTER_LIST_WHITELIST));
  g_variant_builder_open (&builder, G_VARIANT_TYPE ("as"));

  for (gsize i = 0; filter->app_list[i] != NULL; i++)
    g_variant_builder_add (&builder, "s", filter->app_list[i]);

  g_variant_builder_close (&builder);

  return g_variant_builder_end (&builder);
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

/* Find the object path for the given @user_id on the accountsservice D-Bus
 * interface, by calling its FindUserById() method. This is a synchronous,
 * blocking function. */
static gchar *
accounts_find_user_by_id (GDBusConnection  *connection,
                          uid_t             user_id,
                          gboolean          allow_interactive_authorization,
                          GCancellable     *cancellable,
                          GError          **error)
{
  g_autofree gchar *object_path = NULL;
  g_autoptr(GVariant) result_variant = NULL;
  g_autoptr(GError) local_error = NULL;

  result_variant =
      g_dbus_connection_call_sync (connection,
                                   "org.freedesktop.Accounts",
                                   "/org/freedesktop/Accounts",
                                   "org.freedesktop.Accounts",
                                   "FindUserById",
                                   g_variant_new ("(x)", user_id),
                                   G_VARIANT_TYPE ("(o)"),
                                   allow_interactive_authorization
                                     ? G_DBUS_CALL_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION
                                     : G_DBUS_CALL_FLAGS_NONE,
                                   -1,  /* timeout, ms */
                                   cancellable,
                                   &local_error);
  if (local_error != NULL)
    {
      g_autoptr(GError) app_filter_error = bus_error_to_app_filter_error (local_error,
                                                                          user_id);
      g_propagate_error (error, g_steal_pointer (&app_filter_error));
      return NULL;
    }

  g_variant_get (result_variant, "(o)", &object_path);

  return g_steal_pointer (&object_path);
}

/**
 * epc_get_app_filter:
 * @connection: (nullable): a #GDBusConnection to the system bus, or %NULL to
 *    use the default
 * @user_id: ID of the user to query, typically coming from getuid()
 * @allow_interactive_authorization: %TRUE to allow interactive polkit
 *    authorization dialogues to be displayed during the call; %FALSE otherwise
 * @cancellable: (nullable): a #GCancellable, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * Synchronous version of epc_get_app_filter_async().
 *
 * Returns: (transfer full): app filter for the queried user
 * Since: 0.1.0
 */
EpcAppFilter *
epc_get_app_filter (GDBusConnection  *connection,
                    uid_t             user_id,
                    gboolean          allow_interactive_authorization,
                    GCancellable     *cancellable,
                    GError          **error)
{
  g_autofree gchar *object_path = NULL;
  g_autoptr(GVariant) result_variant = NULL;
  g_autoptr(GVariant) properties = NULL;
  g_autoptr(GError) local_error = NULL;
  g_autoptr(EpcAppFilter) app_filter = NULL;
  gboolean is_whitelist;
  g_auto(GStrv) app_list = NULL;
  const gchar *content_rating_kind;
  g_autoptr(GVariant) oars_variant = NULL;
  g_autoptr(GHashTable) oars_map = NULL;
  gboolean allow_app_installation;

  g_return_val_if_fail (connection == NULL || G_IS_DBUS_CONNECTION (connection), NULL);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (connection == NULL)
    connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, cancellable, error);
  if (connection == NULL)
    return NULL;

  object_path = accounts_find_user_by_id (connection, user_id,
                                          allow_interactive_authorization,
                                          cancellable, error);
  if (object_path == NULL)
    return NULL;

  result_variant =
      g_dbus_connection_call_sync (connection,
                                   "org.freedesktop.Accounts",
                                   object_path,
                                   "org.freedesktop.DBus.Properties",
                                   "GetAll",
                                   g_variant_new ("(s)", "com.endlessm.ParentalControls.AppFilter"),
                                   G_VARIANT_TYPE ("(a{sv})"),
                                   allow_interactive_authorization
                                     ? G_DBUS_CALL_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION
                                     : G_DBUS_CALL_FLAGS_NONE,
                                   -1,  /* timeout, ms */
                                   cancellable,
                                   &local_error);
  if (local_error != NULL)
    {
      g_autoptr(GError) app_filter_error = bus_error_to_app_filter_error (local_error,
                                                                          user_id);
      g_propagate_error (error, g_steal_pointer (&app_filter_error));
      return NULL;
    }

  /* Extract the properties we care about. They may be silently omitted from the
   * results if we don’t have permission to access them. */
  properties = g_variant_get_child_value (result_variant, 0);
  if (!g_variant_lookup (properties, "app-filter", "(b^as)",
                         &is_whitelist, &app_list))
    {
      g_set_error (error, EPC_APP_FILTER_ERROR,
                   EPC_APP_FILTER_ERROR_PERMISSION_DENIED,
                   _("Not allowed to query app filter data for user %u"),
                   user_id);
      return NULL;
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
      g_set_error (error, EPC_APP_FILTER_ERROR,
                   EPC_APP_FILTER_ERROR_INVALID_DATA,
                   _("OARS filter for user %u has an unrecognized kind ‘%s’"),
                   user_id, content_rating_kind);
      return NULL;
    }

  if (!g_variant_lookup (properties, "allow-app-installation", "b",
                         &allow_app_installation))
    {
      /* Default value. */
      allow_app_installation = TRUE;
    }

  /* Success. Create an #EpcAppFilter object to contain the results. */
  app_filter = g_new0 (EpcAppFilter, 1);
  app_filter->ref_count = 1;
  app_filter->user_id = user_id;
  app_filter->app_list = g_steal_pointer (&app_list);
  app_filter->app_list_type =
    is_whitelist ? EPC_APP_FILTER_LIST_WHITELIST : EPC_APP_FILTER_LIST_BLACKLIST;
  app_filter->oars_ratings = g_steal_pointer (&oars_variant);
  app_filter->allow_app_installation = allow_app_installation;

  return g_steal_pointer (&app_filter);
}

static void get_app_filter_thread_cb (GTask        *task,
                                      gpointer      source_object,
                                      gpointer      task_data,
                                      GCancellable *cancellable);

typedef struct
{
  GDBusConnection *connection;  /* (nullable) (owned) */
  uid_t user_id;
  gboolean allow_interactive_authorization;
} GetAppFilterData;

static void
get_app_filter_data_free (GetAppFilterData *data)
{
  g_clear_object (&data->connection);
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
 * @cancellable: (nullable): a #GCancellable, or %NULL
 * @callback: a #GAsyncReadyCallback
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
  g_autoptr(GTask) task = NULL;
  g_autoptr(GetAppFilterData) data = NULL;

  g_return_if_fail (connection == NULL || G_IS_DBUS_CONNECTION (connection));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (NULL, cancellable, callback, user_data);
  g_task_set_source_tag (task, epc_get_app_filter_async);

  data = g_new0 (GetAppFilterData, 1);
  data->connection = (connection != NULL) ? g_object_ref (connection) : NULL;
  data->user_id = user_id;
  data->allow_interactive_authorization = allow_interactive_authorization;
  g_task_set_task_data (task, g_steal_pointer (&data),
                        (GDestroyNotify) get_app_filter_data_free);

  g_task_run_in_thread (task, get_app_filter_thread_cb);
}

static void
get_app_filter_thread_cb (GTask        *task,
                          gpointer      source_object,
                          gpointer      task_data,
                          GCancellable *cancellable)
{
  g_autoptr(EpcAppFilter) filter = NULL;
  GetAppFilterData *data = task_data;
  g_autoptr(GError) local_error = NULL;

  filter = epc_get_app_filter (data->connection, data->user_id,
                               data->allow_interactive_authorization,
                               cancellable, &local_error);

  if (local_error != NULL)
    g_task_return_error (task, g_steal_pointer (&local_error));
  else
    g_task_return_pointer (task, g_steal_pointer (&filter),
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

/**
 * epc_set_app_filter:
 * @connection: (nullable): a #GDBusConnection to the system bus, or %NULL to
 *    use the default
 * @user_id: ID of the user to set the filter for, typically coming from getuid()
 * @app_filter: (transfer none): the app filter to set for the user
 * @allow_interactive_authorization: %TRUE to allow interactive polkit
 *    authorization dialogues to be displayed during the call; %FALSE otherwise
 * @cancellable: (nullable): a #GCancellable, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * Synchronous version of epc_set_app_filter_async().
 *
 * Returns: %TRUE on success, %FALSE otherwise
 * Since: 0.1.0
 */
gboolean
epc_set_app_filter (GDBusConnection  *connection,
                    uid_t             user_id,
                    EpcAppFilter     *app_filter,
                    gboolean          allow_interactive_authorization,
                    GCancellable     *cancellable,
                    GError          **error)
{
  g_autofree gchar *object_path = NULL;
  g_autoptr(GVariant) app_filter_variant = NULL;
  g_autoptr(GVariant) oars_filter_variant = NULL;
  g_autoptr(GVariant) allow_app_installation_variant = NULL;
  g_autoptr(GVariant) app_filter_result_variant = NULL;
  g_autoptr(GVariant) oars_filter_result_variant = NULL;
  g_autoptr(GVariant) allow_app_installation_result_variant = NULL;
  g_autoptr(GError) local_error = NULL;

  g_return_val_if_fail (connection == NULL || G_IS_DBUS_CONNECTION (connection), FALSE);
  g_return_val_if_fail (app_filter != NULL, FALSE);
  g_return_val_if_fail (app_filter->ref_count >= 1, FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (connection == NULL)
    connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, cancellable, error);
  if (connection == NULL)
    return FALSE;

  object_path = accounts_find_user_by_id (connection, user_id,
                                          allow_interactive_authorization,
                                          cancellable, error);
  if (object_path == NULL)
    return FALSE;

  app_filter_variant = _epc_app_filter_build_app_filter_variant (app_filter);
  oars_filter_variant = g_variant_new ("(s@a{ss})", "oars-1.1",
                                       app_filter->oars_ratings);
  allow_app_installation_variant = g_variant_new_boolean (app_filter->allow_app_installation);

  app_filter_result_variant =
      g_dbus_connection_call_sync (connection,
                                   "org.freedesktop.Accounts",
                                   object_path,
                                   "org.freedesktop.DBus.Properties",
                                   "Set",
                                   g_variant_new ("(ssv)",
                                                  "com.endlessm.ParentalControls.AppFilter",
                                                  "app-filter",
                                                  g_steal_pointer (&app_filter_variant)),
                                   G_VARIANT_TYPE ("()"),
                                   allow_interactive_authorization
                                     ? G_DBUS_CALL_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION
                                     : G_DBUS_CALL_FLAGS_NONE,
                                   -1,  /* timeout, ms */
                                   cancellable,
                                   &local_error);
  if (local_error != NULL)
    {
      g_propagate_error (error, bus_error_to_app_filter_error (local_error, user_id));
      return FALSE;
    }

  oars_filter_result_variant =
      g_dbus_connection_call_sync (connection,
                                   "org.freedesktop.Accounts",
                                   object_path,
                                   "org.freedesktop.DBus.Properties",
                                   "Set",
                                   g_variant_new ("(ssv)",
                                                  "com.endlessm.ParentalControls.AppFilter",
                                                  "oars-filter",
                                                  g_steal_pointer (&oars_filter_variant)),
                                   G_VARIANT_TYPE ("()"),
                                   allow_interactive_authorization
                                     ? G_DBUS_CALL_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION
                                     : G_DBUS_CALL_FLAGS_NONE,
                                   -1,  /* timeout, ms */
                                   cancellable,
                                   &local_error);
  if (local_error != NULL)
    {
      g_propagate_error (error, bus_error_to_app_filter_error (local_error, user_id));
      return FALSE;
    }

  allow_app_installation_result_variant =
      g_dbus_connection_call_sync (connection,
                                   "org.freedesktop.Accounts",
                                   object_path,
                                   "org.freedesktop.DBus.Properties",
                                   "Set",
                                   g_variant_new ("(ssv)",
                                                  "com.endlessm.ParentalControls.AppFilter",
                                                  "allow-app-installation",
                                                  g_steal_pointer (&allow_app_installation_variant)),
                                   G_VARIANT_TYPE ("()"),
                                   allow_interactive_authorization
                                     ? G_DBUS_CALL_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION
                                     : G_DBUS_CALL_FLAGS_NONE,
                                   -1,  /* timeout, ms */
                                   cancellable,
                                   &local_error);
  if (local_error != NULL)
    {
      g_propagate_error (error, bus_error_to_app_filter_error (local_error, user_id));
      return FALSE;
    }

  return TRUE;
}

static void set_app_filter_thread_cb (GTask        *task,
                                      gpointer      source_object,
                                      gpointer      task_data,
                                      GCancellable *cancellable);

typedef struct
{
  GDBusConnection *connection;  /* (nullable) (owned) */
  uid_t user_id;
  EpcAppFilter *app_filter;  /* (owned) */
  gboolean allow_interactive_authorization;
} SetAppFilterData;

static void
set_app_filter_data_free (SetAppFilterData *data)
{
  g_clear_object (&data->connection);
  epc_app_filter_unref (data->app_filter);
  g_free (data);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (SetAppFilterData, set_app_filter_data_free)

/**
 * epc_set_app_filter_async:
 * @connection: (nullable): a #GDBusConnection to the system bus, or %NULL to
 *    use the default
 * @user_id: ID of the user to set the filter for, typically coming from getuid()
 * @app_filter: (transfer none): the app filter to set for the user
 * @allow_interactive_authorization: %TRUE to allow interactive polkit
 *    authorization dialogues to be displayed during the call; %FALSE otherwise
 * @cancellable: (nullable): a #GCancellable, or %NULL
 * @callback: a #GAsyncReadyCallback
 * @user_data: user data to pass to @callback
 *
 * Asynchronously set the app filter settings for the given @user_id to the
 * given @app_filter instance. This will set all fields of the app filter.
 *
 * @connection should be a connection to the system bus, where accounts-service
 * runs. It’s provided mostly for testing purposes, or to allow an existing
 * connection to be re-used. Pass %NULL to use the default connection.
 *
 * On failure, an #EpcAppFilterError, a #GDBusError or a #GIOError will be
 * returned. The user’s app filter settings will be left in an undefined state.
 *
 * Since: 0.1.0
 */
void
epc_set_app_filter_async (GDBusConnection     *connection,
                          uid_t                user_id,
                          EpcAppFilter        *app_filter,
                          gboolean             allow_interactive_authorization,
                          GCancellable        *cancellable,
                          GAsyncReadyCallback  callback,
                          gpointer             user_data)
{
  g_autoptr(GTask) task = NULL;
  g_autoptr(SetAppFilterData) data = NULL;

  g_return_if_fail (connection == NULL || G_IS_DBUS_CONNECTION (connection));
  g_return_if_fail (app_filter != NULL);
  g_return_if_fail (app_filter->ref_count >= 1);
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (NULL, cancellable, callback, user_data);
  g_task_set_source_tag (task, epc_set_app_filter_async);

  data = g_new0 (SetAppFilterData, 1);
  data->user_id = user_id;
  data->app_filter = epc_app_filter_ref (app_filter);
  data->allow_interactive_authorization = allow_interactive_authorization;
  g_task_set_task_data (task, g_steal_pointer (&data),
                        (GDestroyNotify) set_app_filter_data_free);

  g_task_run_in_thread (task, set_app_filter_thread_cb);
}

static void
set_app_filter_thread_cb (GTask        *task,
                          gpointer      source_object,
                          gpointer      task_data,
                          GCancellable *cancellable)
{
  gboolean success;
  SetAppFilterData *data = task_data;
  g_autoptr(GError) local_error = NULL;

  success = epc_set_app_filter (data->connection, data->user_id,
                                data->app_filter,
                                data->allow_interactive_authorization,
                                cancellable, &local_error);

  if (local_error != NULL)
    g_task_return_error (task, g_steal_pointer (&local_error));
  else
    g_task_return_boolean (task, success);
}

/**
 * epc_set_app_filter_finish:
 * @result: a #GAsyncResult
 * @error: return location for a #GError, or %NULL
 *
 * Finish an asynchronous operation to set the app filter for a user, started
 * with epc_set_app_filter_async().
 *
 * Returns: %TRUE on success, %FALSE otherwise
 * Since: 0.1.0
 */
gboolean
epc_set_app_filter_finish (GAsyncResult  *result,
                           GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (result, NULL), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

/*
 * Actual implementation of #EpcAppFilterBuilder.
 *
 * All members are %NULL if un-initialised, cleared, or ended.
 */
typedef struct
{
  GPtrArray *paths_blacklist;  /* (nullable) (owned) (element-type filename) */
  GHashTable *oars;  /* (nullable) (owned) (element-type utf8 EpcAppFilterOarsValue) */
  gboolean allow_app_installation;

  /*< private >*/
  gpointer padding[2];
} EpcAppFilterBuilderReal;

G_STATIC_ASSERT (sizeof (EpcAppFilterBuilderReal) ==
                 sizeof (EpcAppFilterBuilder));
G_STATIC_ASSERT (__alignof__ (EpcAppFilterBuilderReal) ==
                 __alignof__ (EpcAppFilterBuilder));

G_DEFINE_BOXED_TYPE (EpcAppFilterBuilder, epc_app_filter_builder,
                     epc_app_filter_builder_copy, epc_app_filter_builder_free)

/**
 * epc_app_filter_builder_init:
 * @builder: an uninitialised #EpcAppFilterBuilder
 *
 * Initialise the given @builder so it can be used to construct a new
 * #EpcAppFilter. @builder must have been allocated on the stack, and must not
 * already be initialised.
 *
 * Construct the #EpcAppFilter by calling methods on @builder, followed by
 * epc_app_filter_builder_end(). To abort construction, use
 * epc_app_filter_builder_clear().
 *
 * Since: 0.1.0
 */
void
epc_app_filter_builder_init (EpcAppFilterBuilder *builder)
{
  EpcAppFilterBuilder local_builder = EPC_APP_FILTER_BUILDER_INIT ();
  EpcAppFilterBuilderReal *_builder = (EpcAppFilterBuilderReal *) builder;

  g_return_if_fail (_builder != NULL);
  g_return_if_fail (_builder->paths_blacklist == NULL);
  g_return_if_fail (_builder->oars == NULL);

  memcpy (builder, &local_builder, sizeof (local_builder));
}

/**
 * epc_app_filter_builder_clear:
 * @builder: an #EpcAppFilterBuilder
 *
 * Clear @builder, freeing any internal state in it. This will not free the
 * top-level storage for @builder itself, which is assumed to be allocated on
 * the stack.
 *
 * If called on an already-cleared #EpcAppFilterBuilder, this function is
 * idempotent.
 *
 * Since: 0.1.0
 */
void
epc_app_filter_builder_clear (EpcAppFilterBuilder *builder)
{
  EpcAppFilterBuilderReal *_builder = (EpcAppFilterBuilderReal *) builder;

  g_return_if_fail (_builder != NULL);

  g_clear_pointer (&_builder->paths_blacklist, g_ptr_array_unref);
  g_clear_pointer (&_builder->oars, g_hash_table_unref);
}

/**
 * epc_app_filter_builder_new:
 *
 * Construct a new #EpcAppFilterBuilder on the heap. This is intended for
 * language bindings. The returned builder must eventually be freed with
 * epc_app_filter_builder_free(), but can be cleared zero or more times with
 * epc_app_filter_builder_clear() first.
 *
 * Returns: (transfer full): a new heap-allocated #EpcAppFilterBuilder
 * Since: 0.1.0
 */
EpcAppFilterBuilder *
epc_app_filter_builder_new (void)
{
  g_autoptr(EpcAppFilterBuilder) builder = NULL;

  builder = g_new0 (EpcAppFilterBuilder, 1);
  epc_app_filter_builder_init (builder);

  return g_steal_pointer (&builder);
}

/**
 * epc_app_filter_builder_copy:
 * @builder: an #EpcAppFilterBuilder
 *
 * Copy the given @builder to a newly-allocated #EpcAppFilterBuilder on the
 * heap. This is safe to use with cleared, stack-allocated
 * #EpcAppFilterBuilders.
 *
 * Returns: (transfer full): a copy of @builder
 * Since: 0.1.0
 */
EpcAppFilterBuilder *
epc_app_filter_builder_copy (EpcAppFilterBuilder *builder)
{
  EpcAppFilterBuilderReal *_builder = (EpcAppFilterBuilderReal *) builder;
  g_autoptr(EpcAppFilterBuilder) copy = NULL;
  EpcAppFilterBuilderReal *_copy;

  g_return_val_if_fail (builder != NULL, NULL);

  copy = epc_app_filter_builder_new ();
  _copy = (EpcAppFilterBuilderReal *) copy;

  epc_app_filter_builder_clear (copy);
  if (_builder->paths_blacklist != NULL)
    _copy->paths_blacklist = g_ptr_array_ref (_builder->paths_blacklist);
  if (_builder->oars != NULL)
    _copy->oars = g_hash_table_ref (_builder->oars);
  _copy->allow_app_installation = _builder->allow_app_installation;

  return g_steal_pointer (&copy);
}

/**
 * epc_app_filter_builder_free:
 * @builder: a heap-allocated #EpcAppFilterBuilder
 *
 * Free an #EpcAppFilterBuilder originally allocated using
 * epc_app_filter_builder_new(). This must not be called on stack-allocated
 * builders initialised using epc_app_filter_builder_init().
 *
 * Since: 0.1.0
 */
void
epc_app_filter_builder_free (EpcAppFilterBuilder *builder)
{
  g_return_if_fail (builder != NULL);

  epc_app_filter_builder_clear (builder);
  g_free (builder);
}

/**
 * epc_app_filter_builder_end:
 * @builder: an initialised #EpcAppFilterBuilder
 *
 * Finish constructing an #EpcAppFilter with the given @builder, and return it.
 * The #EpcAppFilterBuilder will be cleared as if epc_app_filter_builder_clear()
 * had been called.
 *
 * Returns: (transfer full): a newly constructed #EpcAppFilter
 * Since: 0.1.0
 */
EpcAppFilter *
epc_app_filter_builder_end (EpcAppFilterBuilder *builder)
{
  EpcAppFilterBuilderReal *_builder = (EpcAppFilterBuilderReal *) builder;
  g_autoptr(EpcAppFilter) app_filter = NULL;
  g_auto(GVariantBuilder) oars_builder = G_VARIANT_BUILDER_INIT (G_VARIANT_TYPE ("a{ss}"));
  GHashTableIter iter;
  gpointer key, value;
  g_autoptr(GVariant) oars_variant = NULL;

  g_return_val_if_fail (_builder != NULL, NULL);
  g_return_val_if_fail (_builder->paths_blacklist != NULL, NULL);
  g_return_val_if_fail (_builder->oars != NULL, NULL);

  /* Ensure the paths list is %NULL-terminated. */
  g_ptr_array_add (_builder->paths_blacklist, NULL);

  /* Build the OARS variant. */
  g_hash_table_iter_init (&iter, _builder->oars);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      const gchar *oars_section = key;
      EpcAppFilterOarsValue oars_value = GPOINTER_TO_INT (value);
      const gchar *oars_value_strs[] =
        {
          NULL,  /* EPC_APP_FILTER_OARS_VALUE_UNKNOWN */
          "none",
          "mild",
          "moderate",
          "intense",
        };

      g_assert ((int) oars_value >= 0 &&
                (int) oars_value < (int) G_N_ELEMENTS (oars_value_strs));

      if (oars_value_strs[oars_value] != NULL)
        g_variant_builder_add (&oars_builder, "{ss}",
                               oars_section, oars_value_strs[oars_value]);
    }

  oars_variant = g_variant_ref_sink (g_variant_builder_end (&oars_builder));

  /* Build the #EpcAppFilter. */
  app_filter = g_new0 (EpcAppFilter, 1);
  app_filter->ref_count = 1;
  app_filter->user_id = -1;
  app_filter->app_list = (gchar **) g_ptr_array_free (g_steal_pointer (&_builder->paths_blacklist), FALSE);
  app_filter->app_list_type = EPC_APP_FILTER_LIST_BLACKLIST;
  app_filter->oars_ratings = g_steal_pointer (&oars_variant);
  app_filter->allow_app_installation = _builder->allow_app_installation;

  epc_app_filter_builder_clear (builder);

  return g_steal_pointer (&app_filter);
}

/**
 * epc_app_filter_builder_blacklist_path:
 * @builder: an initialised #EpcAppFilterBuilder
 * @path: (type filename): an absolute path to blacklist
 *
 * Add @path to the blacklist of app paths in the filter under construction. It
 * will be canonicalised (without doing any I/O) before being added.
 * The canonicalised @path will not be added again if it’s already been added.
 *
 * Since: 0.1.0
 */
void
epc_app_filter_builder_blacklist_path (EpcAppFilterBuilder *builder,
                                       const gchar         *path)
{
  EpcAppFilterBuilderReal *_builder = (EpcAppFilterBuilderReal *) builder;

  g_return_if_fail (_builder != NULL);
  g_return_if_fail (_builder->paths_blacklist != NULL);
  g_return_if_fail (path != NULL);
  g_return_if_fail (g_path_is_absolute (path));

  g_autofree gchar *canonical_path = g_canonicalize_filename (path, "/");

  if (!g_ptr_array_find_with_equal_func (_builder->paths_blacklist,
                                         canonical_path, g_str_equal, NULL))
    g_ptr_array_add (_builder->paths_blacklist, g_steal_pointer (&canonical_path));
}

/**
 * epc_app_filter_builder_blacklist_flatpak_ref:
 * @builder: an initialised #EpcAppFilterBuilder
 * @app_ref: a flatpak app ref to blacklist
 *
 * Add @app_ref to the blacklist of flatpak refs in the filter under
 * construction. The @app_ref will not be added again if it’s already been
 * added.
 *
 * Since: 0.1.0
 */
void
epc_app_filter_builder_blacklist_flatpak_ref (EpcAppFilterBuilder *builder,
                                              const gchar         *app_ref)
{
  EpcAppFilterBuilderReal *_builder = (EpcAppFilterBuilderReal *) builder;

  g_return_if_fail (_builder != NULL);
  g_return_if_fail (_builder->paths_blacklist != NULL);
  g_return_if_fail (app_ref != NULL);

  if (!g_ptr_array_find_with_equal_func (_builder->paths_blacklist,
                                         app_ref, g_str_equal, NULL))
    g_ptr_array_add (_builder->paths_blacklist, g_strdup (app_ref));
}

/**
 * epc_app_filter_builder_set_oars_value:
 * @builder: an initialised #EpcAppFilterBuilder
 * @oars_section: name of the OARS section to set the value for
 * @value: value to set for the @oars_section
 *
 * Set the OARS value for the given @oars_section, indicating the intensity of
 * content covered by that section which the user is allowed to see (inclusive).
 * Any apps which have more intense content in this section should not be usable
 * by the user.
 *
 * Since: 0.1.0
 */
void
epc_app_filter_builder_set_oars_value (EpcAppFilterBuilder   *builder,
                                       const gchar           *oars_section,
                                       EpcAppFilterOarsValue  value)
{
  EpcAppFilterBuilderReal *_builder = (EpcAppFilterBuilderReal *) builder;

  g_return_if_fail (_builder != NULL);
  g_return_if_fail (_builder->oars != NULL);
  g_return_if_fail (oars_section != NULL && *oars_section != '\0');

  g_hash_table_insert (_builder->oars, g_strdup (oars_section),
                       GUINT_TO_POINTER (value));
}

/**
 * epc_app_filter_builder_set_allow_app_installation:
 * @builder: an initialised #EpcAppFilterBuilder
 * @allow_app_installation: %TRUE to allow app installation; %FALSE to
 *    unconditionally disallow it
 *
 * Set whether app installation is allowed in general for the user. If this is
 * %TRUE, app installation is still subject to the OARS values
 * (epc_app_filter_builder_set_oars_value()). If it is %FALSE, app installation
 * is unconditionally disallowed for this user.
 *
 * Since: 0.1.0
 */
void
epc_app_filter_builder_set_allow_app_installation (EpcAppFilterBuilder *builder,
                                                   gboolean             allow_app_installation)
{
  EpcAppFilterBuilderReal *_builder = (EpcAppFilterBuilderReal *) builder;

  g_return_if_fail (_builder != NULL);

  _builder->allow_app_installation = allow_app_installation;
}
