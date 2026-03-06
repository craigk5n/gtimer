core_base_sources = files(
  'db-manager.c',
  'task-object.c',
  'project-object.c',
  'task-list-model.c',
  'report-generator.c',
  'timer-utils.c',
  'timer-service.c'
)

core_sources = core_base_sources + files('idle-monitor.c')

core_lib = static_library('gtimer-core',
  core_sources,
  dependencies: [sqlite_dep, glib_dep, x11_dep, xss_dep],
  include_directories: include_directories('.')
)

core_dep = declare_dependency(
  link_with: core_lib,
  include_directories: include_directories('.')
)
