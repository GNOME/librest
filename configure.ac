m4_define([rest_major], [0])
m4_define([rest_minor], [8])
m4_define([rest_micro], [0])

m4_define([rest_version], [rest_major.rest_minor.rest_micro])


AC_PREREQ([2.63])
AC_INIT([rest], [rest_version],
        [],
        [rest],
        [http://git.gnome.org/browse/librest/])

AC_CONFIG_SRCDIR([rest/rest-proxy.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build])
AC_CONFIG_MACRO_DIR([build])

AM_INIT_AUTOMAKE([1.11 foreign -Wno-portability no-define dist-xz])

AM_SILENT_RULES([yes])

API_MAJOR=0
API_MINOR=7
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

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.24)
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

# gtkdocize greps for ^GTK_DOC_CHECK and parses it, so you need to have
# it on it's own line.
m4_ifdef([GTK_DOC_CHECK], [
GTK_DOC_CHECK([1.13], [--flavour no-tmpl])
])

GOBJECT_INTROSPECTION_CHECK([0.6.7])

AC_PATH_PROG([GLIB_MKENUMS],[glib-mkenums])

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

dnl Stolen from glib-networking - those guys rock
AC_MSG_CHECKING([location of system Certificate Authority list])
AC_ARG_WITH(ca-certificates,
            [AC_HELP_STRING([--with-ca-certificates=@<:@path@:>@],
                            [path to system Certificate Authority list])])
if test "$with_ca_certificates" = "no"; then
    AC_MSG_RESULT([disabled])
else
    if test -z "$with_ca_certificates"; then
        for f in /etc/pki/tls/certs/ca-bundle.crt \
                 /etc/ssl/certs/ca-certificates.crt \
                 /etc/ssl/ca-bundle.pem; do
            if test -f "$f"; then
                with_ca_certificates="$f"
            fi
        done
        if test -z "$with_ca_certificates"; then
            AC_MSG_ERROR([could not find. Use --with-ca-certificates=path to set, or --without-ca-certificates to disable])
        fi
    fi

    AC_MSG_RESULT($with_ca_certificates)
    AC_DEFINE_UNQUOTED(REST_SYSTEM_CA_FILE, ["$with_ca_certificates"], [The system TLS CA list])
fi

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
echo "                 ============="
echo ""
echo "                   prefix:   ${prefix}"
echo ""
echo "            Documentation:   ${enable_gtk_doc}"
echo "       Introspection data:   ${enable_introspection}"
echo ""
