test_sources = files(
  'test-main.c'
)

test_db_sources = files(
  'test-db.c'
)

test_exe = executable('test-gtimer',
  test_sources,
  dependencies: [core_dep, gtk_dep]
)

test_db_exe = executable('test-db',
  test_db_sources,
  dependencies: [core_dep, gtk_dep]
)

test_model_sources = files(
  'test-model.c'
)

test_model_exe = executable('test-model',
  test_model_sources,
  dependencies: [core_dep, gtk_dep]
)

test_utils_sources = files(
  'test-utils.c'
)

test_utils_exe = executable('test-utils',
  test_utils_sources,
  dependencies: [core_dep, gtk_dep]
)

test_service_sources = files(
  'test-service.c'
)

test_service_exe = executable('test-service',
  test_service_sources,
  dependencies: [core_dep, gtk_dep]
)

test_idle_init_sources = files(
  'test-idle-init.c'
)

test_idle_init_exe = executable('test-idle-init',
  test_idle_init_sources,
  dependencies: [core_dep, gtk_dep]
)

test_db_manager_sources = files(
  'test-db-manager.c'
)

test_db_manager_exe = executable('test-db-manager',
  test_db_manager_sources,
  dependencies: [core_dep, gtk_dep]
)

test_objects_exe = executable('test-objects',
  'test-objects.c',
  dependencies: [core_dep, gtk_dep]
)

test_report_exe = executable('test-report',
  'test-report.c',
  dependencies: [core_dep, gtk_dep]
)

test_cli_exe = executable('test-cli',
  'test-cli.c',
  dependencies: [core_dep, gtk_dep],
  c_args: ['-DGTIMER_BINARY="' + gtimer_exe.full_path() + '"']
)

# Mock Idle Monitor Test
mock_idle_monitor_sources = files(
  'mock-idle-monitor.c',
  'test-idle-logic.c',
  '../core/db-manager.c',
  '../core/task-object.c',
  '../core/project-object.c',
  '../core/task-list-model.c',
  '../core/report-generator.c',
  '../core/timer-utils.c',
  '../core/timer-service.c'
)

test_idle_logic_exe = executable('test-idle-logic',
  mock_idle_monitor_sources,
  dependencies: [sqlite_dep, glib_dep, gtk_dep], # Manual deps since we're not using core_dep
  c_args: ['-I' + meson.project_source_root() + '/core']
)

test('basic', test_exe)
test('db', test_db_exe)
test('db-manager', test_db_manager_exe)
test('model', test_model_exe)
test('utils', test_utils_exe)
test('service', test_service_exe)
test('idle-init', test_idle_init_exe)
test('objects', test_objects_exe)
test('report', test_report_exe)
test('cli', test_cli_exe)
test('idle-logic', test_idle_logic_exe)
