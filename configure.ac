AC_PREREQ(2.60)
AC_INIT([infinoted-plugin-replacer], [1.0], [armin@arbur.net])
AM_INIT_AUTOMAKE([-Wall dist-bzip2 foreign])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES])
AM_MAINTAINER_MODE
AC_CONFIG_SRCDIR([src/infinoted-plugin-replacer.c])
AC_CONFIG_HEADER([config.h])

AC_CANONICAL_HOST

AC_LANG(C)
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_AR
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

PKG_CHECK_MODULES([infinoted_plugin_replacer], [libinfinity-0.6 libinfinoted-plugin-manager-0.6 libinftext-0.6])

AC_CONFIG_FILES([
  Makefile
    src/Makefile
])

AC_OUTPUT

# vim:set et:
