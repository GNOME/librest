librest_enums = gnome.mkenums_simple('rest-enum-types',
  sources: [ 'rest-proxy.h', 'rest-proxy-call.h' ],
  install_header: true,
  install_dir: get_option('prefix') / get_option('includedir') / librest_pkg_string / 'rest',
)

librest_marshal = gnome.genmarshal('rest-marshal',
  sources: 'rest-marshal.txt',
)

librest_sources = [
  'rest-param.c',
  'rest-params.c',
  'rest-proxy.c',
  'rest-proxy-call.c',
  'rest-proxy-auth.c',
  'rest-xml-node.c',
  'rest-xml-parser.c',
  'rest-main.c',
  'sha1.c',

  'rest-oauth2-proxy.c',
  'rest-oauth2-proxy-call.c',
  'rest-pkce-code-challenge.c',
  'rest-utils.c',

  librest_enums[0],
  librest_marshal[0],
]

librest_headers = [
  'rest-param.h',
  'rest-params.h',
  'rest-proxy-call.h',
  'rest-proxy.h',
  'rest-proxy-auth.h',
  'rest-xml-node.h',
  'rest-xml-parser.h',

  'rest-oauth2-proxy.h',
  'rest-oauth2-proxy-call.h',
  'rest-pkce-code-challenge.h',
  'rest-utils.h',
  'rest.h',
]

librest_headers_public = librest_headers + [
  librest_enums[1],
]

librest_headers_private = [
  librest_marshal[1],
]

install_headers(librest_headers,
  subdir: librest_pkg_string / 'rest',
)

librest_deps = [
  glib_dep,
  libsoup_dep,
  libjson_glib_dep,
  libxml_dep,
  config_dep,
]

librest_c_args = [
  '-DG_LOG_DOMAIN="Rest"',
]

librest_lib = library('rest-@0@'.format(librest_api_version),
  librest_sources,
  librest_headers_public,
  librest_headers_private,
  dependencies: librest_deps,
  c_args: librest_c_args,
  include_directories: [root_inc, include_directories('.')],
  version: librest_module_version,
  soversion: librest_soversion,
  install: true,
)

# GObject Introspection
if get_option('vapi') and not get_option('introspection')
  warning('Vapi generation needs "introspection" option in order to generate the .vapi file')
endif

if get_option('introspection')
  librest_gir_extra_args = [
    '--accept-unprefixed',
  ]

  librest_gir = gnome.generate_gir(librest_lib,
    sources: librest_sources + librest_headers_public,
    nsversion: librest_api_version,
    namespace: 'Rest',
    symbol_prefix: 'rest',
    identifier_prefix: 'Rest',
    header: 'rest/rest.h',
    export_packages: librest_pkg_string,
    includes: [ 'GObject-2.0', 'Gio-2.0', 'Soup-@0@'.format(libsoup_api_version) ],
    extra_args: librest_gir_extra_args,
    install: true,
  )

  if get_option('vapi')
    librest_vapi = gnome.generate_vapi(librest_pkg_string,
      sources: librest_gir [0],
      packages: [ 'glib-2.0', 'libsoup-@0@'.format(libsoup_api_version) ],
      metadata_dirs : meson.current_source_dir(),
      install: true,
    )
  endif
endif

librest_dep = declare_dependency(
  link_with: librest_lib,
  include_directories: [root_inc, include_directories('.')],
  sources: [librest_enums, librest_marshal],
  dependencies: librest_deps,
)

meson.override_dependency('rest-1.0', librest_dep)