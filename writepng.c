/* Copyright (c) 1999, 2000 Massachusetts Institute of Technology
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

static void convert_row(int png_width, int data_width,
			REAL scaley, REAL offsety,
			REAL *datarow, REAL *datarow2, REAL weightrow,
			int stride, REAL *maskrow, REAL *maskrow2,
			REAL mask_thresh, REAL *mask_prev,
			REAL minrange, REAL maxrange, REAL scale, int invert,
			png_byte * row_pointer)
{
     int i;

     for (i = 0; i < png_width; ++i) {
	  REAL y = i * scaley + offsety;
	  int n = (int) (y + 0.5);
	  double delta = y - n;
	  REAL val, maskval = 0.0;

	  if (n < 0 || n > data_width) {
	       row_pointer[i] = 255;
	       continue;
	  }
	  
	  if (delta == 0.0) {
	       val = (datarow[n * stride] * weightrow +
		      datarow2[n * stride] * (1.0 - weightrow));
	       if (maskrow != NULL) {
		    maskval = (maskrow[n * stride] * weightrow +
			       maskrow2[n * stride] * (1.0 - weightrow));
	       }
	  }
	  else {
	       int n2 = (data_width + n +
			 (delta < 0.0 ? -1 : 1)) % data_width;
	       REAL absdelta = fabs(delta);
	       val = 
		    (datarow[n * stride] * (1.0 - absdelta) +
		     datarow[n2 * stride] * absdelta) * weightrow +
		    (datarow2[n * stride] * (1.0 - absdelta) +
		     datarow2[n2 * stride] * absdelta) * 
		    (1.0 - weightrow);
	       if (maskrow != NULL) {
		    maskval = 
			 (maskrow[n * stride] * (1.0 - absdelta) +
			  maskrow[n2 * stride] * absdelta) * weightrow +
			 (maskrow2[n * stride] * (1.0 - absdelta) +
			  maskrow2[n2 * stride] * absdelta) * 
			 (1.0 - weightrow);
	       }
	  }

	  if (maskrow != NULL) {
	       REAL maskmin, maskmax;
	       maskmin = MIN(MIN(maskval, i ? mask_prev[i-1] : maskval), 
				 mask_prev[i]);
	       maskmax = MAX(MAX(maskval, i ? mask_prev[i-1] : maskval), 
				 mask_prev[i]);
	       mask_prev[i] = maskval;
	       if (maskmin <= mask_thresh && maskmax >= mask_thresh) {
		    row_pointer[i] = 255;
		    continue;
	       }
	  }	 
	  
	  if (val > maxrange)
	       val = maxrange;
	  else if (val < minrange)
	       val = minrange;
	  
	  if (invert)
	       row_pointer[i] = 254 - (val - minrange) * scale;
	  else
	       row_pointer[i] = (val - minrange) * scale;
     }
}

static void init_palette(png_colorp palette, colormap_t colormap)
{
     int i;
     int mid = 0.5 * 254;

     if (colormap == BLUE_WHITE_RED) {
	  for (i = 0; i < mid; ++i) {
	       palette[i].green = palette[i].red = i * 254.0 / mid;
	       palette[i].blue = 254;
	  }
	  for (i = mid; i < 255; ++i) {
	       palette[i].green = palette[i].blue = 
		    (254 - i) * 254.0 / (254 - mid);
	       palette[i].red = 254;
	  }
     } else {
	  /* default to grayscale palette: */
	  for (i = 0; i < 255; ++i)
	       palette[i].green = palette[i].blue = palette[i].red = 255 - i;
     }

     /* set mask color: */

     if (palette[mid].red/3. + palette[mid].green/3. + palette[mid].blue/3.
	 > mid)
	  /* black */
	  palette[255].green = palette[255].blue = palette[255].red = 0; 
     else
	  /* white */
	  palette[255].green = palette[255].blue = palette[255].red = 255; 
}

void writepng(char *filename,
	      int nx, int ny, int transpose, 
	      REAL skew, REAL scalex, REAL scaley,
	      REAL * data,
	      REAL *mask, REAL mask_thresh,
	      REAL minrange, REAL maxrange,
	      int invert,
	      colormap_t colormap)
{
     FILE *fp;
     png_structp png_ptr;
     png_infop info_ptr;
     int height, width;
     double skewsin = sin(skew), skewcos = cos(skew);

     /* compute png size from scaled (and possibly transposed) data size,
      * and reverse the meaning of the scale factors; now they are what we 
      * multiply png coordinates by to get data coordinates: */

     if (transpose) {
	  height = MAX(1, ny * scalex * skewcos);
	  width = MAX(1, nx * scaley * (1.0 + fabs(skewsin)));
	  scalex = (1.0 * ny) / height;
	  scaley = ((1.0 + fabs(skewsin)) * nx) / width;
     } else {
	  height = MAX(1, nx * scalex * skewcos);
	  width = MAX(1, ny * scaley * (1.0 + fabs(skewsin)));
	  scalex = (1.0 * nx) / height;
	  scaley = ((1.0 + fabs(skewsin)) * ny) / width;
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
     if (setjmp(png_ptr->jmpbuf)) {
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

     png_set_IHDR(png_ptr, info_ptr, width, height, 8 /* bit_depth */ ,
		  PNG_COLOR_TYPE_PALETTE,
		  PNG_INTERLACE_NONE,
		  PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

     {
	  png_colorp palette;

	  /* set the palette if there is one.  REQUIRED for indexed-color
	   * images */

	  palette = (png_colorp) png_malloc(png_ptr, 256 * sizeof(png_color));
	  init_palette(palette, colormap);
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

	  row_pointer = (png_byte *) malloc(width * sizeof(png_byte));
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
	  for (row = 0; row < height; ++row) {
	       REAL x = row * scalex;
	       int n = ((int) (x + 0.5)) % data_height;
	       double delta = x - n;
	       int n2 = (n + data_height + (delta>0.0 ? 1 : -1)) % data_height;
	       int n3 = (n + 1) % data_height;
	       REAL offset;

	       if (skewsin < 0.0)
		    offset = x*skewsin;
	       else
		    offset = (x - (height-1)*scalex) * skewsin;
	       if (transpose)
		    convert_row(width, data_width, scaley, offset,
				data + n, data + n2, 1.0 - fabs(delta),
				data_height, 
				mask ? mask + n : NULL,
				mask ? mask + n3 : NULL,
				mask_thresh, mask_prev,
				minrange, maxrange, scale, invert,
				row_pointer);
	       else
		    convert_row(width, data_width, scaley, offset,
				data + n * data_width, data + n2 * data_width,
				1.0 - fabs(delta),
				1, 
				mask ? mask + n * data_width : NULL,
				mask ? mask + n3 * data_width : NULL,
				mask_thresh, mask_prev,
				minrange, maxrange, scale, invert,
				row_pointer);
	       png_write_rows(png_ptr, &row_pointer, 1);
	  }

	  free(row_pointer);
	  free(mask_prev);
     }

     /* It is REQUIRED to call this to finish writing the rest of the file */
     png_write_end(png_ptr, info_ptr);

     /* if you malloced the palette, free it here */
     free(info_ptr->palette);

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
			int invert,
			colormap_t colormap)
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
	      data, mask, mask_thresh, -range, range, invert, colormap);
}
