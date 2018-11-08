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

#pragma once

#include <gio/gio.h>
#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

/**
 * EpcAppFilterError:
 * @EPC_APP_FILTER_ERROR_INVALID_USER: Given user ID doesn’t exist
 * @EPC_APP_FILTER_ERROR_PERMISSION_DENIED: Not authorized to query the app
 *    filter for the given user
 * @EPC_APP_FILTER_ERROR_INVALID_DATA: The data stored in the app filter for
 *    a user is inconsistent or invalid
 *
 * Errors which can be returned by epc_get_app_filter_async().
 *
 * Since: 0.1.0
 */
typedef enum
{
  EPC_APP_FILTER_ERROR_INVALID_USER,
  EPC_APP_FILTER_ERROR_PERMISSION_DENIED,
  EPC_APP_FILTER_ERROR_INVALID_DATA,
} EpcAppFilterError;

GQuark epc_app_filter_error_quark (void);
#define EPC_APP_FILTER_ERROR epc_app_filter_error_quark ()

/**
 * EpcAppFilterOarsValue:
 * @EPC_APP_FILTER_OARS_VALUE_UNKNOWN: Unknown value for the given
 *    section.
 * @EPC_APP_FILTER_OARS_VALUE_NONE: No rating for the given section.
 * @EPC_APP_FILTER_OARS_VALUE_MILD: Mild rating for the given section.
 * @EPC_APP_FILTER_OARS_VALUE_MODERATE: Moderate rating for the given
 *    section.
 * @EPC_APP_FILTER_OARS_VALUE_INTENSE: Intense rating for the given
 *    section.
 *
 * Rating values of the intensity of a given section in an app or game.
 * These are directly equivalent to the values in the #AsContentRatingValue
 * enumeration in libappstream.
 *
 * Since: 0.1.0
 */
typedef enum
{
  EPC_APP_FILTER_OARS_VALUE_UNKNOWN,
  EPC_APP_FILTER_OARS_VALUE_NONE,
  EPC_APP_FILTER_OARS_VALUE_MILD,
  EPC_APP_FILTER_OARS_VALUE_MODERATE,
  EPC_APP_FILTER_OARS_VALUE_INTENSE,
} EpcAppFilterOarsValue;

/**
 * EpcAppFilter:
 *
 * #EpcAppFilter is an opaque, immutable structure which contains a snapshot of
 * the app filtering settings for a user at a given time. This includes a list
 * of apps which are explicitly banned or allowed to be run by that user.
 *
 * Typically, app filter settings can only be changed by the administrator, and
 * are read-only for non-administrative users. The precise policy is set using
 * polkit.
 *
 * Since: 0.1.0
 */
typedef struct _EpcAppFilter EpcAppFilter;
GType epc_app_filter_get_type (void);

EpcAppFilter *epc_app_filter_ref   (EpcAppFilter *filter);
void          epc_app_filter_unref (EpcAppFilter *filter);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (EpcAppFilter, epc_app_filter_unref)

uid_t    epc_app_filter_get_user_id            (EpcAppFilter *filter);
gboolean epc_app_filter_is_path_allowed        (EpcAppFilter *filter,
                                                const gchar  *path);
gboolean epc_app_filter_is_flatpak_ref_allowed (EpcAppFilter *filter,
                                                const gchar  *flatpak_ref);

const gchar           **epc_app_filter_get_oars_sections (EpcAppFilter *filter);
EpcAppFilterOarsValue   epc_app_filter_get_oars_value    (EpcAppFilter *filter,
                                                          const gchar  *oars_section);

EpcAppFilter *epc_get_app_filter        (GDBusConnection      *connection,
                                         uid_t                 user_id,
                                         gboolean              allow_interactive_authorization,
                                         GCancellable         *cancellable,
                                         GError              **error);
void          epc_get_app_filter_async  (GDBusConnection      *connection,
                                         uid_t                 user_id,
                                         gboolean              allow_interactive_authorization,
                                         GCancellable         *cancellable,
                                         GAsyncReadyCallback   callback,
                                         gpointer              user_data);
EpcAppFilter *epc_get_app_filter_finish (GAsyncResult         *result,
                                         GError              **error);

gboolean      epc_set_app_filter        (GDBusConnection      *connection,
                                         uid_t                 user_id,
                                         EpcAppFilter         *app_filter,
                                         gboolean              allow_interactive_authorization,
                                         GCancellable         *cancellable,
                                         GError              **error);
void          epc_set_app_filter_async  (GDBusConnection      *connection,
                                         uid_t                 user_id,
                                         EpcAppFilter         *app_filter,
                                         gboolean              allow_interactive_authorization,
                                         GCancellable         *cancellable,
                                         GAsyncReadyCallback   callback,
                                         gpointer              user_data);
gboolean      epc_set_app_filter_finish (GAsyncResult         *result,
                                         GError              **error);

/**
 * EpcAppFilterBuilder:
 *
 * #EpcAppFilterBuilder is a stack-allocated mutable structure used to build an
 * #EpcAppFilter instance. Use epc_app_filter_builder_init(), various method
 * calls to set properties of the app filter, and then
 * epc_app_filter_builder_end(), to construct an #EpcAppFilter.
 *
 * Since: 0.1.0
 */
typedef struct
{
  /*< private >*/
  gpointer p0;
  gpointer p1;
  gpointer p2;
  gpointer p3;
} EpcAppFilterBuilder;

GType epc_app_filter_builder_get_type (void);

/**
 * EPC_APP_FILTER_BUILDER_INIT:
 *
 * Initialise a stack-allocated #EpcAppFilterBuilder instance at declaration
 * time.
 *
 * This is typically used with g_auto():
 * |[
 * g_auto(EpcAppFilterBuilder) builder = EPC_APP_FILTER_BUILDER_INIT ();
 * ]|
 *
 * Since: 0.1.0
 */
#define EPC_APP_FILTER_BUILDER_INIT() \
  { \
    g_ptr_array_new_with_free_func (g_free), \
    g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL) \
  }

void epc_app_filter_builder_init  (EpcAppFilterBuilder *builder);
void epc_app_filter_builder_clear (EpcAppFilterBuilder *builder);

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC (EpcAppFilterBuilder,
                                  epc_app_filter_builder_clear)

EpcAppFilterBuilder *epc_app_filter_builder_new  (void);
EpcAppFilterBuilder *epc_app_filter_builder_copy (EpcAppFilterBuilder *builder);
void                 epc_app_filter_builder_free (EpcAppFilterBuilder *builder);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (EpcAppFilterBuilder, epc_app_filter_builder_free)

EpcAppFilter *epc_app_filter_builder_end (EpcAppFilterBuilder *builder);

void epc_app_filter_builder_blacklist_path        (EpcAppFilterBuilder   *builder,
                                                   const gchar           *path);
void epc_app_filter_builder_blacklist_flatpak_ref (EpcAppFilterBuilder *builder,
                                                   const gchar         *app_ref);
void epc_app_filter_builder_set_oars_value        (EpcAppFilterBuilder   *builder,
                                                   const gchar           *oars_section,
                                                   EpcAppFilterOarsValue  value);

G_END_DECLS
