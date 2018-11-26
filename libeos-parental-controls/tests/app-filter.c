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


/* Check two arrays contain exactly the same items in the same order. */
static void
assert_strv_equal (const gchar * const *strv_a,
                   const gchar * const *strv_b)
{
  gsize i;

  for (i = 0; strv_a[i] != NULL && strv_b[i] != NULL; i++)
    g_assert_cmpstr (strv_a[i], ==, strv_b[i]);

  g_assert_null (strv_a[i]);
  g_assert_null (strv_b[i]);
}


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
  g_autofree const gchar **sections = NULL;

  epc_app_filter_builder_blacklist_path (fixture->builder, "/bin/true");
  epc_app_filter_builder_blacklist_path (fixture->builder, "/usr/bin/gnome-software");

  epc_app_filter_builder_blacklist_flatpak_ref (fixture->builder,
                                                "app/org.doom.Doom/x86_64/master");

  epc_app_filter_builder_set_oars_value (fixture->builder, "drugs-alcohol",
                                         EPC_APP_FILTER_OARS_VALUE_MILD);
  epc_app_filter_builder_set_oars_value (fixture->builder, "language-humor",
                                         EPC_APP_FILTER_OARS_VALUE_MODERATE);
  epc_app_filter_builder_set_allow_app_installation (fixture->builder, FALSE);

  filter = epc_app_filter_builder_end (fixture->builder);

  g_assert_true (epc_app_filter_is_path_allowed (filter, "/bin/false"));
  g_assert_false (epc_app_filter_is_path_allowed (filter,
                                                  "/usr/bin/gnome-software"));

  g_assert_true (epc_app_filter_is_flatpak_ref_allowed (filter,
                                                        "app/org.gnome.Ponies/x86_64/master"));
  g_assert_true (epc_app_filter_is_flatpak_app_allowed (filter, "org.gnome.Ponies"));
  g_assert_false (epc_app_filter_is_flatpak_ref_allowed (filter,
                                                         "app/org.doom.Doom/x86_64/master"));
  g_assert_false (epc_app_filter_is_flatpak_app_allowed (filter, "org.doom.Doom"));

  g_assert_cmpint (epc_app_filter_get_oars_value (filter, "drugs-alcohol"), ==,
                   EPC_APP_FILTER_OARS_VALUE_MILD);
  g_assert_cmpint (epc_app_filter_get_oars_value (filter, "language-humor"), ==,
                   EPC_APP_FILTER_OARS_VALUE_MODERATE);
  g_assert_cmpint (epc_app_filter_get_oars_value (filter, "something-else"), ==,
                   EPC_APP_FILTER_OARS_VALUE_UNKNOWN);

  sections = epc_app_filter_get_oars_sections (filter);
  const gchar * const expected_sections[] = { "drugs-alcohol", "language-humor", NULL };
  assert_strv_equal ((const gchar * const *) sections, expected_sections);

  g_assert_false (epc_app_filter_is_app_installation_allowed (filter));
}

/* Test building an empty #EpcAppFilter using an #EpcAppFilterBuilder. */
static void
test_app_filter_builder_empty (BuilderFixture *fixture,
                               gconstpointer   test_data)
{
  g_autoptr(EpcAppFilter) filter = NULL;
  g_autofree const gchar **sections = NULL;

  filter = epc_app_filter_builder_end (fixture->builder);

  g_assert_true (epc_app_filter_is_path_allowed (filter, "/bin/false"));
  g_assert_true (epc_app_filter_is_path_allowed (filter,
                                                 "/usr/bin/gnome-software"));

  g_assert_true (epc_app_filter_is_flatpak_ref_allowed (filter,
                                                        "app/org.gnome.Ponies/x86_64/master"));
  g_assert_true (epc_app_filter_is_flatpak_app_allowed (filter, "org.gnome.Ponies"));
  g_assert_true (epc_app_filter_is_flatpak_ref_allowed (filter,
                                                        "app/org.doom.Doom/x86_64/master"));
  g_assert_true (epc_app_filter_is_flatpak_app_allowed (filter, "org.doom.Doom"));

  g_assert_cmpint (epc_app_filter_get_oars_value (filter, "drugs-alcohol"), ==,
                   EPC_APP_FILTER_OARS_VALUE_UNKNOWN);
  g_assert_cmpint (epc_app_filter_get_oars_value (filter, "language-humor"), ==,
                   EPC_APP_FILTER_OARS_VALUE_UNKNOWN);
  g_assert_cmpint (epc_app_filter_get_oars_value (filter, "something-else"), ==,
                   EPC_APP_FILTER_OARS_VALUE_UNKNOWN);

  sections = epc_app_filter_get_oars_sections (filter);
  const gchar * const expected_sections[] = { NULL };
  assert_strv_equal ((const gchar * const *) sections, expected_sections);

  g_assert_true (epc_app_filter_is_app_installation_allowed (filter));
}

/* Check that copying a cleared #EpcAppFilterBuilder works, and the copy can
 * then be initialised and used to build a filter. */
static void
test_app_filter_builder_copy_empty (void)
{
  g_autoptr(EpcAppFilterBuilder) builder = epc_app_filter_builder_new ();
  g_autoptr(EpcAppFilterBuilder) builder_copy = NULL;
  g_autoptr(EpcAppFilter) filter = NULL;

  epc_app_filter_builder_clear (builder);
  builder_copy = epc_app_filter_builder_copy (builder);

  epc_app_filter_builder_init (builder_copy);
  epc_app_filter_builder_blacklist_path (builder_copy, "/bin/true");
  filter = epc_app_filter_builder_end (builder_copy);

  g_assert_true (epc_app_filter_is_path_allowed (filter, "/bin/false"));
  g_assert_false (epc_app_filter_is_path_allowed (filter, "/bin/true"));

  g_assert_true (epc_app_filter_is_app_installation_allowed (filter));
}

/* Check that copying a filled #EpcAppFilterBuilder works, and the copy can be
 * used to build a filter. */
static void
test_app_filter_builder_copy_full (void)
{
  g_autoptr(EpcAppFilterBuilder) builder = epc_app_filter_builder_new ();
  g_autoptr(EpcAppFilterBuilder) builder_copy = NULL;
  g_autoptr(EpcAppFilter) filter = NULL;

  epc_app_filter_builder_blacklist_path (builder, "/bin/true");
  epc_app_filter_builder_set_allow_app_installation (builder, FALSE);
  builder_copy = epc_app_filter_builder_copy (builder);
  filter = epc_app_filter_builder_end (builder_copy);

  g_assert_true (epc_app_filter_is_path_allowed (filter, "/bin/false"));
  g_assert_false (epc_app_filter_is_path_allowed (filter, "/bin/true"));
  g_assert_false (epc_app_filter_is_app_installation_allowed (filter));
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
  g_test_add_func ("/app-filter/builder/copy/empty",
                   test_app_filter_builder_copy_empty);
  g_test_add_func ("/app-filter/builder/copy/full",
                   test_app_filter_builder_copy_full);

  return g_test_run ();
}
