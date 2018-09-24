project('eos-parental-controls', 'c',
  version : '0.1.0',
  meson_version : '>= 0.47.0',
  license: 'LGPLv2.1+',
  default_options : [
    'buildtype=debugoptimized',
    'warning_level=2',
    'c_std=gnu11',
  ]
)

i18n = import('i18n')
pkgconfig = import('pkgconfig')

meson_make_symlink = join_paths(meson.source_root(), 'tools', 'meson-make-symlink.sh')
po_dir = join_paths(meson.source_root(), 'po')

prefix = get_option('prefix')
datadir = join_paths(prefix, get_option('datadir'))

# FIXME: This isn’t exposed in accountsservice.pc
accountsserviceinterfacesdir = join_paths(datadir, 'accountsservice', 'interfaces')

dbus = dependency('dbus-1')
dbusinterfacesdir = dbus.get_pkgconfig_variable('interfaces_dir',
  define_variable: ['datadir', datadir])

polkit_gobject = dependency('polkit-gobject-1')
polkitpolicydir = polkit_gobject.get_pkgconfig_variable('policydir',
  define_variable: ['prefix', prefix])

subdir('accounts-service')