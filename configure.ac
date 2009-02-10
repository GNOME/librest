AC_PREREQ(2.53)
AC_INIT(rest, 0.1)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)

AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_CC_C_O

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.14)
PKG_CHECK_MODULES(SOUP, libsoup-2.4)
PKG_CHECK_MODULES(XML, libxml-2.0)
PKG_CHECK_MODULES(GTHREAD, gthread-2.0)

GTK_DOC_CHECK(1.9)

localedir=${datadir}/locale
AC_SUBST(localedir)

AC_OUTPUT([
        Makefile
        rest/Makefile
        examples/Makefile
        docs/Makefile
        docs/reference/Makefile
        docs/reference/rest/Makefile
        tests/Makefile
        rest.pc
])

