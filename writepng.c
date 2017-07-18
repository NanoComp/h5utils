/* Copyright (c) 1999-2017 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <png.h>

#include "writepng.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define PIN(min, x, max) MIN(MAX(min, x), max)

/* convert a value val in [0,1] to a color from the colormap */
static void cmap_lookup(REAL val, colormap_t cmap,
			float *r, float *g, float *b, float *a)
{
     double w;
     int i = val * (cmap.n - 1);
     if (i > cmap.n - 2) i = cmap.n - 2;
     if (i < 0) i = 0;
     w = val * (cmap.n - 1) - i;
     *r = cmap.rgba[i].r * (1 - w) + cmap.rgba[i+1].r * w;
     *g = cmap.rgba[i].g * (1 - w) + cmap.rgba[i+1].g * w;
     *b = cmap.rgba[i].b * (1 - w) + cmap.rgba[i+1].b * w;
     *a = cmap.rgba[i].a * (1 - w) + cmap.rgba[i+1].a * w;
}

static void convert_row(int png_width, int data_width,
			REAL scaley, REAL offsety,
			REAL *datarow, REAL *datarow2, REAL weightrow,
			int stride, REAL *maskrow, REAL *maskrow2,
			REAL mask_thresh, REAL *mask_prev, int init_mask_prev,
			png_byte mask_byte,
			int mny, int mstride,
			int overlay, REAL *olayrow, REAL *olayrow2,
			colormap_t olay_cmap, REAL olaymin, REAL olaymax,
			int ony, int ostride,
			colormap_t cmap,
			REAL minrange, REAL maxrange, REAL scale,
			png_byte * row_pointer, int eight_bit)
{
     int i;

     for (i = 0; i < png_width; ++i) {
	  REAL y = i * scaley + offsety;
	  int n = PIN(0, (int) (y + 0.5), data_width-1);
	  double delta = y - n;
	  REAL val, maskval = 0.0, olayval = olaymin;

	  if (n < 0 || n > data_width) {
	       if (eight_bit)
		    row_pointer[i] = 255;
	       else
		    row_pointer[3*i]
			 = row_pointer[3*i + 1]
			 = row_pointer[3*i + 2] = mask_byte;
	       continue;
	  }

	  if (delta == 0.0) {
	       val = (datarow[n * stride] * weightrow +
		      datarow2[n * stride] * (1 - weightrow));
	       if (maskrow != NULL) {
		    maskval = (maskrow[(n%mny) * mstride] * weightrow +
			       maskrow2[(n%mny) * mstride] * (1 - weightrow));
	       }
	       if (overlay)
		    olayval = (olayrow[(n%ony) * ostride] * weightrow +
			       olayrow2[(n%ony) * ostride] * (1 - weightrow));
	  }
	  else {
	       int n2 = PIN(0, n + (delta < 0.0 ? -1 : 1), data_width-1);
	       REAL absdelta = fabs(delta);
	       val =
		    (datarow[n * stride] * (1 - absdelta) +
		     datarow[n2 * stride] * absdelta) * weightrow +
		    (datarow2[n * stride] * (1 - absdelta) +
		     datarow2[n2 * stride] * absdelta) *
		    (1 - weightrow);
	       if (overlay)
		    olayval =
			 (olayrow[(n%ony) * ostride] * (1 - absdelta) +
			  olayrow[(n2%ony) * ostride] * absdelta) * weightrow +
			 (olayrow2[(n%ony) * ostride] * (1 - absdelta) +
			  olayrow2[(n2%ony) * ostride] * absdelta) *
		    (1 - weightrow);
	       if (maskrow != NULL) {
		    maskval =
			 (maskrow[(n%mny) * mstride] * (1 - absdelta) +
			  maskrow[(n2%mny) * mstride] * absdelta) * weightrow +
			 (maskrow2[(n%mny) * mstride] * (1 - absdelta) +
			  maskrow2[(n2%mny) * mstride] * absdelta) *
			 (1 - weightrow);
	       }
	  }

	  if (maskrow != NULL) {
	       REAL maskmin, maskmax;
	       if (init_mask_prev)
		    maskmin = maskmax = maskval;
	       else {
		    maskmin = MIN(MIN(maskval, i ? mask_prev[i-1] : maskval),
				  mask_prev[i]);
		    maskmax = MAX(MAX(maskval, i ? mask_prev[i-1] : maskval),
				  mask_prev[i]);
	       }
	       mask_prev[i] = maskval;
	       if (maskmin <= mask_thresh && maskmax >= mask_thresh) {
		    if (eight_bit)
			 row_pointer[i] = 255;
		    else
			 row_pointer[3*i]
			      = row_pointer[3*i + 1]
			      = row_pointer[3*i + 2] = mask_byte;
		    continue;
	       }
	  }

	  if (val > maxrange)
	       val = maxrange;
	  else if (val < minrange)
	       val = minrange;

	  if (eight_bit)
	       row_pointer[i] = (val - minrange) * scale;
	  else if (overlay) {
	       float r, g, b, a, ro, go, bo, ao;
	       cmap_lookup((val - minrange) / (maxrange - minrange),
			   cmap, &r, &g, &b, &a);
	       cmap_lookup((olayval - olaymin) / (olaymax - olaymin),
			   olay_cmap, &ro, &go, &bo, &ao);
	       r = r * (1 - ao) + ro * ao;
	       g = g * (1 - ao) + go * ao;
	       b = b * (1 - ao) + bo * ao;
	       row_pointer[3*i    ] = r * 255 + 0.5;
	       row_pointer[3*i + 1] = g * 255 + 0.5;
	       row_pointer[3*i + 2] = b * 255 + 0.5;
	  }
	  else {
	       float r, g, b, a;
	       cmap_lookup((val - minrange) / (maxrange - minrange),
			   cmap, &r, &g, &b, &a);
	       row_pointer[3*i    ] = r * 255 + 0.5;
	       row_pointer[3*i + 1] = g * 255 + 0.5;
	       row_pointer[3*i + 2] = b * 255 + 0.5;
	  }
     }
}

static void init_palette(png_colorp palette, colormap_t colormap,
			 png_byte mask_byte)
{
     int i;

     for (i = 0; i < 255; ++i) {
	  int j = i * 1.0/254 * (colormap.n - 1);
	  int j2 = (j == colormap.n - 1) ? j : j + 1;
	  REAL dj = i * 1.0/254 * (colormap.n - 1) - j;
	  float r,g,b;
	  r = colormap.rgba[j].r * (1-dj) + colormap.rgba[j2].r * dj;
	  g = colormap.rgba[j].g * (1-dj) + colormap.rgba[j2].g * dj;
	  b = colormap.rgba[j].b * (1-dj) + colormap.rgba[j2].b * dj;
	  palette[i].red = r * 255 + 0.5;
	  palette[i].green = g * 255 + 0.5;
	  palette[i].blue = b * 255 + 0.5;
     }

     /* set mask color: */
     palette[255].green = palette[255].blue = palette[255].red = mask_byte;
}

#define USE_ALPHA 0

#if USE_ALPHA
static void init_alpha(png_structp png_ptr, png_infop info_ptr,
		       colormap_t colormap)
{
     int i;
     png_bytep trans;

     for (i = 0; i < colormap.n; ++i)
	  if ((int) (colormap.rgba[i].a * 255 + 0.5) < 255)
	       break;
     if (i >= colormap.n)
	  return; /* all colors are opaque */

     trans = (png_bytep) malloc(sizeof(png_byte) * 256);
     for (i = 0; i < 255; ++i) {
          int j = i * 1.0/254 * (colormap.n - 1);
          int j2 = (j == colormap.n - 1) ? j : j + 1;
          REAL dj = i * 1.0/254 * (colormap.n - 1) - j;
          float a = colormap.rgba[j].a * (1-dj) + colormap.rgba[j2].a * dj;
	  trans[i] = a * 255 + 0.5;
     }
     trans[255] = 255; /* mask is always opaque */

     png_set_tRNS(png_ptr, info_ptr, trans, 256, 0);
}
#endif

void writepng(char *filename,
	      int nx, int ny, int transpose,
	      REAL skew, REAL scalex, REAL scaley,
	      REAL * data,
	      REAL *mask, REAL mask_thresh,
	      int mnx, int mny,
	      REAL *overlay, colormap_t overlay_cmap,
	      int onx, int ony,
	      REAL minrange, REAL maxrange,
	      colormap_t colormap, int eight_bit)
{
     FILE *fp;
     png_structp png_ptr;
     png_infop info_ptr;
     int height, width;
     double skewsin = sin(skew), skewcos = cos(skew);
     REAL minoverlay = 0, maxoverlay = 0;
     png_byte mask_byte;

     /* we must use direct color for translucent overlays */
     if (overlay)
	  eight_bit = 0;

     /* compute png size from scaled (and possibly transposed) data size,
      * and reverse the meaning of the scale factors; now they are what we
      * multiply png coordinates by to get data coordinates: */

     if (transpose) {
	  height = MAX(1, ny * scalex * skewcos);
	  width = MAX(1, nx * scaley * (1.0 + fabs(skewsin)));
	  scalex = height==1 ? 0 : (1.0 * (ny-1)) / (height-1);
	  scaley = width==1 ? 0 : ((1.0 + fabs(skewsin)) * (nx-1)) / (width-1);
     } else {
	  height = MAX(1, nx * scalex * skewcos);
	  width = MAX(1, ny * scaley * (1.0 + fabs(skewsin)));
	  scalex = height==1 ? 0 : (1.0 * (nx-1)) / (height-1);
	  scaley = width==1 ? 0 : ((1.0 + fabs(skewsin)) * (ny-1)) / (width-1);
     }

     if (overlay) {
	  int i;
	  minoverlay = maxoverlay = overlay[0];
	  for (i = 1; i < onx * ony; ++i) {
	       if (minoverlay > overlay[i])
		    minoverlay = overlay[i];
	       if (maxoverlay < overlay[i])
		    maxoverlay = overlay[i];
	  }
     }

     /* determine mask color by middle of colormap (FIXME: use
	median color of the data or some such thing instead?) */
     {
	  float r,g,b,a;
	  cmap_lookup(0.5, colormap, &r, &g, &b, &a);
	  if ((r + g + b) / 3.0 > 0.5)
	       mask_byte = 0; /* black */
	  else
	       mask_byte = 255; /* white */
     }

     fp = fopen(filename, "wb");
     if (fp == NULL) {
	  perror("Error creating file to write PNG in");
	  return;
     }
     /* Create and initialize the png_struct with the desired error
      * handler * functions.  If you want to use the default stderr and
      * longjump method, * you can supply NULL for the last three
      * parameters.  We also check that * the library version is
      * compatible with the one used at compile time, * in case we are
      * using dynamically linked libraries.  REQUIRED. */
     png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL,NULL);

     if (png_ptr == NULL) {
	  fclose(fp);
	  return;
     }
     /* Allocate/initialize the image information data.  REQUIRED */
     info_ptr = png_create_info_struct(png_ptr);
     if (info_ptr == NULL) {
	  fclose(fp);
	  png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
	  return;
     }
     /* Set error handling.  REQUIRED if you aren't supplying your own *
      * error hadnling functions in the png_create_write_struct() call. */
     if (setjmp(png_jmpbuf(png_ptr))) {
	  /* If we get here, we had a problem reading the file */
	  fclose(fp);
	  png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
	  return;
     }
     /* set up the output control if you are using standard C streams */
     png_init_io(png_ptr, fp);

     /* Set the image information here.  Width and height are up to
       2^31, bit_depth is one of 1, 2, 4, 8, or 16, but valid values
       also depend on the color_type selected. color_type is one of
       PNG_COLOR_TYPE_GRAY, PNG_COLOR_TYPE_GRAY_ALPHA,
       PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB, or
       PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either
       PNG_INTERLACE_NONE or PNG_INTERLACE_ADAM7, and the
       compression_type and filter_type MUST currently be
       PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED */

     if (!eight_bit)
	  png_set_IHDR(png_ptr, info_ptr, width, height, 8 /* bit_depth */ ,
		       PNG_COLOR_TYPE_RGB,
		       PNG_INTERLACE_NONE,
		       PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
     else {
	  png_colorp palette;
	  png_set_IHDR(png_ptr, info_ptr, width, height, 8 /* bit_depth */ ,
		       PNG_COLOR_TYPE_PALETTE,
		       PNG_INTERLACE_NONE,
		       PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	  /* initialize alpha channel (if any) via png_set_tRNS */
#if USE_ALPHA
	  init_alpha(png_ptr, info_ptr, colormap);
#endif
	  palette = (png_colorp) png_malloc(png_ptr, 256 * sizeof(png_color));

	  /* set the palette if there is one.  REQUIRED for indexed-color
	   * images */
	  init_palette(palette, colormap, mask_byte);
	  png_set_PLTE(png_ptr, info_ptr, palette, 256);
     }

     /* Write the file header information.  REQUIRED */
     png_write_info(png_ptr, info_ptr);

     /* Write out data, one row at a time: */
     {
	  REAL scale, *mask_prev = NULL;
	  png_byte *row_pointer;
	  int row;
	  int data_height = transpose ? ny : nx;
	  int data_width = transpose ? nx : ny;

	  if (maxrange > minrange)
	       scale = 254.0 / (maxrange - minrange);
	  else
	       scale = 0.0;

	  row_pointer = (png_byte *) malloc(width * sizeof(png_byte) *
					    (eight_bit ? 1 : 3));
	  if (row_pointer == NULL) {
	       fclose(fp);
	       return;
	  }
	  if (mask) {
	       mask_prev = (REAL *) malloc(width * sizeof(REAL));
	       if (mask_prev == NULL) {
		    free(row_pointer);
		    fclose(fp);
		    return;
	       }
	  }
	  for (row = height-1; row >= 0; --row) {
	       REAL x = row * scalex;
	       int n = PIN(0,(int) (x + 0.5), data_height-1);
	       double delta = x - n;
	       int n2 = PIN(0,n + (delta>0.0 ? 1 : -1), data_height-1);
	       int n3 = PIN(0,n + 1, data_height-1);
	       REAL offset;

	       if (skewsin < 0.0)
		    offset = x*skewsin;
	       else
		    offset = (x - (height-1)*scalex) * skewsin;
	       if (transpose)
		    convert_row(width, data_width, scaley, offset,
				data + n, data + n2, 1 - fabs(delta),
				data_height,
				mask ? mask + (n%mny) : NULL,
				mask ? mask + (n3%mny) : NULL,
				mask_thresh, mask_prev, row == height-1,
				mask_byte, mnx, mny,
				overlay != 0,
				overlay + (n%ony), overlay + (n2%ony),
				overlay_cmap, minoverlay, maxoverlay, onx, ony,
				colormap, minrange, maxrange, scale,
				row_pointer, eight_bit);
	       else
		    convert_row(width, data_width, scaley, offset,
				data + n * data_width, data + n2 * data_width,
				1 - fabs(delta),
				1,
				mask ? mask + (n%mnx) * mny : NULL,
				mask ? mask + (n3%mnx) * mny : NULL,
				mask_thresh, mask_prev, row == height-1,
				mask_byte, mny, 1,
				overlay != 0,
				overlay + (n%onx) * ony,
				overlay + (n2%onx) * ony,
				overlay_cmap, minoverlay, maxoverlay, ony, 1,
				colormap, minrange, maxrange, scale,
				row_pointer, eight_bit);
	       png_write_rows(png_ptr, &row_pointer, 1);
	  }

	  free(row_pointer);
	  free(mask_prev);
     }

     /* It is REQUIRED to call this to finish writing the rest of the file */
     png_write_end(png_ptr, info_ptr);

     /* if you malloced the palette, free it here */
     {
          png_colorp palette; int num_palette;
          if (0 != png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette))
               png_free(png_ptr, palette);
     }

     /* if you allocated any text comments, free them here */

     /* clean up after the write, and free any memory allocated */
     png_destroy_write_struct(&png_ptr, (png_infopp) NULL);

     /* close the file */
     fclose(fp);

     /* that's it */
}

/* In the following code, we use a heuristic algorithm to compute
 * the range.  The range is set to [-r, r], where r is computed
 * as follows:
 *
 * 1) for each new data set, compute
 * r' = sqrt(2.0 * (average of non-zero data[i]^2))
 *
 * 2) r = max(r', r of previous data set)
 */

#define WHITE_EPSILON 0.003921568627	/* 1 / 255 */

void writepng_autorange(char *filename,
			int nx, int ny, int transpose,
			REAL skew, REAL scalex,REAL scaley,
			REAL * data,
			REAL *mask, REAL mask_thresh,
			REAL *overlay, colormap_t overlay_cmap,
			colormap_t colormap, int eight_bit)
{
     static REAL range = 0.0;
     REAL sum = 0, newrange, max = -1.0;
     int i, count = 0;

     sum = 0;
     for (i = 0; i < nx * ny; ++i) {
	  REAL absval = fabs(data[i]);

	  if (absval >= WHITE_EPSILON * range) {
	       sum += absval * absval;
	       ++count;
	  }
	  if (absval > max)
	       max = absval;
     }

     if (count) {
	  newrange = 5 * sqrt(sum / count);
	  if (newrange > max)
	       newrange = max;
	  if (newrange > range)
	       range = newrange;
     }
     writepng(filename, nx, ny, transpose, skew, scalex, scaley,
	      data, mask, mask_thresh, nx,ny, overlay, overlay_cmap, nx,ny,
	      -range, range, colormap, eight_bit);
}
