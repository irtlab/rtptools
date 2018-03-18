dnl Process this file with autoconf to produce a configure script.
AC_INIT([rtptools], [1.22])
AC_CONFIG_SRCDIR([./rtpplay.c])
AM_INIT_AUTOMAKE

AC_CANONICAL_HOST

ts_cv_darwin="no"
case "${host}" in
*-*-darwin*)
  AC_DEFINE(unix)
  AC_DEFINE(__darwin__)
  ts_cv_darwin="yes";;
esac

AM_CONDITIONAL(DARWIN, test "$ts_cv_darwin" = yes)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL

if test "x$GCC" = "xyes"; then
  CFLAGS="$CFLAGS -Wall"
fi

dnl Checks for html conversion tool from man page
AC_CHECK_PROG([FOUND_GROFF], [groff], [yes])
if test x"$FOUND_GROFF" == x"yes" ; then
  if $(groff -Thtml -mdoc rtpplay.1 > /dev/null 2>&1) ; then
    WORKS_GROFF="yes"
  fi
fi
AM_CONDITIONAL([HAVE_GROFF], [test x"$WORKS_GROFF" == x"yes"])

AC_CHECK_PROG([FOUND_MANDOC], [mandoc], [yes])
if test x"$FOUND_MANDOC" == x"yes" ; then
  if $(mandoc -Thtml rtpplay.1 > /dev/null 2>&1)  ; then
    WORKS_MANDOC="yes"
  fi
fi
AM_CONDITIONAL([HAVE_MANDOC], [test x"$WORKS_MANDOC" == x"yes"])

AM_CONDITIONAL([FOUND_GEN_HTML], [test x"$WORKS_GROFF" == x"yes"] || 
  [test x"$WORKS_MANDOC" == x"yes"])

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(search.h sys/time.h unistd.h stdint.h)
AC_CHECK_HEADER([rpcsvc/ypclnt.h],
                [AC_DEFINE([HAVE_YP],[1],[Define to 1 if you have rpcsvc/ypclnt.h])])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_BIGENDIAN
AC_TYPE_UID_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_HEADER_TIME
AC_STRUCT_TM

dnl Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(gethostbyname gettimeofday mktime select \
        strdup strerror strstr strtol uname srand48)

dnl Checks for libraries.

dnl Substitute for AC_CHECK_FUNC; check for function normally, then in
dnl library

AC_DEFUN([CU_CHECK_LIB],
[AC_CHECK_FUNCS($2, , AC_CHECK_LIB($1, $2))])

CU_CHECK_LIB(nsl, gethostbyname)
CU_CHECK_LIB(nsl, yp_match)
CU_CHECK_LIB(socket, socket)

AC_OUTPUT(Makefile)
