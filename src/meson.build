src_sources = files(
  'main.c',
  'gtimer-window.c',
  'report-window.c'
)

gtimer_exe = executable('gtimer',
  [src_sources, resources],
  dependencies: [gtk_dep, adwaita_dep, core_dep],
  install: true
)
