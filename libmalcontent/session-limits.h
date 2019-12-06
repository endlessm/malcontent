/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 *
 * Copyright © 2019 Endless Mobile, Inc.
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
 * MctSessionLimits:
 *
 * #MctSessionLimits is an opaque, immutable structure which contains a snapshot
 * of the session limits settings for a user at a given time. This includes
 * whether session limits are being enforced, and the limit policy — for
 * example, the times of day when a user is allowed to use the computer.
 *
 * Typically, session limits settings can only be changed by the administrator,
 * and are read-only for non-administrative users. The precise policy is set
 * using polkit.
 *
 * Since: 0.5.0
 */
typedef struct _MctSessionLimits MctSessionLimits;
GType mct_session_limits_get_type (void);

MctSessionLimits *mct_session_limits_ref   (MctSessionLimits *limits);
void              mct_session_limits_unref (MctSessionLimits *limits);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (MctSessionLimits, mct_session_limits_unref)

uid_t    mct_session_limits_get_user_id          (MctSessionLimits *limits);
gboolean mct_session_limits_check_time_remaining (MctSessionLimits *limits,
                                                  guint64           now_usecs,
                                                  guint64          *time_remaining_secs_out,
                                                  gboolean         *time_limit_enabled_out);

G_END_DECLS
