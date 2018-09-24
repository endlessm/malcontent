install_data('com.endlessm.ParentalControls.policy',
  install_dir: polkitpolicydir)

install_data('com.endlessm.ParentalControls.AppFilter.xml',
  install_dir: dbusinterfacesdir)
meson.add_install_script(meson_make_symlink,
  join_paths(dbusinterfacesdir, 'com.endlessm.ParentalControls.AppFilter.xml'),
  join_paths(accountsserviceinterfacesdir, 'com.endlessm.ParentalControls.AppFilter.xml'))