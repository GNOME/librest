# librest

This library has been designed to make it easier to access web services that
claim to be "RESTful". A reasonable definition of what this means can be found
on [Wikipedia][1]. However a reasonable description is that a RESTful service
should have urls that represent remote objects which methods can then be
called on.

However it should be noted that the majority of services don't actually adhere
to this strict definition. Instead their RESTful end-point usually has an API
that is just simpler to use compared to other types of APIs they may support
(XML-RPC, for instance.). It is this kind of API that this library is
attempting to support.

It comprises of two parts: the first aims to make it easier to make requests
by providing a wrapper around [libsoup][2], the second aids with XML parsing by
wrapping [libxml2][3].


Nightly documentation: <https://gnome.pages.gitlab.gnome.org/librest/>

## Making requests

When a proxy is created for a url you are able to present a format string.
This format string is intended to represent the type of path for a remote
object, it is then possible to bind this format string with specific names of
objects to make this proxy concrete rather than abstract. We abstract the
parameters required for a particular call into it's own object which we can
then invoke both asynchronously and pseudo-asynchronously (by spinning the
main loop whilst waiting for an answer.) This has the advantage of allowing us
to support complex behaviour that depends on the parameters, for instance
signing a request: a requirement for many web services.

## Handling the result

Standard XML parsers require a significant amount of work to parse a piece of
XML. In terms of a SAX parser this involves setting up the functions before
hand, in terms of a DOM parser this means dealing with complexity of a DOM
tree. The XML parsing components of librest are designed to try and behave a
bit like the [BeautifulSoup parser][4]. It does this by parsing the XML into an
easily walkable tree were nodes have children for their descendents and
siblings for the nodes of the same type that share the same parent. This makes
it easy for instance to get a list of all the "photo" nodes in a document from
the root.

## Demo application

A small Postman clone is provided in `examples/demo` to test the requests librest
is making. Flatpak is the recommended way of building and installing it:

```
# Add Flathub and the gnome-nightly repo
flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak remote-add --user --if-not-exists gnome-nightly https://nightly.gnome.org/gnome-nightly.flatpakrepo

# Install the gnome-nightly Sdk and Platform runtime
flatpak install --user gnome-nightly org.gnome.Sdk org.gnome.Platform
```

**Inside** the `examples/demo` folder run:
```
flatpak-builder --user --install app org.gnome.Librest.json
```
Run the application
```
flatpak run org.gnome.RestDemo
```

[1]: https://en.wikipedia.org/wiki/Representational_State_Transfer
[2]: https://wiki.gnome.org/Projects/libsoup
[3]: https://gitlab.gnome.org/GNOME/libxml2/-/wikis/
[4]: https://www.crummy.com/software/BeautifulSoup/
