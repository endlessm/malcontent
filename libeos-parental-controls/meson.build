libeos_parental_controls_api_version = '0'
libeos_parental_controls_api_name = 'eos-parental-controls-' + libeos_parental_controls_api_version
libeos_parental_controls_sources = [
  'app-filter.c',
]
libeos_parental_controls_headers = [
  'app-filter.h',
]

libeos_parental_controls_public_deps = [
  dependency('gio-2.0', version: '>= 2.44'),
  dependency('glib-2.0', version: '>= 2.54.2'),
  dependency('gobject-2.0', version: '>= 2.54'),
]

# FIXME: Would be good to use subdir here: https://github.com/mesonbuild/meson/issues/2969
libeos_parental_controls_include_subdir = join_paths(libeos_parental_controls_api_name, 'libeos-parental-controls')

libeos_parental_controls = library(libeos_parental_controls_api_name,
  libeos_parental_controls_sources + libeos_parental_controls_headers,
  dependencies: libeos_parental_controls_public_deps,
  include_directories: root_inc,
  install: true,
  version: meson.project_version(),
  soversion: libeos_parental_controls_api_version,
)
libeos_parental_controls_dep = declare_dependency(
  link_with: libeos_parental_controls,
  include_directories: root_inc,
)

# Public library bits.
install_headers(libeos_parental_controls_headers,
  subdir: libeos_parental_controls_include_subdir,
)

pkgconfig.generate(
  libraries: [ libeos_parental_controls ],
  subdirs: libeos_parental_controls_api_name,
  version: meson.project_version(),
  name: 'libeos-parental-controls',
  filebase: libeos_parental_controls_api_name,
  description: 'Library providing access to parental control settings.',
  requires: libeos_parental_controls_public_deps,
)

gnome.generate_gir(libeos_parental_controls,
  sources: libeos_parental_controls_sources + libeos_parental_controls_headers,
  nsversion: libeos_parental_controls_api_version,
  namespace: 'EosParentalControls',
  symbol_prefix: 'epc_',
  identifier_prefix: 'Epc',
  export_packages: 'libeos-parental-controls',
  includes: ['GObject-2.0', 'Gio-2.0'],
  install: true,
)