resources = gnome.compile_resources('gtimer-resources',
  'gtimer.gresource.xml',
  source_dir: '.',
  c_name: 'gtimer'
)

install_data('icons/hicolor/scalable/apps/us.k5n.GTimer.svg',
  install_dir: get_option('datadir') / 'icons/hicolor/scalable/apps'
)

install_data('icons/hicolor/symbolic/apps/us.k5n.GTimer-symbolic.svg',
  install_dir: get_option('datadir') / 'icons/hicolor/symbolic/apps'
)

install_data('us.k5n.GTimer.desktop',
  install_dir: get_option('datadir') / 'applications'
)

install_data('us.k5n.GTimer.metainfo.xml',
  install_dir: get_option('datadir') / 'metainfo'
)

install_data('us.k5n.GTimer.gschema.xml',
  install_dir: get_option('datadir') / 'glib-2.0' / 'schemas'
)

# Compile the schema in the install prefix after installation so the installed
# app finds 'gschemas.compiled'. Without this, only the .xml is installed and
# the GUI aborts at startup (see issues #9 and #10). For a standard --prefix=/usr
# this is all that is needed; for a non-standard prefix the schema dir must also
# be on the runtime search path (XDG_DATA_DIRS or GSETTINGS_SCHEMA_DIR) -- see
# the "Running" section of README.md.
gnome.post_install(glib_compile_schemas: true)

# Compile the schema into the build directory too, so the app can run uninstalled
# via GSETTINGS_SCHEMA_DIR=build/data (see README.md).
gnome.compile_schemas(
  build_by_default: true,
  depend_files: 'us.k5n.GTimer.gschema.xml'
)

compile_schemas = find_program('glib-compile-schemas', required: false)
if compile_schemas.found()
  test('Validate schema', compile_schemas,
    args: ['--strict', '--dry-run', meson.current_source_dir()]
  )
endif
