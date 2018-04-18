# Process this file with autoconf to produce a configure script.
AC_INIT(h5utils, 1.13.1, stevenj@alum.mit.edu)
AM_INIT_AUTOMAKE([foreign silent-rules 1.6])
AM_SILENT_RULES(yes)
AC_CONFIG_SRCDIR([h5topng.c])
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

# Checks for header files.
AC_HEADER_STDC

AC_CHECK_LIB(m, sin)
AC_CHECK_FUNCS(snprintf)

MORE_H5UTILS=""
MORE_H5UTILS_MANS=""

###########################################################################

dnl override CFLAGS selection when debugging
if test "${enable_debug}" = "yes"; then
	CFLAGS="-g"
fi

dnl add gcc warnings, in debug/maintainer mode only
if test "$enable_debug" = yes || test "$USE_MAINTAINER_MODE" = yes; then
	if test $ac_cv_prog_gcc = yes; then
		CFLAGS="$CFLAGS -Wall -W -Wcast-qual -Wpointer-arith -Wcast-align -pedantic -Wno-long-long -Wshadow -Wbad-function-cast -Wwrite-strings -Wstrict-prototypes -Wredundant-decls -Wnested-externs" # -Wundef -Wconversion -Wmissing-prototypes -Wmissing-declarations
	fi
fi

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
	MORE_H5UTILS="h5topng\$(EXEEXT) $MORE_H5UTILS"
	H5TOPNG_MAN=doc/man/h5topng.1
fi

AC_SUBST(H5TOPNG_MAN)
AC_SUBST(PNG_LIBS)

###########################################################################

AC_CHECK_LIB(matheval, evaluator_get_variables, H5MATH=yes, H5MATH=no)

if test $H5MATH = yes; then
	MORE_H5UTILS="h5math\$(EXEEXT) $MORE_H5UTILS"
	MORE_H5UTILS_MANS="doc/man/h5math.1 $MORE_H5UTILS_MANS"
else
	AC_MSG_WARN([can't find libmatheval: won't be able to compile h5math])
fi

###########################################################################

# Only build h5fromh4 if we are using a version of HDF5 prior to 1.4, and
# thus don't have the superior h4toh5 utility.  Similarly for h5toh4.
AC_CHECK_PROG(H4TOH5, h4toh5, h4toh5)
AC_CHECK_PROG(H5TOH4, h5toh4, h5toh4)

AC_ARG_WITH(hdf4,
	[AC_HELP_STRING([--with-hdf4], [build hdf4 utils even if h4toh5 and h5toh4 are present])],
	ok=$withval, ok=maybe)
if test "x$ok" = xyes; then
	H4TOH5=""
	H5TOH4=""
elif test "x$ok" = xno; then
	H4TOH5="h4toh5"
	H5TOH4="h5toh4"
fi

HDF4=no
if test "x$H4TOH5" != xh4toh5 -o "x$H5TOH4" != xh5toh4; then
	AC_CHECK_LIB(jpeg, jpeg_start_compress,
		[AC_CHECK_LIB(df, DFSDgetdata,
			[H4_LIBS="-ldf -ljpeg"; HDF4=yes],
			[AC_MSG_WARN([can't find libdf (HDF4): won't be able to compile h5fromh4 or h4fromh5])], -ljpeg)],
		[AC_MSG_WARN([can't find libjpeg: won't be able to compile h5fromh4 or h4fromh5])])

	if test $HDF4 = yes; then
		if test "x$H4TOH5" != xh4toh5; then
		   MORE_H5UTILS="h5fromh4\$(EXEEXT) $MORE_H5UTILS"
		   MORE_H5UTILS_MANS="doc/man/h5fromh4.1 $MORE_H5UTILS_MANS"
		fi
		if test "x$H5TOH4" != xh5toh4; then
		   MORE_H5UTILS="h4fromh5\$(EXEEXT) $MORE_H5UTILS"
		   # MORE_H5UTILS_MANS="doc/man/h4fromh5.1 $MORE_H5UTILS_MANS"
		fi
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
	AC_CHECK_PROGS(OCTAVE_CONFIG, octave-config, echo)
	AC_MSG_CHECKING(where octave plugins go)
	OCT_INSTALL_DIR=`octave-config --oct-site-dir 2> /dev/null | grep '/'`
	if test -z "$OCT_INSTALL_DIR"; then
		OCT_INSTALL_DIR=`octave-config --print OCTFILEDIR 2> /dev/null | grep '/'`
	fi
	if test -z "$OCT_INSTALL_DIR"; then
		OCT_INSTALL_DIR=`echo "path" | $OCTAVE -q 2> /dev/null | grep "/oct/" | head -1`
	fi
	if test -z "$OCT_INSTALL_DIR"; then
		OCT_INSTALL_DIR=`echo "DEFAULT_LOADPATH" | $OCTAVE -q 2> /dev/null | tr ':' '\n' | grep "site/oct" | head -1`
        fi
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
	MORE_H5UTILS="h5tov5d\$(EXEEXT) $MORE_H5UTILS"
	MORE_H5UTILS_MANS="doc/man/h5tov5d.1 $MORE_H5UTILS_MANS"
fi
AC_SUBST(V5D_FILES)
AC_SUBST(V5D_INCLUDES)

###########################################################################

AC_CHECK_HEADERS([arpa/inet.h netinet/in.h stdint.h inttypes.h])
AC_CHECK_TYPES([uint16_t, uint32_t])

AC_MSG_CHECKING([for htons])
AC_TRY_LINK([#if defined(HAVE_ARPA_INET_H)
#include <arpa/inet.h>
#elif defined(HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif], [unsigned short i; htons(i);], [htons=yes
	AC_DEFINE([HAVE_HTONS],1,[Define if you have htons.])], htons=no)
AC_MSG_RESULT($htons)

AC_CHECK_SIZEOF(float)

AC_MSG_CHECKING([for htonl])
AC_TRY_LINK([#if defined(HAVE_ARPA_INET_H)
#include <arpa/inet.h>
#elif defined(HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif], [unsigned long i; htonl(i);], [htonl=yes
     AC_DEFINE([HAVE_HTONL],1,[Define if you have htonl.])], htonl=no)
AC_MSG_RESULT($htonl)

if test "x$htons" != xyes -o "x$htonl" != xyes; then
	AC_C_BIGENDIAN
fi

###########################################################################

# Store datadir (e.g. /usr/local/share) in DATADIR #define.

# Requires some hackery to actually get this value...
save_prefix=$prefix
test "x$prefix" = xNONE && prefix=$ac_default_prefix
eval datadir_val=$datadir
eval datadir_val=$datadir_val
prefix=$save_prefix

AC_DEFINE_UNQUOTED(DATADIR, "$datadir_val", [datadir installation prefix])
AC_SUBST(datadir_val)

###########################################################################
AC_SUBST(MORE_H5UTILS)
AC_SUBST(MORE_H5UTILS_MANS)
###########################################################################

AC_CONFIG_FILES([Makefile doc/man/h5topng.1])
AC_OUTPUT
