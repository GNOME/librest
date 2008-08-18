#!/bin/sh
gtkdocize || exit 1
autoreconf -v -i && ./configure $@
