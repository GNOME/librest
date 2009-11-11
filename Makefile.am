SUBDIRS = rest rest-extras examples docs tests

%-$(API_VERSION).pc: %.pc
	$(AM_V_GEN)cp $< $@

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = rest-$(API_VERSION).pc rest-extras-$(API_VERSION).pc

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

ACLOCAL_AMFLAGS = -I .
