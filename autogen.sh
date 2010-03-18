#!/bin/sh
gtkdocize --flavour no-tmpl || exit 1
ACLOCAL="${ACLOCAL-aclocal} $ACLOCAL_FLAGS" autoreconf -v -i && ./configure $@
