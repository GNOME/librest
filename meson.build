project('librest', 'c')

gnome = import('gnome')

librest_sources = files([
  'rest/oauth-proxy-call.c',
  'rest/oauth-proxy.c',
  'rest/oauth2-proxy-call.c',
  'rest/oauth2-proxy.c',
  'rest/rest-main.c',
  'rest/rest-param.c',
  'rest/rest-params.c',
  'rest/rest-proxy-auth.c',
  'rest/rest-proxy-call.c',
  'rest/rest-proxy.c',
  'rest/rest-xml-node.c',
  'rest/rest-xml-parser.c',
  'rest/sha1.c',
])

librest_headers = files([
  'rest/oauth-proxy-call.h',
  'rest/oauth-proxy.h',
  'rest/oauth2-proxy-call.h',
  'rest/oauth2-proxy.h',
  'rest/rest-enum-types.h',
  'rest/rest-param.h',
  'rest/rest-params.h',
  'rest/rest-proxy-auth.h',
  'rest/rest-proxy-call.h',
  'rest/rest-proxy.h',
  'rest/rest-xml-node.h',
  'rest/rest-xml-parser.h',
])



# generate rest/rest-marshal.c
rest_marshal = gnome.genmarshal(
  'rest-marshal',
  sources: files(['rest/rest-marshal.txt'])
)


cdata = configuration_data()
configure_file(
  input : 'config.h.in',
  output: 'config.h',
  configuration: cdata
)

soup_dep = dependency('libsoup-2.4', version: '>= 2.42')
xml_dep  = dependency('libxml-2.0')

rest_deps = [soup_dep, xml_dep]

librest = shared_library(
  'rest',
  [librest_sources, rest_marshal],
  dependencies: rest_deps,
  install: true
)
install_headers(librest_headers)

librest_dep = declare_dependency(
  link_with: librest,
  dependencies: rest_deps,
  include_directories: [include_directories('.')]
)

if get_option('extras')
  subdir('rest-extras')
endif

if get_option('gtk-doc')
  subdir('docs/reference/rest/')
endif

subdir('tests')
