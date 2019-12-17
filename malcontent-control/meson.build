application_id = 'org.freedesktop.MalcontentControl'

malcontent_control = executable('malcontent-control',
  [
    'application.c',
    'application.h',
    'main.c',
  ],
  dependencies: [
    dependency('gio-2.0', version: '>= 2.44'),
    dependency('glib-2.0', version: '>= 2.54.2'),
    dependency('gobject-2.0', version: '>= 2.54'),
    dependency('gtk+-3.0'),
    libmalcontent_dep,
  ],
  include_directories: root_inc,
  install: true,
)

# FIXME: Add icons and tests
#subdir('icons')
#subdir('tests')
