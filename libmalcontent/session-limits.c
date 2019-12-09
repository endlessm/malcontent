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

/*
 * Actual implementation of #MctSessionLimitsBuilder.
 *
 * All members are %NULL if un-initialised, cleared, or ended.
 */
typedef struct
{
  MctSessionLimitsType limit_type;

  /* Which member is used is determined by @limit_type: */
  union
    {
      struct
        {
          guint start_time;  /* seconds since midnight */
          guint end_time;  /* seconds since midnight */
        } daily_schedule;
    };

  /*< private >*/
  gpointer padding[10];
} MctSessionLimitsBuilderReal;

G_STATIC_ASSERT (sizeof (MctSessionLimitsBuilderReal) ==
                 sizeof (MctSessionLimitsBuilder));
G_STATIC_ASSERT (__alignof__ (MctSessionLimitsBuilderReal) ==
                 __alignof__ (MctSessionLimitsBuilder));

G_DEFINE_BOXED_TYPE (MctSessionLimitsBuilder, mct_session_limits_builder,
                     mct_session_limits_builder_copy, mct_session_limits_builder_free)

/**
 * mct_session_limits_builder_init:
 * @builder: an uninitialised #MctSessionLimitsBuilder
 *
 * Initialise the given @builder so it can be used to construct a new
 * #MctSessionLimits. @builder must have been allocated on the stack, and must
 * not already be initialised.
 *
 * Construct the #MctSessionLimits by calling methods on @builder, followed by
 * mct_session_limits_builder_end(). To abort construction, use
 * mct_session_limits_builder_clear().
 *
 * Since: 0.5.0
 */
void
mct_session_limits_builder_init (MctSessionLimitsBuilder *builder)
{
  MctSessionLimitsBuilder local_builder = MCT_SESSION_LIMITS_BUILDER_INIT ();
  MctSessionLimitsBuilderReal *_builder = (MctSessionLimitsBuilderReal *) builder;

  g_return_if_fail (_builder != NULL);
  g_return_if_fail (_builder->limit_type == MCT_SESSION_LIMITS_TYPE_NONE);

  memcpy (builder, &local_builder, sizeof (local_builder));
}

/**
 * mct_session_limits_builder_clear:
 * @builder: an #MctSessionLimitsBuilder
 *
 * Clear @builder, freeing any internal state in it. This will not free the
 * top-level storage for @builder itself, which is assumed to be allocated on
 * the stack.
 *
 * If called on an already-cleared #MctSessionLimitsBuilder, this function is
 * idempotent.
 *
 * Since: 0.5.0
 */
void
mct_session_limits_builder_clear (MctSessionLimitsBuilder *builder)
{
  MctSessionLimitsBuilderReal *_builder = (MctSessionLimitsBuilderReal *) builder;

  g_return_if_fail (_builder != NULL);

  /* Nothing to free here for now. */
  _builder->limit_type = MCT_SESSION_LIMITS_TYPE_NONE;
}

/**
 * mct_session_limits_builder_new:
 *
 * Construct a new #MctSessionLimitsBuilder on the heap. This is intended for
 * language bindings. The returned builder must eventually be freed with
 * mct_session_limits_builder_free(), but can be cleared zero or more times with
 * mct_session_limits_builder_clear() first.
 *
 * Returns: (transfer full): a new heap-allocated #MctSessionLimitsBuilder
 * Since: 0.5.0
 */
MctSessionLimitsBuilder *
mct_session_limits_builder_new (void)
{
  g_autoptr(MctSessionLimitsBuilder) builder = NULL;

  builder = g_new0 (MctSessionLimitsBuilder, 1);
  mct_session_limits_builder_init (builder);

  return g_steal_pointer (&builder);
}

/**
 * mct_session_limits_builder_copy:
 * @builder: an #MctSessionLimitsBuilder
 *
 * Copy the given @builder to a newly-allocated #MctSessionLimitsBuilder on the
 * heap. This is safe to use with cleared, stack-allocated
 * #MctSessionLimitsBuilders.
 *
 * Returns: (transfer full): a copy of @builder
 * Since: 0.5.0
 */
MctSessionLimitsBuilder *
mct_session_limits_builder_copy (MctSessionLimitsBuilder *builder)
{
  MctSessionLimitsBuilderReal *_builder = (MctSessionLimitsBuilderReal *) builder;
  g_autoptr(MctSessionLimitsBuilder) copy = NULL;
  MctSessionLimitsBuilderReal *_copy;

  g_return_val_if_fail (builder != NULL, NULL);

  copy = mct_session_limits_builder_new ();
  _copy = (MctSessionLimitsBuilderReal *) copy;

  mct_session_limits_builder_clear (copy);
  _copy->limit_type = _builder->limit_type;

  switch (_builder->limit_type)
    {
    case MCT_SESSION_LIMITS_TYPE_DAILY_SCHEDULE:
      _copy->daily_schedule.start_time = _builder->daily_schedule.start_time;
      _copy->daily_schedule.end_time = _builder->daily_schedule.end_time;
      break;
    case MCT_SESSION_LIMITS_TYPE_NONE:
    default:
      break;
    }

  return g_steal_pointer (&copy);
}

/**
 * mct_session_limits_builder_free:
 * @builder: a heap-allocated #MctSessionLimitsBuilder
 *
 * Free an #MctSessionLimitsBuilder originally allocated using
 * mct_session_limits_builder_new(). This must not be called on stack-allocated
 * builders initialised using mct_session_limits_builder_init().
 *
 * Since: 0.5.0
 */
void
mct_session_limits_builder_free (MctSessionLimitsBuilder *builder)
{
  g_return_if_fail (builder != NULL);

  mct_session_limits_builder_clear (builder);
  g_free (builder);
}

/**
 * mct_session_limits_builder_end:
 * @builder: an initialised #MctSessionLimitsBuilder
 *
 * Finish constructing an #MctSessionLimits with the given @builder, and return
 * it. The #MctSessionLimitsBuilder will be cleared as if
 * mct_session_limits_builder_clear() had been called.
 *
 * Returns: (transfer full): a newly constructed #MctSessionLimits
 * Since: 0.5.0
 */
MctSessionLimits *
mct_session_limits_builder_end (MctSessionLimitsBuilder *builder)
{
  MctSessionLimitsBuilderReal *_builder = (MctSessionLimitsBuilderReal *) builder;
  g_autoptr(MctSessionLimits) session_limits = NULL;

  g_return_val_if_fail (_builder != NULL, NULL);

  /* Build the #MctSessionLimits. */
  session_limits = g_new0 (MctSessionLimits, 1);
  session_limits->ref_count = 1;
  session_limits->user_id = -1;
  session_limits->limit_type = _builder->limit_type;

  switch (_builder->limit_type)
    {
    case MCT_SESSION_LIMITS_TYPE_DAILY_SCHEDULE:
      session_limits->daily_start_time = _builder->daily_schedule.start_time;
      session_limits->daily_end_time = _builder->daily_schedule.end_time;
      break;
    case MCT_SESSION_LIMITS_TYPE_NONE:
    default:
      /* Defaults: */
      session_limits->daily_start_time = 0;
      session_limits->daily_end_time = 24 * 60 * 60;
      break;
    }

  mct_session_limits_builder_clear (builder);

  return g_steal_pointer (&session_limits);
}

/**
 * mct_session_limits_builder_set_none:
 * @builder: an initialised #MctSessionLimitsBuilder
 *
 * Unset any session limits currently set in the @builder.
 *
 * Since: 0.5.0
 */
void
mct_session_limits_builder_set_none (MctSessionLimitsBuilder *builder)
{
  MctSessionLimitsBuilderReal *_builder = (MctSessionLimitsBuilderReal *) builder;

  g_return_if_fail (_builder != NULL);

  /* This will need to free other limit types’ data first in future. */
  _builder->limit_type = MCT_SESSION_LIMITS_TYPE_NONE;
}

/**
 * mct_session_limits_builder_set_daily_schedule:
 * @builder: an initialised #MctSessionLimitsBuilder
 * @start_time_secs: number of seconds since midnight when the user’s session
 *     can first start
 * @end_time_secs: number of seconds since midnight when the user’s session can
 *     last end
 *
 * Set the session limits in @builder to be a daily schedule, where sessions are
 * allowed between @start_time_secs and @end_time_secs every day.
 * @start_time_secs and @end_time_secs are given as offsets from the start of
 * the day, in seconds. @end_time_secs must be greater than @start_time_secs.
 * @end_time_secs must be at most `24 * 60 * 60`.
 *
 * This will overwrite any other session limits.
 *
 * Since: 0.5.0
 */
void
mct_session_limits_builder_set_daily_schedule (MctSessionLimitsBuilder *builder,
                                               guint                    start_time_secs,
                                               guint                    end_time_secs)
{
  MctSessionLimitsBuilderReal *_builder = (MctSessionLimitsBuilderReal *) builder;

  g_return_if_fail (_builder != NULL);
  g_return_if_fail (start_time_secs < end_time_secs);
  g_return_if_fail (end_time_secs <= 24 * 60 * 60);

  /* This will need to free other limit types’ data first in future. */
  _builder->limit_type = MCT_SESSION_LIMITS_TYPE_DAILY_SCHEDULE;
  _builder->daily_schedule.start_time = start_time_secs;
  _builder->daily_schedule.end_time = end_time_secs;
}
