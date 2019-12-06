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

#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include <libmalcontent/session-limits.h>

#include "libmalcontent/session-limits-private.h"


/* struct _MctSessionLimits is defined in session-limits-private.h */

G_DEFINE_BOXED_TYPE (MctSessionLimits, mct_session_limits,
                     mct_session_limits_ref, mct_session_limits_unref)

/**
 * mct_session_limits_ref:
 * @limits: (transfer none): an #MctSessionLimits
 *
 * Increment the reference count of @limits, and return the same pointer to it.
 *
 * Returns: (transfer full): the same pointer as @limits
 * Since: 0.5.0
 */
MctSessionLimits *
mct_session_limits_ref (MctSessionLimits *limits)
{
  g_return_val_if_fail (limits != NULL, NULL);
  g_return_val_if_fail (limits->ref_count >= 1, NULL);
  g_return_val_if_fail (limits->ref_count <= G_MAXINT - 1, NULL);

  limits->ref_count++;
  return limits;
}

/**
 * mct_session_limits_unref:
 * @limits: (transfer full): an #MctSessionLimits
 *
 * Decrement the reference count of @limits. If the reference count reaches
 * zero, free the @limits and all its resources.
 *
 * Since: 0.5.0
 */
void
mct_session_limits_unref (MctSessionLimits *limits)
{
  g_return_if_fail (limits != NULL);
  g_return_if_fail (limits->ref_count >= 1);

  limits->ref_count--;

  if (limits->ref_count <= 0)
    {
      g_free (limits);
    }
}

/**
 * mct_session_limits_get_user_id:
 * @limits: an #MctSessionLimits
 *
 * Get the user ID of the user this #MctSessionLimits is for.
 *
 * Returns: user ID of the relevant user
 * Since: 0.5.0
 */
uid_t
mct_session_limits_get_user_id (MctSessionLimits *limits)
{
  g_return_val_if_fail (limits != NULL, (uid_t) -1);
  g_return_val_if_fail (limits->ref_count >= 1, (uid_t) -1);

  return limits->user_id;
}

/**
 * mct_session_limits_check_time_remaining:
 * @limits: an #MctSessionLimits
 * @now_usecs: current time as microseconds since the Unix epoch (UTC),
 *     typically queried using g_get_real_time()
 * @time_remaining_secs_out: (out) (optional): return location for the number
 *     of seconds remaining before the user’s session has to end, if limits are
 *     in force
 * @time_limit_enabled_out: (out) (optional): return location for whether time
 *     limits are enabled for this user
 *
 * Check whether the user has time remaining in which they are allowed to use
 * the computer, assuming that @now_usecs is the current time, and applying the
 * session limit policy from @limits to it.
 *
 * This will return whether the user is allowed to use the computer now; further
 * information about the policy and remaining time is provided in
 * @time_remaining_secs_out and @time_limit_enabled_out.
 *
 * Returns: %TRUE if the user this @limits corresponds to is allowed to be in
 *     an active session at the given time; %FALSE otherwise
 * Since: 0.5.0
 */
gboolean
mct_session_limits_check_time_remaining (MctSessionLimits *limits,
                                         guint64           now_usecs,
                                         guint64          *time_remaining_secs_out,
                                         gboolean         *time_limit_enabled_out)
{
  guint64 time_remaining_secs;
  gboolean time_limit_enabled;
  gboolean user_allowed_now;
  g_autoptr(GDateTime) now_dt = NULL;
  guint64 now_time_of_day_secs;

  g_return_val_if_fail (limits != NULL, FALSE);
  g_return_val_if_fail (limits->ref_count >= 1, FALSE);

  /* Helper calculations. */
  now_dt = g_date_time_new_from_unix_utc (now_usecs / G_USEC_PER_SEC);
  if (now_dt == NULL)
    {
      time_remaining_secs = 0;
      time_limit_enabled = TRUE;
      user_allowed_now = FALSE;
      goto out;
    }

  now_time_of_day_secs = ((g_date_time_get_hour (now_dt) * 60 +
                           g_date_time_get_minute (now_dt)) * 60 +
                          g_date_time_get_second (now_dt));

  /* Work out the limits. */
  switch (limits->limit_type)
    {
    case MCT_SESSION_LIMITS_TYPE_DAILY_SCHEDULE:
      user_allowed_now = (now_time_of_day_secs >= limits->daily_start_time &&
                          now_time_of_day_secs < limits->daily_end_time);
      time_remaining_secs = user_allowed_now ? (limits->daily_end_time - now_time_of_day_secs) : 0;
      time_limit_enabled = TRUE;

      g_debug ("%s: Daily schedule limit allowed in %u–%u (now is %"
               G_GUINT64_FORMAT "); %" G_GUINT64_FORMAT " seconds remaining",
               G_STRFUNC, limits->daily_start_time, limits->daily_end_time,
               now_time_of_day_secs, time_remaining_secs);

      break;
    case MCT_SESSION_LIMITS_TYPE_NONE:
    default:
      user_allowed_now = TRUE;
      time_remaining_secs = G_MAXUINT64;
      time_limit_enabled = FALSE;

      g_debug ("%s: No limit enabled", G_STRFUNC);

      break;
    }

out:
  /* Postconditions. */
  g_assert (!user_allowed_now || time_remaining_secs > 0);
  g_assert (user_allowed_now || time_remaining_secs == 0);
  g_assert (time_limit_enabled || time_remaining_secs == G_MAXUINT64);

  /* Output. */
  if (time_remaining_secs_out != NULL)
    *time_remaining_secs_out = time_remaining_secs;
  if (time_limit_enabled_out != NULL)
    *time_limit_enabled_out = time_limit_enabled;

  return user_allowed_now;
}
