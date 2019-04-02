malcontent
==========

malcontent implements support for restricting the abilities of
non-administrator accounts on a Linux system. Typically, when this is
used, a non-administrator account will be for a child using the system; and the
administrator accounts will be for the parents.

It provides an
[accounts-service](https://gitlab.freedesktop.org/accountsservice/accountsservice)
vendor extension for storing an app filter to
restrict the child’s access to certain applications; and a simple library for
accessing and applying the app filter. This results in the policy being stored
in `/var/lib/AccountsService/users/${user}`, which is a key file readable and
writable only by the accounts-service daemon. Access to the data is mediated
through accounts-service’s D-Bus interface, which libmalcontent is a client
library for

All the library APIs are currently unstable and are likely to change wildly.

Two kinds of policy are currently supported:
 * A filter specifying whether installed applications are allowed to be run;
   this is typically set up to restrict access to a limited set of
   already-installed applications — but it can be set up to only allow access
   to a fixed list of applications and deny access to all others.
   Applications which are not currently installed are not subject to this
   filter.
 * A set of mappings from [OARS categories](https://hughsie.github.io/oars/) to
   the maximum ratings for those categories which are permissible for a user to
   install apps with. For example, a mapping of `violence-realistic=mild` would
   prevent any applications containing more than ‘mild’ violence from being
   installed. Applications which are already installed are not subject to this
   filter.

Additional policies may be added in future.

Example usage
---

malcontent ships a `malcontent-client` application which can be used to get and
set parental controls policies for users.

```
$ # This sets the parental controls policy for user ‘philip’ to allow no \\
    installation of apps with anything more than ‘none’ for realistic violence, \\
    and to blacklist running the org.freedesktop.Bustle flatpak:
$ malcontent-client set philip \\
    violence-realistic=none \\
    app/org.freedesktop.Bustle/x86_64/stable
App filter for user 1000 set
```

With that policy in place, other applications which are aware of malcontent will
apply the policy:

```
$ flatpak run org.freedesktop.Bustle
error: Running app/org.freedesktop.Bustle/x86_64/stable is not allowed by the policy set by your administrator
```

Dependencies
------------

 * accounts-service
 * dbus-daemon
 * gio-2.0 ≥ 2.54
 * glib-2.0 ≥ 2.54
 * gobject-2.0 ≥ 2.54

Licensing
---------

All code in this project is licensed under LGPL-2.1+. See COPYING for more details.

Bugs
----

Bug reports and patches should be filed in
[GitLab](https://gitlab.freedesktop.org/pwithnall/malcontent).
