subdir('po')

demo_sources = [
  'demo-main.c',
  'demo-window.c',
  'demo-rest-page.c',
  'demo-table.c',
  'demo-table-row.c',
]

demo_deps = [
  dependency('libadwaita-1'),
  dependency('gtksourceview-5'),
  dependency('json-glib-1.0'),
  librest_dep,
]

gnome = import('gnome')
demo_gresources = gnome.compile_resources(
  'demo-resources',
  'org.gnome.RestDemo.gresource.xml',
  c_name: 'demo'
)

desktop_file = i18n.merge_file(
  input: 'org.gnome.RestDemo.desktop.in',
  output: 'org.gnome.RestDemo.desktop',
  type: 'desktop',
  po_dir: 'po',
  install: true,
  install_dir: get_option('datadir') / 'applications'
)

desktop_utils = find_program('desktop-file-validate', required: false)
if desktop_utils.found()
  test('Validate desktop file', desktop_utils,
    args: [desktop_file]
  )
endif


executable('librest-demo',
  demo_sources + demo_gresources,
  dependencies: demo_deps,
  install: true,
)
