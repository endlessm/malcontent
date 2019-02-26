malcontent
==========

malcontent implements support for restricting the abilities of
non-administrator accounts on a Linux system. Typically, when this is
used, a non-administrator account will be for a child using the system; and the
administrator accounts will be for the parents.

It provides an accounts service vendor extension for storing an app filter to
restrict the child’s access to certain applications; and a simple library for
accessing and applying the app filter.

All the library APIs are currently unstable and are likely to change wildly.

Dependencies
------------

 • accounts-service
 • dbus-daemon
 • gio-2.0 ≥ 2.54
 • glib-2.0 ≥ 2.54
 • gobject-2.0 ≥ 2.54

Licensing
---------

All code in this project is licensed under LGPL-2.1+. See COPYING for more details.

Bugs
----

Bug reports and patches should be filed in
[GitLab](https://gitlab.freedesktop.org/pwithnall/malcontent).
