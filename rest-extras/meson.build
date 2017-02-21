

rest_extras_sources = files([
  'flickr-proxy.c',
  'flickr-proxy-call.c',
  'lastfm-proxy.c',
  'lastfm-proxy-call.c',
])


librest_extras = shared_library(
  'rest_extras',
  rest_extras_sources,
  dependencies : librest_dep
)

librest_extras_dep = declare_dependency(
  link_with: librest_extras,
  dependencies: librest_dep,
)
