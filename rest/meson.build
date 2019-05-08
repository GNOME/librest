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
  'rest-param.h',
  'rest-params.h',
  'rest-proxy.h',
  'rest-proxy-auth.h',
  'rest-proxy-call.h',
  'oauth-proxy.h',
  'oauth-proxy-call.h',
  'oauth2-proxy.h',
  'oauth2-proxy-call.h',
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

librest_lib = shared_library('rest-@0@'.format(librest_api_version),
  librest_sources,
  dependencies: librest_deps,
  c_args: librest_c_args,
  include_directories: config_h_inc,
  version: librest_api_version,
  install: true,
)

install_headers(librest_headers,
  subdir: librest_pkg_string / 'rest',
)

librest_gir = gnome.generate_gir(librest_lib,
  sources: [ librest_headers, librest_enums[1] ],
  namespace: 'Rest',
  nsversion: librest_api_version,
  includes: [ 'GObject-2.0', 'Gio-2.0', 'Soup-2.4' ],
  extra_args: [ '--accept-unprefixed' ],
  install: true,
)

librest_vapi = gnome.generate_vapi(librest_pkg_string,
  sources: librest_gir.get(0),
  packages: [ 'gobject-2.0', 'gio-2.0', 'libsoup-2.4', 'libxml-2.0' ],
  install: true,
)

librest_dep = declare_dependency(
  link_with: librest_lib,
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
