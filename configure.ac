#	This file is an input file used by the GNU "autoconf" program to
#	generate the file "configure", which is run during installation
#	to configure the system for the local environment.

AC_INIT(regions, 1.0, saord@cfa.harvard.edu)
AC_CONFIG_SRCDIR([./regions.h])
AC_CANONICAL_HOST
AC_PROG_CC
AC_PROG_RANLIB
m4_include([m4/ac_prog_ar.m4])
AC_PROG_AR
AC_PROG_AWK
AC_CHECK_FUNCS(setenv)
AC_CHECK_SIZEOF([void *])
case $ac_cv_sizeof_void_p in
    8) LSUFFIX="64";;
esac

AC_EXEEXT
if test x"${EXEEXT}" = "xno"; then
  EXEEXT=""
fi
# check for external cfitsio libraries
havelib=0
AC_MSG_CHECKING(for cfitsio)
AC_ARG_WITH(cfitsio,
    [--with-cfitsio=<path> cfitsio top-level directory], havelib=1, havelib=0)
if test x"$havelib" = x1 ; then
  CFITSIOBASE=$withval
  # make sure essential libraries are present
  if test   -r $CFITSIOBASE/lib$LSUFFIX/libcfitsio.a -o \
            -r $CFITSIOBASE/lib$LSUFFIX/libcfitsio.so -o \
	    -r $CFITSIOBASE/lib$LSUFFIX/libcfitsio.dylib; then
      CFITSIOINC=$CFITSIOBASE/include
      CFITSIOLIB=$CFITSIOBASE/lib$LSUFFIX
      AC_MSG_RESULT(yes ($CFITSIOBASE))
  elif test -r $CFITSIOBASE/lib/libcfitsio.a -o \
	    -r $CFITSIOBASE/lib/libcfitsio.so -o \
	    -r $CFITSIOBASE/lib/libcfitsio.dylib; then
      CFITSIOINC=$CFITSIOBASE/include
      CFITSIOLIB=$CFITSIOBASE/lib
      AC_MSG_RESULT(yes ($CFITSIOBASE))
  elif test -r $CFITSIOBASE/libcfitsio.a -o \
	    -r $CFITSIOBASE/libcfitsio.so -o \
	    -r $CFITSIOBASE/libcfitsio.dylib; then
      CFITSIOINC=$CFITSIOBASE
      CFITSIOLIB=$CFITSIOBASE
      AC_MSG_RESULT(yes ($CFITSIOBASE))
  else
    AC_MSG_ERROR([cannot find CFITSIO library files])
  fi
  if test x$CFITSIOINC != x; then
    EXTRA_INCS="$EXTRA_INCS -I$CFITSIOINC"
  fi
  if test x$CFITSIOLIB != x; then
    EXTRA_LIBS="$EXTRA_LIBS -L$CFITSIOLIB -lcfitsio -lcurl"
  fi
fi
if test x"$havelib" = x0 ; then
  AC_CHECK_LIB(cfitsio, ffopentest, havelib=1, havelib=0, -lm)
  if test x"$havelib" = x1 ; then
    EXTRA_LIBS="$EXTRA_LIBS -lcfitsio -lcurl"
  else
    AC_MSG_ERROR([--with-cfitsio=<path> is required to specify the CFITSIO library])
  fi
fi

AC_CHECK_LIB(dl, dlopen, have_dl=yes, have_dl=no)
if test "$have_dl" = yes; then
  dofpic="yes"
  HAVE_DL="-DHAVE_DL=1"
  EXTRA_LIBS="$EXTRA_LIBS -ldl"  
  if test `$CC -v -rdynamic 2>&1 | grep -c unrecognized` = "0" ; then
    LDFLAGS="$LDFLAGS -rdynamic"
  fi
fi
AC_SUBST(HAVE_DL)

AC_MSG_CHECKING([${host_os}-specific configuration])
case $host_os in
    *cygwin*|*Cygwin* )
        CFLAGS="$CFLAGS -enable-auto-import"
        AC_MSG_RESULT(Cygwin configuration)
        ;;
    *mingw32*|*Mingw32*)
        CFLAGS="$CFLAGS -mconsole -D_WSTRING_DEFINED=1"
	EXTRA_LIBS="$EXTRA_LIBS -lwsock32"
        AC_MSG_RESULT(MinGW configuration)
        ;;
    *solaris*|*sunos*)
	EXTRA_LIBS="$EXTRA_LIBS -lsocket -lnsl"
        AC_MSG_RESULT(SunOS configuration)
        ;;
    *)  AC_MSG_RESULT(none)
        ;;
esac

AC_MSG_CHECKING([compiler-specific configuration])
case $CC in
    *gcc*)
        if test x"$dofpic" = x"yes" ; then
          CFLAGS="$CFLAGS -fPIC"
          AC_MSG_RESULT(adding -fPIC to gcc)
        fi
        ;;
    *)  AC_MSG_RESULT(none)
        ;;
esac

AC_MSG_CHECKING(for regions compiler)
AC_ARG_WITH(regions-cc, [ --with-regions-cc=CC  compiler],
	REGIONS_CC=\\\"$withval\\\", REGIONS_CC=NULL)
AC_MSG_RESULT($REGIONS_CC)
AC_SUBST(REGIONS_CC)


AC_MSG_CHECKING(for regions cflags)
AC_ARG_WITH(regions-cflags, [ --with-regions-cflags=CFLAGS  compiler flags],
	REGIONS_CFLAGS=\\\"$withval\\\", REGIONS_CFLAGS=\\\"-O2\\\")
AC_MSG_RESULT($REGIONS_CFLAGS)
AC_SUBST(REGIONS_CFLAGS)

AC_SUBST(EXTRA_INCS)
AC_SUBST(EXTRA_LIBS)

AC_CONFIG_FILES(Makefile)
AC_CONFIG_FILES(regions.pc)

AC_OUTPUT
