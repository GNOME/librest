SUBDIRS = rest examples docs tests

%-$(API_VERSION).pc: %.pc
	cp $< $@

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = rest-$(API_VERSION).pc

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc
