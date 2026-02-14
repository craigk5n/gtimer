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

test_cli_sources = files(
  'test-cli.c'
)

test_cli_exe = executable('test-cli',
  test_cli_sources,
  dependencies: [core_dep, gtk_dep]
)

test('basic', test_exe)
test('db', test_db_exe)
test('model', test_model_exe)
test('utils', test_utils_exe)
test('service', test_service_exe)
test('idle-init', test_idle_init_exe)
test('cli', test_cli_exe)
