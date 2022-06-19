example_names = [
  'test-raw',
  'test-xml',
  'dump-xml',
  'get-flickr-favorites',
  'lastfm-shout',
  'gitlab-oauth2-example',
]

example_deps = [
  glib_dep,
  libsoup_dep,
  librest_dep,
  librest_extras_dep,
]

foreach name : example_names
  example = executable(name,
    '@0@.c'.format(name),
    dependencies: example_deps,
    include_directories: config_h_inc,
  )
endforeach

subdir('demo')
