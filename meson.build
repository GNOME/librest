project('librest', 'c',
  version: '0.10.3',
  license: 'LGPL2.1+',
  meson_version: '>= 0.56',
)

# Versioning
librest_module_version = '0.0.0'
librest_soversion = '0'
librest_api_version = '1.0'
librest_pkg_string = 'rest-@0@'.format(librest_api_version)

# Modules
gnome = import('gnome')
pkgconfig = import('pkgconfig')

# Paths
rest_datadir = join_paths(get_option('prefix'), get_option('datadir'))

# CA certificates
if get_option('ca_certificates')
  ca_certificates_path = get_option('ca_certificates_path')

  if ca_certificates_path == ''
    default_certificate_paths = [
      '/etc/ssl/ca-bundle.pem',
      '/etc/ssl/certs/ca-certificates.crt',
      '/etc/pki/tls/certs/ca-bundle.crt',
    ]

    foreach location : default_certificate_paths
      if file_exists(location)
        ca_certificates_path = location
        break
      endif
    endforeach
  endif

  if ca_certificates_path == ''
    error('Did not specify system CA list and could not find any in the default locations.')
  endif
endif

# Dependencies
if get_option('soup2')
  libsoup_name = 'libsoup-2.4'
  libsoup_req_version = '>= 2.42'
  libsoup_api_version = '2.4'
  add_project_arguments('-DWITH_SOUP_2', language: 'c')
else
  libsoup_name = 'libsoup-3.0'
  libsoup_req_version = '>= 2.99.2'
  libsoup_api_version = '3.0'
endif


glib_dep = dependency('glib-2.0', version: '>= 2.44')
gobject_dep = dependency('gobject-2.0', version: '>= 2.44')
libsoup_dep = dependency(libsoup_name, version: libsoup_req_version)
libjson_glib_dep = dependency('json-glib-1.0')
libxml_dep = dependency('libxml-2.0')

# config.h
conf = configuration_data()
conf.set_quoted('PACKAGE_NAME', meson.project_name())
conf.set_quoted('PACKAGE_STRING', '@0@ - @1@'.format(meson.project_name(), meson.project_version()))
conf.set_quoted('PACKAGE_VERSION', meson.project_version())
if get_option('ca_certificates')
  conf.set_quoted('REST_SYSTEM_CA_FILE', ca_certificates_path)
endif
config_h = configure_file(output: 'config.h', configuration: conf)
root_inc = include_directories('.')
config_dep = declare_dependency(
  sources: config_h,
  include_directories: root_inc,
)

# Subdirectories
subdir('rest')
subdir('rest-extras')
if get_option('tests')
  subdir('tests')
endif
if get_option('examples')
  subdir('examples')
endif
if get_option('gtk_doc')
  subdir('docs')
endif

# pkg-config
pkgconfig.generate(librest_lib,
  name: meson.project_name(),
  filebase: librest_pkg_string,
  description: 'RESTful web api query library',
  subdirs: librest_pkg_string,
  requires: [ glib_dep, libsoup_dep, libxml_dep, libjson_glib_dep ],
  variables: [
    'apiversion=@0@'.format(librest_api_version),
    'soupapiversion=@0@'.format(libsoup_api_version),
  ],
)

pkgconfig.generate(librest_extras_lib,
  name: meson.project_name(),
  filebase: librest_extras_pkg_string,
  description: 'RESTful web api query library',
  subdirs: librest_pkg_string,
  requires: [ glib_dep, libsoup_dep, libxml_dep],
  variables: [
    'apiversion=@0@'.format(librest_api_version),
    'soupapiversion=@0@'.format(libsoup_api_version),
  ],
)

summary({
    'prefix': get_option('prefix'),
    'libdir': get_option('prefix') / get_option('libdir'),
  },
  section: 'Directories',
)

summary({
    'Introspection': get_option('introspection'),
    'Vapi': get_option('vapi'),
    'Documentation': get_option('gtk_doc'),
    'Tests': get_option('tests'),
    'Examples': get_option('examples'),
    'Soup 2': get_option('soup2'),
  },
  section: 'Build',
  bool_yn: true,
)
