librest_extras_pkg_string = 'rest-extras-@0@'.format(librest_api_version)

librest_extras_sources = [
  'flickr-proxy.c',
  'flickr-proxy-call.c',
  'lastfm-proxy.c',
  'lastfm-proxy-call.c',
  'youtube-proxy.c',
]

librest_extras_headers = [
  'flickr-proxy.h',
  'flickr-proxy-call.h',
  'lastfm-proxy.h',
  'lastfm-proxy-call.h',
  'youtube-proxy.h',
]

librest_extras_deps = [
  glib_dep,
  libsoup_dep,
  libxml_dep,
  librest_dep,
]

librest_extras_c_args = [
  '-DG_LOG_DOMAIN="Rest"',
]

librest_extras_lib = library(librest_extras_pkg_string,
  librest_extras_sources,
  dependencies: librest_extras_deps,
  c_args: librest_extras_c_args,
  include_directories: config_h_inc,
  version: librest_module_version,
  soversion: librest_soversion,
  install: true,
)

install_headers(librest_extras_headers,
  subdir: librest_pkg_string / 'rest-extras',
)

librest_extras_dep = declare_dependency(
  link_with: librest_extras_lib,
)

if get_option('introspection').enabled()
  librest_extras_gir = gnome.generate_gir(librest_extras_lib,
    sources: librest_extras_sources + librest_extras_headers,
    namespace: 'RestExtras',
    nsversion: librest_api_version,
    includes: [ 'GObject-2.0', 'Gio-2.0', 'Soup-@0@'.format(libsoup_api_version), librest_gir[0] ],
    extra_args: [ '--accept-unprefixed' ],
    install: true,
  )
endif

# Test suite
test_runner_c_args = [
  '-DBUILD_TESTS',
]

test_runner_bin = executable('test-runner',
  [ 'test-runner.c', librest_extras_sources ],
  dependencies: librest_extras_deps,
  c_args: test_runner_c_args,
  include_directories: config_h_inc,
)

test('test-runner', test_runner_bin,
  suite: 'rest-extras',
)