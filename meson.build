project('gtimer', 'c',
  version: '3.1.1',
  meson_version: '>= 0.61.0',
  default_options: [
    'warning_level=2',
    'c_std=gnu11',
  ]
)

gnome = import('gnome')
i18n = import('i18n')

# Dependencies
gtk_dep = dependency('gtk4')
adwaita_dep = dependency('libadwaita-1')
sqlite_dep = dependency('sqlite3')
glib_dep = dependency('glib-2.0')
x11_dep = dependency('x11', required: false)
xss_dep = dependency('xss', required: false)

# Configuration
conf_data = configuration_data()
if xss_dep.found()
  conf_data.set('HAVE_XSS', 1)
endif
conf_data.set_quoted('VERSION', meson.project_version())
conf_data.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf_data.set_quoted('LOCALEDIR', get_option('prefix') / get_option('localedir'))

configure_file(
  output: 'config.h',
  configuration: conf_data
)

add_project_arguments('-include', 'config.h', language: 'c')

# Subdirectories
subdir('core')
subdir('data')
subdir('src')
subdir('tests')
subdir('po')
