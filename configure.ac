# Process this file with autoconf to produce a configure script.
AC_INIT(h5utils, 1.8beta)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([h5topng.c])
AM_CONFIG_HEADER(config.h)

# Checks for programs.
AC_PROG_CC

# Checks for header files.
AC_HEADER_STDC

AC_CHECK_LIB(m, sqrt)

MORE_H5UTILS=""
MORE_H5UTILS_MANS=""

###########################################################################

H5TOPNG=yes
PNG_LIBS=""

AC_CHECK_LIB(z, inflate, ok=yes, ok=no)
if test "$ok" = "yes"; then
	LIBS="-lz $LIBS"

	AC_CHECK_LIB(png, png_create_write_struct, ok=yes, ok=no)
	if test "$ok" = "yes"; then
		PNG_LIBS="-lpng"
	else
		AC_MSG_WARN([can't find libpng: won't be able to compile h5topng])
		H5TOPNG=no
	fi
else
	AC_MSG_WARN([can't find libz: won't be able to compile h5topng])
	H5TOPNG=no
fi

if test $H5TOPNG = yes; then
	MORE_H5UTILS="h5topng $MORE_H5UTILS"
	H5TOPNG_MAN=h5topng.1
fi

AC_SUBST(H5TOPNG_MAN)
AC_SUBST(PNG_LIBS)

###########################################################################

# Only build h5fromh4 if we are using a version of HDF5 prior to 1.4, and
# thus don't have the superior h4toh5 utility.
AC_CHECK_PROG(H4TOH5, h4toh5, h4toh5)

AC_ARG_WITH(h5fromh4,
	[AC_HELP_STRING([--without-h5fromh4], [don't compile h5fromh4])],
	ok=$withval, ok=maybe)
if test "x$ok" = xyes; then
	H4TOH5=""
elif test "x$ok" = xno; then
	H4TOH5="h4toh5"
fi

H5FROMH4=no
if test "x$H4TOH5" != xh4toh5; then
	AC_CHECK_LIB(jpeg, jpeg_start_compress,
		[AC_CHECK_LIB(df, DFSDgetdata,
			[H4_LIBS="-ldf -ljpeg"; H5FROMH4=yes],
			[AC_MSG_WARN([can't find libdf (HDF4): won't be able to compile h5fromh4])], -ljpeg)],
		[AC_MSG_WARN([can't find libjpeg: won't be able to compile h5fromh4])])

	if test $H5FROMH4 = yes; then
		MORE_H5UTILS="h5fromh4 $MORE_H5UTILS"
		MORE_H5UTILS_MANS="h5fromh4.1 $MORE_H5UTILS_MANS"
	fi
fi
AC_CHECK_HEADERS(hdf.h hdf/hdf.h)

AC_SUBST(H4_LIBS)

###########################################################################

AC_CHECK_LIB(hdf5, H5Fopen, [LIBS="-lhdf5 $LIBS"],
	     [AC_MSG_ERROR([hdf5 libraries are required for compilation])])

###########################################################################

AC_ARG_WITH(octave, [AC_HELP_STRING([--without-octave], [don't compile h5read Octave plugin])], ok=$withval, ok=yes)

H5READ=""
OCT_INSTALL_DIR=""

if test "x$ok" = xyes; then
AC_CHECK_PROGS(MKOCTFILE, mkoctfile, echo)
if test "$MKOCTFILE" = "echo"; then
	AC_MSG_WARN([can't find mkoctfile: won't be able to compile h5read.oct])
else
	# try to find installation directory
	AC_CHECK_PROGS(OCTAVE, octave, echo)
	AC_MSG_CHECKING(where octave plugins go)
	OCT_INSTALL_DIR=`echo "DEFAULT_LOADPATH" | $OCTAVE -q | tr ':' '\n' | grep "site/oct" | head -1`
	if test -n "$OCT_INSTALL_DIR"; then
		AC_MSG_RESULT($OCT_INSTALL_DIR)
		H5READ=h5read.oct
	else
		AC_MSG_RESULT(unknown)
		AC_MSG_WARN([can't find where to install octave plugins: won't be able to compile h5read.oct])
	fi
fi
fi

AC_SUBST(H5READ)
AC_SUBST(OCT_INSTALL_DIR)

###########################################################################

AC_ARG_WITH(v5d, [AC_HELP_STRING([--with-v5d=<dir>], [use Vis5d in <dir> for h5tov5d])], ok=$withval, ok=yes)
H5TOV5D=no
V5D_FILES=""
V5D_INCLUDES=""
if test "$ok" = "yes"; then
	AC_CHECK_LIB(v5d, v5dCreate, V5D_FILES="-lv5d"; H5TOV5D=yes)
	AC_CHECK_HEADERS(vis5d/v5d.h)
	AC_CHECK_HEADER(vis5d+/v5d.h, [AC_DEFINE([HAVE_VIS5Dp_V5D_H], 1, [[Define if you have the <vis5d+/v5d.h> header file.]])])
elif test "$ok" != "no"; then
	AC_MSG_CHECKING([for Vis5d object files and headers])
	if test -r "$ok/src/v5d.o" -a -r "$ok/src/binio.o" -a -r "$ok/src/v5d.h" -a -r "$ok/src/binio.h"; then
		V5D_FILES="$ok/src/v5d.o $ok/src/binio.o"
		V5D_INCLUDES="-I$ok/src"
	elif test -r "$ok/v5d.o" -a -r "$ok/binio.o" -a -r "$ok/v5d.h" -a -r "$ok/binio.h"; then
		V5D_FILES="$ok/v5d.o $ok/binio.o"
		V5D_INCLUDES="-I$ok"
	fi
	if test -z "$V5D_FILES"; then
		AC_MSG_RESULT([not found])
		AC_MSG_ERROR([couldn't read Vis5D object files in $ok])
	else
		AC_MSG_RESULT([found])
	fi
	H5TOV5D=yes
fi
if test $H5TOV5D = yes; then
	MORE_H5UTILS="h5tov5d $MORE_H5UTILS"
	MORE_H5UTILS_MANS="h5tov5d.1 $MORE_H5UTILS_MANS"
fi
AC_SUBST(V5D_FILES)
AC_SUBST(V5D_INCLUDES)

###########################################################################

AC_CHECK_HEADERS([netinet/in.h])

AC_MSG_CHECKING([for htons])
AC_TRY_LINK([#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif], [unsigned short i; htons(i);], [htons=yes
	AC_DEFINE([HAVE_HTONS],1,[Define if you have htons.])], htons=no)
AC_MSG_RESULT($htons)

AC_CHECK_SIZEOF(unsigned long)
AC_CHECK_SIZEOF(float)

if test "x$ac_cv_sizeof_unsigned_long" = "x$ac_cv_sizeof_float"; then
	AC_MSG_CHECKING([for htonl])
	AC_TRY_LINK([#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif], [unsigned short i; htonl(i);], [htonl=yes
	     AC_DEFINE([HAVE_HTONL],1,[Define if you have htonl.])], htonl=no)
	AC_MSG_RESULT($htonl)
fi

if test "x$htons" != xyes -o "x$htonl" != xyes; then
	AC_C_BIGENDIAN
fi

###########################################################################

# Store datadir (e.g. /usr/local/share) in DATADIR #define.

# Requires some hackery to actually get this value...
save_prefix=$prefix
test "x$prefix" = xNONE && prefix=$ac_default_prefix
eval datadir_val=$datadir
prefix=$save_prefix

AC_DEFINE_UNQUOTED(DATADIR, "$datadir_val", [datadir installation prefix])
AC_SUBST(datadir_val)

###########################################################################
AC_SUBST(MORE_H5UTILS)
AC_SUBST(MORE_H5UTILS_MANS)
###########################################################################

AC_CONFIG_FILES([Makefile h5topng.1])
AC_OUTPUT