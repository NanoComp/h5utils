#ifndef WRITEPNG_H

#ifdef SINGLE_PRECISION
typedef float REAL;
#else
typedef double REAL;
#endif

typedef enum {
     GRAYSCALE,
     BLUE_WHITE_RED
} colormap_t;

void writepng(char *filename,
	      int nx, int ny, int transpose,
	      REAL skew, REAL scalex, REAL scaley,
	      REAL *data, int *mask,
	      REAL minrange, REAL maxrange,
	      int invert, colormap_t colormap);

void writepng_autorange(char *filename,
			int nx, int ny, int transpose,
			REAL skew, REAL scalex, REAL scaley,
			REAL *data, int *mask,
			int invert, colormap_t colormap);

void compute_outlinemask(int nx, int ny, REAL * data, int *mask,
			 REAL background_value);

#endif
