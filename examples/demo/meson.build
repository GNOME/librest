demo_sources = [
  'demo-main.c',
  'demo-window.c',
  'demo-rest-page.c',
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

executable('librest-demo',
  demo_sources + demo_gresources,
  dependencies: demo_deps,
  install: true,
)
