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
 *
 * Errors which can be returned by epc_get_app_filter_async().
 *
 * Since: 0.1.0
 */
typedef enum
{
  EPC_APP_FILTER_ERROR_INVALID_USER,
  EPC_APP_FILTER_ERROR_PERMISSION_DENIED,
} EpcAppFilterError;

GQuark epc_app_filter_error_quark (void);
#define EPC_APP_FILTER_ERROR epc_app_filter_error_quark ()

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

uid_t    epc_app_filter_get_user_id     (EpcAppFilter *filter);
gboolean epc_app_filter_is_path_allowed (EpcAppFilter *filter,
                                         const gchar  *path);

void          epc_get_app_filter_async  (GDBusConnection      *connection,
                                         uid_t                 user_id,
                                         gboolean              allow_interactive_authorization,
                                         GCancellable         *cancellable,
                                         GAsyncReadyCallback   callback,
                                         gpointer              user_data);
EpcAppFilter *epc_get_app_filter_finish (GAsyncResult         *result,
                                         GError              **error);

G_END_DECLS
