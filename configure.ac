AC_PREREQ(2.53)
AC_INIT(rest, 0.6)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)

m4_define([api_version], [0.6])
AC_SUBST([API_VERSION],[api_version])
AC_DEFINE_UNQUOTED(API_VERSION, [$API_VERSION], [API version])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_CC_C_O

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.18)
PKG_CHECK_MODULES(SOUP, libsoup-2.4)
PKG_CHECK_MODULES(XML, libxml-2.0)
PKG_CHECK_MODULES(GTHREAD, gthread-2.0)

AC_MSG_CHECKING([whether to use the GNOME environment])
AC_ARG_WITH([gnome],[AS_HELP_STRING([--without-gnome], [disable support for GNOME environment])],
            [], [with_gnome=yes])

AS_IF(
        [test "$with_gnome" = yes],
        [
        AC_MSG_RESULT([yes])
        AC_DEFINE([WITH_GNOME], 1, [Use GNOME])
        PKG_CHECK_MODULES(SOUP_GNOME, libsoup-gnome-2.4 >= 2.25.1)
        ],
        AC_MSG_RESULT([no])
)

GTK_DOC_CHECK(1.9)

localedir=${datadir}/locale
AC_SUBST(localedir)

AC_OUTPUT([
        Makefile
        rest/Makefile
        rest-extras/Makefile
        examples/Makefile
        docs/Makefile
        docs/reference/Makefile
        docs/reference/rest/Makefile
        tests/Makefile
        rest.pc
        rest-extras.pc
])
