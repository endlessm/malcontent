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

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "application.h"


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
};

G_DEFINE_TYPE (MctApplication, mct_application, GTK_TYPE_APPLICATION)

static void
mct_application_init (MctApplication *self)
{
  /* Nothing to do here. */
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
mct_application_class_init (MctApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = mct_application_constructed;
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
