m4_define([api_major], [0])
m4_define([api_minor], [7])
m4_define([api_micro], [10])

m4_define([rest_version], [api_major.api_minor.api_micro])


AC_PREREQ([2.63])
AC_INIT([rest], [rest_version],
        [],
        [rest],
        [http://meego.gitorious.org/meego-middleware/librest])

AC_CONFIG_SRCDIR([rest/rest-proxy.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build])
AC_CONFIG_MACRO_DIR([build])

AM_INIT_AUTOMAKE([1.11 foreign -Wno-portability no-define])

AM_SILENT_RULES([yes])

API_MAJOR=api_major
API_MINOR=api_minor
AC_SUBST([API_VERSION],[$API_MAJOR.$API_MINOR])
AC_SUBST([API_VERSION_AM],[$API_MAJOR\_$API_MINOR])
AC_DEFINE_UNQUOTED(API_VERSION, [$API_VERSION], [API version])

AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_INSTALL
AC_ISC_POSIX
AC_HEADER_STDC

AM_PROG_CC_C_O

# require libtool >= 2.2
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

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

GTK_DOC_CHECK([1.13], [--flavour no-tmpl])
GOBJECT_INTROSPECTION_CHECK([0.6.7])

localedir=${datadir}/locale
AC_SUBST(localedir)

dnl === Coverage report =======================================================
AC_PATH_PROG([GCOV], [lcov], [enable_gcov=no])

AC_MSG_CHECKING([whether to build with gcov testing])

AC_ARG_ENABLE([gcov],
              [AS_HELP_STRING([--enable-gcov],
                              [Whether to enable coverage testing (requires gcc
and lcov)])],
              [],
              [enable_gcov=no])

AS_IF([test "x$enable_gcov" = "xyes" && test "x$GCC" = "xyes"],
      [
        AS_IF([test "x$enable_gtk_doc" = "xyes"],
              [AC_MSG_WARN([gtk-doc is enabled, this may go horribly wrong])],
              [AC_MSG_RESULT([yes])]
        )

        GCOV_CFLAGS="-g -O0 -fprofile-arcs -ftest-coverage"
        GCOV_LDFLAGS="-lgcov"
      ],
      [AC_MSG_RESULT([no])]
)

AM_CONDITIONAL([GCOV_ENABLED], [test "x$enable_gcov" = "xyes"])
AC_SUBST([GCOV_CFLAGS])
AC_SUBST([GCOV_LDFLAGS])

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

echo ""
echo "                 LibRest $VERSION"
echo "                 ================"
echo ""
echo "                   prefix:   ${prefix}"
echo ""
echo "            Documentation:   ${enable_gtk_doc}"
echo "       Introspection data:   ${enable_introspection}"
echo ""
