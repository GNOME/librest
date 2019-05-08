project('rest', 'c',
  version: '0.9.0',
  license: 'LGPL2.1+',
  meson_version: '>= 0.49',
)

# API version
librest_api_version = '1.0'
librest_pkg_string = 'rest-@0@'.format(librest_api_version)

# Modules
gnome = import('gnome')
pkgconfig = import('pkgconfig')

# Dependencies
glib_dep = dependency('glib-2.0', version: '>= 2.44')
gobject_dep = dependency('gobject-2.0', version: '>= 2.44')
libsoup_dep = dependency('libsoup-2.4', version: '>= 2.42')
libxml_dep = dependency('libxml-2.0')

# config.h
conf = configuration_data()
configure_file(output: 'config.h', configuration: conf)
config_h_inc = include_directories('.')

# Subdirectories
subdir('rest')
subdir('rest-extras')
subdir('tests')
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
  requires: [ glib_dep, libsoup_dep, libxml_dep, ],
  variables: [
    'apiversion=@0@'.format(librest_api_version),
  ],
)

pkgconfig.generate(librest_extras_lib,
  name: meson.project_name(),
  filebase: librest_extras_pkg_string,
  description: 'RESTful web api query library',
  subdirs: librest_pkg_string,
  requires: [ glib_dep, libsoup_dep, libxml_dep, ],
  variables: [
    'apiversion=@0@'.format(librest_api_version),
  ],
)
