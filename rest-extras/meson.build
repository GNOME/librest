librest_extras_pkg_string = 'rest-extras-@0@'.format(librest_api_version)

librest_extras_sources = [
  'flickr-proxy.c',
  'flickr-proxy-call.c',
  'lastfm-proxy.c',
  'lastfm-proxy-call.c',
  'youtube-proxy.c',
]

librest_extras_headers = [
  'rest-extras.h',
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
  config_dep,
]

librest_extras_c_args = [
  '-DG_LOG_DOMAIN="Rest"',
]

librest_extras_lib = library(librest_extras_pkg_string,
  librest_extras_sources,
  dependencies: librest_extras_deps,
  c_args: librest_extras_c_args,
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

if get_option('vapi') and not get_option('introspection')
  warning('Vapi generation needs "introspection" option in order to generate the .vapi file')
endif

if get_option('introspection')
  librest_extras_gir = gnome.generate_gir(librest_extras_lib,
    sources: librest_extras_sources + librest_extras_headers,
    namespace: 'RestExtras',
    symbol_prefix: 'rest_extras',
    identifier_prefix: 'RestExtras',
    header: 'rest-extras/rest-extras.h',
    nsversion: librest_api_version,
    includes: [ 'GObject-2.0', 'Gio-2.0', 'Soup-@0@'.format(libsoup_api_version), librest_gir[0] ],
    extra_args: [ '--accept-unprefixed' ],
    dependencies: librest_extras_deps,
    install: true,
  )
  
  if get_option('vapi')
    librest_extras_vapi = gnome.generate_vapi(librest_extras_pkg_string,
      sources: librest_extras_gir [0],
      packages: [ 'glib-2.0', 'libsoup-@0@'.format(libsoup_api_version), librest_vapi ],
      install: true,
    )
  endif
endif

# Test suite
test_runner_c_args = [
  '-DBUILD_TESTS',
]

test_runner_bin = executable('test-runner',
  [ 'test-runner.c', librest_extras_sources ],
  dependencies: librest_extras_deps,
  c_args: test_runner_c_args,
)

test('test-runner', test_runner_bin,
  suite: 'rest-extras',
)
