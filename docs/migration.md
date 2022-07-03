Title: Migrating from librest 0.7 to 1.0
Slug: librest-migrating-0.7-to-1.0

# Migrating from librest 0.7 to 1.0

Application built against 0.7 should also work against 1.0. In order to make the migration easier just read this migration guide.

## Stop using the OAuth 1.0 proxy

The OAuth 1.0 standard is old and has several flaws therefore it shouldn't be used anymore. By today most providers should allow to authenticate with OAuth 2.0 and therefore i removed the `OAuthProxy` from `librest`.

## Building against libsoup3 by default

If your application still uses `libsoup2` then it should be updated according to the libsoup documentation. `librest` is now built by default aginst `libsoup3`.

## Stop using `RestAuth`

This object never really came into play for Basic authentication because most REST API endpoints allow an unauthenticated as well as an authenticated access to their resources. This means there never was an 401 return status code to re-access the resource with a Basic Auth. Therefore it got removed.

## Test with Demo application

There is now also a demo application available which should allow testing the capabilities of `librest` interactively.


