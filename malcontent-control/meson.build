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

desktop_file = i18n.merge_file('desktop-file',
  type: 'desktop',
  input: '@0@.desktop.in'.format(application_id),
  output: '@0@.desktop'.format(application_id),
  po_dir: join_paths(meson.source_root(), 'po'),
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications'),
)

desktop_file_validate = find_program('desktop-file-validate', required: false)
if desktop_file_validate.found()
  test(
    'validate-desktop',
    desktop_file_validate,
    args: [
      desktop_file.full_path(),
    ],
    suite: ['malcontent-control'],
  )
endif

# FIXME: Add icons and tests
#subdir('icons')
#subdir('tests')
