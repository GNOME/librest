librest_enums = gnome.mkenums_simple('rest-enum-types',
  sources: [ 'rest-proxy.h', 'rest-proxy-call.h' ],
  install_header: true,
  install_dir: get_option('includedir') / librest_pkg_string / 'rest',
)

librest_marshal = gnome.genmarshal('rest-marshal',
  sources: 'rest-marshal.txt',
)

librest_sources = [
  'rest-param.c',
  'rest-params.c',
  'rest-proxy.c',
  'rest-proxy-auth.c',
  'rest-proxy-call.c',
  'rest-xml-node.c',
  'rest-xml-parser.c',
  'rest-main.c',
  'oauth-proxy.c',
  'oauth-proxy-call.c',
  'oauth2-proxy.c',
  'oauth2-proxy-call.c',
  'sha1.c',

  librest_enums,
  librest_marshal,
]

librest_headers = [
  'oauth-proxy-call.h',
  'oauth-proxy.h',
  'oauth2-proxy-call.h',
  'oauth2-proxy.h',
  'rest-param.h',
  'rest-params.h',
  'rest-proxy-auth.h',
  'rest-proxy-call.h',
  'rest-proxy.h',
  'rest-xml-node.h',
  'rest-xml-parser.h',
]

librest_deps = [
  glib_dep,
  libsoup_dep,
  libxml_dep,
]

librest_c_args = [
  '-DG_LOG_DOMAIN="Rest"',
]

librest_lib = library('rest-@0@'.format(librest_api_version),
  librest_sources,
  dependencies: librest_deps,
  c_args: librest_c_args,
  include_directories: config_h_inc,
  version: librest_module_version,
  soversion: librest_soversion,
  install: true,
)

install_headers(librest_headers,
  subdir: librest_pkg_string / 'rest',
)

rest_dep_sources = []

# GObject Introspection
if get_option('introspection').enabled()
  librest_gir_extra_args = [
    '--accept-unprefixed',
    '--c-include=rest/rest-enum-types.h',
  ]

  foreach header : librest_headers
    librest_gir_extra_args += '--c-include=rest/@0@'.format(header)
  endforeach

  librest_gir = gnome.generate_gir(librest_lib,
    sources: [ librest_headers, librest_enums[1] ],
    namespace: 'Rest',
    nsversion: librest_api_version,
    includes: [ 'GObject-2.0', 'Gio-2.0', 'Soup-@0@'.format(libsoup_api_version) ],
    extra_args: librest_gir_extra_args,
    install: true,
  )

  rest_dep_sources += librest_gir
endif

librest_dep = declare_dependency(
  sources: rest_dep_sources,
  link_with: librest_lib,
  include_directories: include_directories('..'),
  dependencies: librest_deps,
)

# Test suite
test_runner_c_args = [
  '-DBUILD_TESTS',
]

test_runner_bin = executable('test-runner',
  [ 'test-runner.c', librest_sources ],
  dependencies: librest_deps,
  c_args: test_runner_c_args,
  include_directories: config_h_inc,
)

test('test-runner', test_runner_bin,
  suite: 'rest',
)
