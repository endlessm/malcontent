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

/* Static definition of the AppFilter interface on org.freedesktop.Accounts.
 * FIXME: Once we can depend on a new enough version of GLib, generate this
 * from introspection XML using `gdbus-codegen --interface-info-{header,body}`. */
static const GDBusPropertyInfo app_filter_property_app_filter =
{
  .ref_count = -1,  /* static */
  .name = (gchar *) "AppFilter",
  .signature = (gchar *) "(bas)",
  .flags = G_DBUS_PROPERTY_INFO_FLAGS_READABLE | G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE,
  .annotations = NULL,
};

static const GDBusPropertyInfo app_filter_property_oars_filter =
{
  .ref_count = -1,  /* static */
  .name = (gchar *) "OarsFilter",
  .signature = (gchar *) "(sa{ss})",
  .flags = G_DBUS_PROPERTY_INFO_FLAGS_READABLE | G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE,
  .annotations = NULL,
};

static const GDBusPropertyInfo app_filter_property_allow_user_installation =
{
  .ref_count = -1,  /* static */
  .name = (gchar *) "AllowUserInstallation",
  .signature = (gchar *) "b",
  .flags = G_DBUS_PROPERTY_INFO_FLAGS_READABLE | G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE,
  .annotations = NULL,
};

static const GDBusPropertyInfo app_filter_property_allow_system_installation =
{
  .ref_count = -1,  /* static */
  .name = (gchar *) "AllowSystemInstallation",
  .signature = (gchar *) "b",
  .flags = G_DBUS_PROPERTY_INFO_FLAGS_READABLE | G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE,
  .annotations = NULL,
};

static const GDBusPropertyInfo *app_filter_properties[] =
{
  (GDBusPropertyInfo *) &app_filter_property_app_filter,
  (GDBusPropertyInfo *) &app_filter_property_oars_filter,
  (GDBusPropertyInfo *) &app_filter_property_allow_user_installation,
  (GDBusPropertyInfo *) &app_filter_property_allow_system_installation,
  NULL,
};

static const GDBusInterfaceInfo app_filter_interface_info =
{
  .ref_count = -1,  /* static */
  .name = (gchar *) "com.endlessm.ParentalControls.AppFilter",
  .methods = NULL,
  .signals = NULL,
  .properties = (GDBusPropertyInfo **) &app_filter_properties,
  .annotations = NULL,
};

static const GDBusArgInfo accounts_method_find_user_by_id_arg_user_id =
{
  .ref_count = -1,  /* static */
  .name = (gchar *) "UserId",
  .signature = (gchar *) "x",
  .annotations = NULL,
};
static const GDBusArgInfo accounts_method_find_user_by_id_arg_object_path =
{
  .ref_count = -1,  /* static */
  .name = (gchar *) "ObjectPath",
  .signature = (gchar *) "o",
  .annotations = NULL,
};
static const GDBusArgInfo *accounts_method_find_user_by_id_in_args[] =
{
  (GDBusArgInfo *) &accounts_method_find_user_by_id_arg_user_id,
  NULL,
};
static const GDBusArgInfo *accounts_method_find_user_by_id_out_args[] =
{
  (GDBusArgInfo *) &accounts_method_find_user_by_id_arg_object_path,
  NULL,
};
static const GDBusMethodInfo accounts_method_find_user_by_id =
{
  .ref_count = -1,  /* static */
  .name = (gchar *) "FindUserById",
  .in_args = (GDBusArgInfo **) &accounts_method_find_user_by_id_in_args,
  .out_args = (GDBusArgInfo **) &accounts_method_find_user_by_id_out_args,
  .annotations = NULL,
};

static const GDBusMethodInfo *accounts_methods[] =
{
  (GDBusMethodInfo *) &accounts_method_find_user_by_id,
  NULL,
};

static const GDBusInterfaceInfo accounts_interface_info =
{
  .ref_count = -1,  /* static */
  .name = (gchar *) "org.freedesktop.Accounts",
  .methods = (GDBusMethodInfo **) &accounts_methods,
  .signals = NULL,
  .properties = NULL,
  .annotations = NULL,
};

G_END_DECLS
