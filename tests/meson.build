test_suites = {
  'rest': [
    'proxy',
    'proxy-continuous',
    'threaded',
    'xml',
    'custom-serialize',
    'oauth2',
  ],
  'rest-extras': [
    'flickr',
    'lastfm',
  ],
}

test_deps = [
  glib_dep,
  libsoup_dep,
  librest_dep,
  librest_extras_dep,
]

foreach suite, test_names : test_suites
  foreach name : test_names
    test_bin = executable(name,
      ['@0@.c'.format(name), 'helper/test-server.c'],
      dependencies: test_deps,
      include_directories: config_h_inc,
    )

    test(name, test_bin,
      suite: suite,
    )
  endforeach
endforeach
