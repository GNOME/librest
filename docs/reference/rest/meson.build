gnome.gtkdoc('rest',
  src_dir: [
    include_directories('../../../rest'),
    include_directories('../../../rest-extras'),
  ],
  main_xml: 'rest-docs.xml',
  module_version: librest_api_version,
  dependencies: [
    librest_deps,
    librest_dep,
    librest_extras_dep,
    librest_extras_deps,
  ],
  scan_args: [
    '--rebuild-types',
  ],
  ignore_headers: [
    'flickr-proxy-private.h',
    'lastfm-proxy-private.h',
    'oauth-proxy-private.h',
    'oauth2-proxy-private.h',
    'rest-private.h',
    'rest-proxy-call-private.h',
    'sha1.h',
  ],
)
