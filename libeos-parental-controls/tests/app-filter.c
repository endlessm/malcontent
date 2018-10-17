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
#include <gio/gio.h>
#include <libeos-parental-controls/app-filter.h>
#include <locale.h>
#include <string.h>


/* A placeholder smoketest which checks that the error quark works. */
static void
test_app_filter_error_quark (void)
{
  g_assert_cmpint (epc_app_filter_error_quark (), !=, 0);
}

/* Fixture for tests which use an #EpcAppFilterBuilder. The builder can either
 * be heap- or stack-allocated. @builder will always be a valid pointer to it.
 */
typedef struct
{
  EpcAppFilterBuilder *builder;
  EpcAppFilterBuilder stack_builder;
} BuilderFixture;

static void
builder_set_up_stack (BuilderFixture *fixture,
                      gconstpointer   test_data)
{
  epc_app_filter_builder_init (&fixture->stack_builder);
  fixture->builder = &fixture->stack_builder;
}

static void
builder_tear_down_stack (BuilderFixture *fixture,
                         gconstpointer   test_data)
{
  epc_app_filter_builder_clear (&fixture->stack_builder);
  fixture->builder = NULL;
}

static void
builder_set_up_stack2 (BuilderFixture *fixture,
                       gconstpointer   test_data)
{
  EpcAppFilterBuilder local_builder = EPC_APP_FILTER_BUILDER_INIT ();
  memcpy (&fixture->stack_builder, &local_builder, sizeof (local_builder));
  fixture->builder = &fixture->stack_builder;
}

static void
builder_tear_down_stack2 (BuilderFixture *fixture,
                          gconstpointer   test_data)
{
  epc_app_filter_builder_clear (&fixture->stack_builder);
  fixture->builder = NULL;
}

static void
builder_set_up_heap (BuilderFixture *fixture,
                     gconstpointer   test_data)
{
  fixture->builder = epc_app_filter_builder_new ();
}

static void
builder_tear_down_heap (BuilderFixture *fixture,
                        gconstpointer   test_data)
{
  g_clear_pointer (&fixture->builder, epc_app_filter_builder_free);
}

/* Test building a non-empty #EpcAppFilter using an #EpcAppFilterBuilder. */
static void
test_app_filter_builder_non_empty (BuilderFixture *fixture,
                                   gconstpointer   test_data)
{
  g_autoptr(EpcAppFilter) filter = NULL;

  epc_app_filter_builder_blacklist_path (fixture->builder, "/bin/true");
  epc_app_filter_builder_blacklist_path (fixture->builder, "/usr/bin/gnome-software");

  epc_app_filter_builder_set_oars_value (fixture->builder, "drugs-alcohol",
                                         EPC_APP_FILTER_OARS_VALUE_MILD);
  epc_app_filter_builder_set_oars_value (fixture->builder, "language-humor",
                                         EPC_APP_FILTER_OARS_VALUE_MODERATE);

  filter = epc_app_filter_builder_end (fixture->builder);

  g_assert_true (epc_app_filter_is_path_allowed (filter, "/bin/false"));
  g_assert_false (epc_app_filter_is_path_allowed (filter,
                                                  "/usr/bin/gnome-software"));

  g_assert_cmpint (epc_app_filter_get_oars_value (filter, "drugs-alcohol"), ==,
                   EPC_APP_FILTER_OARS_VALUE_MILD);
  g_assert_cmpint (epc_app_filter_get_oars_value (filter, "language-humor"), ==,
                   EPC_APP_FILTER_OARS_VALUE_MODERATE);
  g_assert_cmpint (epc_app_filter_get_oars_value (filter, "something-else"), ==,
                   EPC_APP_FILTER_OARS_VALUE_UNKNOWN);
}

/* Test building an empty #EpcAppFilter using an #EpcAppFilterBuilder. */
static void
test_app_filter_builder_empty (BuilderFixture *fixture,
                               gconstpointer   test_data)
{
  g_autoptr(EpcAppFilter) filter = NULL;

  filter = epc_app_filter_builder_end (fixture->builder);

  g_assert_true (epc_app_filter_is_path_allowed (filter, "/bin/false"));
  g_assert_true (epc_app_filter_is_path_allowed (filter,
                                                 "/usr/bin/gnome-software"));

  g_assert_cmpint (epc_app_filter_get_oars_value (filter, "drugs-alcohol"), ==,
                   EPC_APP_FILTER_OARS_VALUE_UNKNOWN);
  g_assert_cmpint (epc_app_filter_get_oars_value (filter, "language-humor"), ==,
                   EPC_APP_FILTER_OARS_VALUE_UNKNOWN);
  g_assert_cmpint (epc_app_filter_get_oars_value (filter, "something-else"), ==,
                   EPC_APP_FILTER_OARS_VALUE_UNKNOWN);
}

int
main (int    argc,
      char **argv)
{
  setlocale (LC_ALL, "");
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/app-filter/error-quark", test_app_filter_error_quark);

  g_test_add ("/app-filter/builder/stack/non-empty", BuilderFixture, NULL,
              builder_set_up_stack, test_app_filter_builder_non_empty,
              builder_tear_down_stack);
  g_test_add ("/app-filter/builder/stack/empty", BuilderFixture, NULL,
              builder_set_up_stack, test_app_filter_builder_empty,
              builder_tear_down_stack);
  g_test_add ("/app-filter/builder/stack2/non-empty", BuilderFixture, NULL,
              builder_set_up_stack2, test_app_filter_builder_non_empty,
              builder_tear_down_stack2);
  g_test_add ("/app-filter/builder/stack2/empty", BuilderFixture, NULL,
              builder_set_up_stack2, test_app_filter_builder_empty,
              builder_tear_down_stack2);
  g_test_add ("/app-filter/builder/heap/non-empty", BuilderFixture, NULL,
              builder_set_up_heap, test_app_filter_builder_non_empty,
              builder_tear_down_heap);
  g_test_add ("/app-filter/builder/heap/empty", BuilderFixture, NULL,
              builder_set_up_heap, test_app_filter_builder_empty,
              builder_tear_down_heap);

  return g_test_run ();
}
