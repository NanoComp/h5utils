COLORMAPS = colormaps/autumn colormaps/bluered colormaps/bone		\
colormaps/colorcube colormaps/cool colormaps/copper colormaps/flag	\
colormaps/gray colormaps/green colormaps/hot colormaps/hsv		\
colormaps/jet colormaps/lines colormaps/pink colormaps/prism		\
colormaps/spring colormaps/summer colormaps/vga colormaps/winter	\
colormaps/yarg colormaps/yellow

EXTRA_DIST = h5read.cc copyright.h $(COLORMAPS)

noinst_PROGRAMS = h5fromitxt # quick hack, not really supported
bin_PROGRAMS = h5totxt h5fromtxt h5tovtk @MORE_H5UTILS@
EXTRA_PROGRAMS = h5topng h5tov5d h5fromh4

dist_man_MANS = h5totxt.1 h5fromtxt.1 h5tovtk.1 @MORE_H5UTILS_MANS@
nodist_man_MANS = @H5TOPNG_MAN@
EXTRA_MANS = h5topng.1.in h5tov5d.1 h5fromh4.1

COMMON_SRC = arrayh5.c arrayh5.h h5utils.c h5utils.h

h5totxt_SOURCES = h5totxt.c $(COMMON_SRC)
h5fromtxt_SOURCES = h5fromtxt.c $(COMMON_SRC)
h5fromitxt_SOURCES = h5fromitxt.c $(COMMON_SRC)
h5tovtk_SOURCES = h5tovtk.c $(COMMON_SRC)

h5topng_SOURCES = h5topng.c writepng.c writepng.h $(COMMON_SRC)
h5topng_LDADD = @PNG_LIBS@

h5tov5d_SOURCES = h5tov5d.c $(COMMON_SRC)
h5tov5d_CPPFLAGS = $(AM_CPPFLAGS) @V5D_INCLUDES@
h5tov5d_LDADD = @V5D_FILES@

h5fromh4_SOURCES = h5fromh4.c arrayh4.c arrayh4.h $(COMMON_SRC)
h5fromh4_LDADD = @H4_LIBS@

octdir = @OCT_INSTALL_DIR@
oct_DATA = @H5READ@

h5read.oct: h5read.cc arrayh5.h arrayh5.o
	mkoctfile $(DEFS) $(CPPFLAGS) h5read.cc arrayh5.o $(LDFLAGS) $(LIBS)

clean-hook:
	rm -f h5read.oct

nobase_dist_pkgdata_DATA = $(COLORMAPS)
