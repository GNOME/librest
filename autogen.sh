#!/bin/sh
gtkdocize --flavour no-tmpl || exit 1
autoreconf -v -i && ./configure $@
