/* Copyright (c) 1999, 2000, 2001, 2002 Massachusetts Institute of Technology
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include <unistd.h>

#include "config.h"
#include "arrayh5.h"
#include "copyright.h"
#include "writepng.h"

#define CHECK(cond, msg) { if (!(cond)) { fprintf(stderr, "h5topng error: %s\n", msg); exit(EXIT_FAILURE); } }

#define CMAP_DEFAULT "gray"
#define CMAP_DIR DATADIR "/h5utils/colormaps/"

void usage(FILE *f)
{
     fprintf(f, "Usage: h5topng [options] [<filenames>]\n"
	     "Options:\n"
	     "         -h : this help message\n"
             "         -V : print version number and copyright\n"
	     "         -v : verbose output\n"
	     "  -o <file> : output to <file> (first input file only)\n"
	     "    -x <ix> : take x=<ix> slice of data\n"
	     "    -y <iy> : take y=<iy> slice of data\n"
	     "    -z <iz> : take z=<iz> slice of data [ default: z=0 ]\n"
	     "         -0 : use dataset center as origin for -x/-y/-z\n"
	     "    -X <sx> : scale width by <sx> [ default: 1.0 ]\n"
	     "    -Y <sy> : scale height by <sy> [ default: 1.0 ]\n"
	     "     -S <s> : equivalent to -X <s> -Y <s>\n"
	     "  -s <skew> : skew axes by <skew> degrees [ default: 0 ]\n"
	     "         -T : transpose the data [default: no]\n"
	     "  -c <cmap> : use colormap <cmap> [default: " CMAP_DEFAULT "]\n"
	     "              (see " CMAP_DIR " for other colormaps)\n"
	     "         -r : reverse color map [default: no]\n"
	     "         -Z : center color scale at zero [default: no]\n"
	     "   -m <min> : set bottom of color scale to data value <min>\n"
	     "   -M <max> : set top of color scale to data value <max>\n"
	     "         -R : use uniform colormap range for all files\n"
	     "  -C <file> : superimpose contour outlines from <file>\n"
	     "   -b <val> : contours around values != <val> [default: 1.0]\n"
	     "  -d <name> : use dataset <name> in the input files (default: first dataset)\n"
	     "              -- you can also specify a dataset via <filename>:<name>\n"
	  );
}

/* given an fname of the form <filename>:<data_name>, return a pointer
   to a newly-allocated string containing <filename>, and point data_name
   to the position of <data_name> in fname.  The user must free() the
   <filename> string. */
static char *split_fname(char *fname, char **data_name)
{
     int fname_len;
     char *colon, *filename;

     fname_len = strlen(fname);
     colon = strchr(fname, ':');
     if (colon) {
          int colon_len = strlen(colon);
          filename = (char*) malloc(sizeof(char) * (fname_len-colon_len+1));
          CHECK(filename, "out of memory");
          strncpy(filename, fname, fname_len-colon_len+1);
	  filename[fname_len-colon_len] = 0;
          *data_name = colon + 1;
     }
     else { /* treat as if ":" were at the end of fname */
          filename = (char*) malloc(sizeof(char) * (fname_len + 1));
          CHECK(filename, "out of memory");
          strcpy(filename, fname);
          *data_name = fname + fname_len;
     }
     return filename;
}

rgba_t gray_colors[2] = { {1,1,1,1}, {0,0,0,1} };
colormap_t gray_cmap = { 2, gray_colors };

static colormap_t load_colormap(FILE *f, int verbose)
{
     colormap_t cmap = {0, NULL};
     int nalloc = 0;
     float r,g,b,a;
     int c;
     
     /* read initial comment lines, and echo if verbose */
     do {
	  while (isspace(c = fgetc(f)));
	  if (c == '#' || c == '%') {
	       while (isspace(c = fgetc(f)) && c != '\n' && c != EOF);
	       if (c != EOF) ungetc(c, f);
	       while ('\n' != (c = fgetc(f)) && c != EOF)
		    if (verbose) 
			 putchar(c);
	       if (verbose)
		    putchar('\n');
	  }
     } while (c == '\n');
     if (c != EOF) ungetc(c, f);

     while (4 == fscanf(f, "%g %g %g %g", &r, &g, &b, &a)) {
	  if (cmap.n >= nalloc) {
	       nalloc = (1 + nalloc) * 2;
	       cmap.rgba = realloc(cmap.rgba, nalloc * sizeof(rgba_t));
	       CHECK(cmap.rgba, "out of memory");
	  }
	  cmap.rgba[cmap.n].r = r;
	  cmap.rgba[cmap.n].g = g;
	  cmap.rgba[cmap.n].b = b;
	  cmap.rgba[cmap.n].a = a;
	  cmap.n++;
     }
     cmap.rgba = realloc(cmap.rgba, cmap.n * sizeof(rgba_t));
     CHECK(cmap.n >= 1, "invalid colormap file");
     if (verbose)
	  printf("%d color entries read from colormap file.\n", cmap.n);
     return cmap;
}

int main(int argc, char **argv)
{
     arrayh5 a, contour_data;
     char *png_fname = NULL, *contour_fname = NULL, *data_name = NULL;
     REAL mask_thresh = 0;
     int mask_thresh_set = 0;
     double min = 0, max = 0, allmin = 0, allmax = 0;
     int min_set = 0, max_set = 0, collect_range = 0;
     extern char *optarg;
     extern int optind;
     int c;
     int slicedim = 2, islice = 0, center_slice = 0;
     int err;
     int nx, ny;
     char *colormap = NULL, *cmap_fname = NULL;
     FILE *cmap_f = NULL;
     colormap_t cmap = { 0, NULL };
     int verbose = 0;
     int transpose = 0;
     int zero_center = 0;
     double scalex = 1.0, scaley = 1.0;
     int invert = 0;
     double skew = 0.0;
     int ifile;

     colormap = (char*) malloc(sizeof(char) * (strlen(CMAP_DEFAULT) + 1));
     CHECK(colormap, "out of memory");
     strcpy(colormap, CMAP_DEFAULT);

     while ((c = getopt(argc, argv, "ho:x:y:z:0c:m:M:RC:b:d:vX:Y:S:TrZs:V")) != -1)
	  switch (c) {
	      case 'h':
		   usage(stdout);
		   return EXIT_SUCCESS;
	      case 'V':
		   printf("h5topng " VERSION " by Steven G. Johnson\n" 
			  COPYRIGHT);
		   return EXIT_SUCCESS;
	      case 'v':
		   verbose = 1;
		   break;
	      case 'T':
		   transpose = 1;
		   break;
	      case '0':
		   center_slice = 1;
		   break;
	      case 'r':
		   invert = 1;
		   break;
	      case 'Z':
		   zero_center = 1;
		   break;
	      case 'R':
		   collect_range = 1;
		   break;
	      case 'o':
		   free(png_fname);
		   png_fname = (char*) malloc(sizeof(char) *
					      (strlen(optarg) + 1));
		   CHECK(png_fname, "out of memory");
		   strcpy(png_fname, optarg);
		   break;
	      case 'd':
		   free(data_name);
		   data_name = (char*) malloc(sizeof(char) *
					      (strlen(optarg) + 1));
		   CHECK(data_name, "out of memory");
		   strcpy(data_name, optarg);
		   break;		   
	      case 'C':
		   free(contour_fname);
		   contour_fname = (char*) malloc(sizeof(char) *
						  (strlen(optarg) + 1));
		   CHECK(contour_fname, "out of memory");
		   strcpy(contour_fname, optarg);
		   break;
	      case 'x':
		   islice = atoi(optarg);
		   slicedim = 0;
		   break;
	      case 'y':
		   islice = atoi(optarg);
		   slicedim = 1;
		   break;
	      case 'z':
		   islice = atoi(optarg);
		   slicedim = 2;
		   break;
	      case 'c':
		   free(colormap);
		   colormap = (char*) malloc(sizeof(char) *
					     (strlen(optarg) + 1));
                   CHECK(colormap, "out of memory");
                   strcpy(colormap, optarg);
		   break;
	      case 'm':
		   min = atof(optarg);
		   min_set = 1;
		   break;
	      case 'M':
		   max = atof(optarg);
		   max_set = 1;
		   break;
	      case 'b':
		   mask_thresh = atof(optarg);
		   mask_thresh_set = 1;
		   break;
	      case 'X':
		   scalex = atof(optarg);
		   break;
	      case 'Y':
		   scaley = atof(optarg);
		   break;
	      case 'S':
		   scalex = scaley = atof(optarg);
		   break;
	      case 's':
		   skew = atof(optarg) * 3.14159265358979323846 / 180.0;
		   break;
	      default:
		   fprintf(stderr, "Invalid argument -%c\n", c);
		   usage(stderr);
		   return EXIT_FAILURE;
	  }

     cmap_fname = (char *) malloc(sizeof(char) *
				  (strlen(CMAP_DIR) + strlen(colormap) + 1));
     CHECK(cmap_fname, "out of memory");
     strcpy(cmap_fname, CMAP_DIR); strcat(cmap_fname, colormap);
     if (!(cmap_f = fopen(cmap_fname, "r"))) {
	  free(cmap_fname);
	  cmap_fname = (char *) malloc(sizeof(char) * (strlen(colormap) + 1));
	  CHECK(cmap_fname, "out of memory");
	  strcpy(cmap_fname, colormap);
	  if (!(cmap_f = fopen(cmap_fname, "r"))) {
	       if (!strcmp(colormap, "gray"))
		    cmap = gray_cmap;
	       else {
		    fprintf(stderr, "Could not find colormap \"%s\"\n",
			    colormap);
		    if (colormap[0] == '-' || strstr(colormap, ".h5"))
			 fprintf(stderr, "  -- Note that the the '-c' option syntax changed in h5topng 1.7!\n     To get the old behavior, use '-c bluered' ('man h5topng' for more info).\n");
		    exit(EXIT_FAILURE);
	       }
	  }
     }
     if (cmap.rgba == gray_colors) {
	  if (verbose)
	       printf("Using built-in gray colormap.\n");
     }
     else {
	  if (verbose)
	       printf("Using colormap \"%s\" in file \"%s\".\n",
		      colormap, cmap_fname);
	  cmap = load_colormap(cmap_f, verbose);
	  fclose(cmap_f);
     }

     if (optind == argc) {  /* no parameters left */
	  usage(stderr);
	  return EXIT_FAILURE;
     }

     if (contour_fname) {
	  int cnx, cny;
	  char *fname, *dname;

	  fname = split_fname(contour_fname, &dname);
	  if (!dname[0])
	       dname = NULL;

	  if (verbose)
	       printf("reading contour data from \"%s\".\n", fname);
	  err = arrayh5_read(&contour_data, fname, dname, NULL,
			     slicedim, islice, center_slice);
	  CHECK(!err, arrayh5_read_strerror[err]);
	  CHECK(contour_data.rank >= 1,
		"data must have at least one dimension");
	  
	  cnx = contour_data.dims[0];
	  cny = contour_data.rank >= 2 ? contour_data.dims[1] : 1;
	  
	  if (!mask_thresh_set) {
               double c_min, c_max;
               arrayh5_getrange(contour_data, &c_min, &c_max);
	       mask_thresh = (c_min + c_max) * 0.5;
	  }

	  free(fname);
     }

 process_files:     
     if (verbose)
	  printf("------\n");
     for (ifile = optind; ifile < argc; ++ifile) {
          char *dname, *h5_fname;
          h5_fname = split_fname(argv[ifile], &dname);
          if (!dname[0])
               dname = data_name;

	  if (verbose)
	       printf("reading from \"%s\", slice at %d in %c dimension.\n",
		      h5_fname, islice, slicedim + 'x');
	  
	  err = arrayh5_read(&a, h5_fname, dname, NULL,
			     slicedim, islice, center_slice);
	  CHECK(!err, arrayh5_read_strerror[err]);
	  CHECK(a.rank >= 1, "data must have at least one dimension");
	  
	  CHECK(!contour_fname || arrayh5_conformant(a, contour_data),
		"contour data must be conformant with source data");
	  
	  if (!png_fname) {
	       png_fname = (char *) malloc(sizeof(char) * 
					   (strlen(h5_fname) + 5));
	       strcpy(png_fname, h5_fname);
	       
	       /* remove ".h5" from filename: */
	       if (strlen(png_fname) >= strlen(".h5") &&
		   !strcmp(png_fname + strlen(png_fname) - strlen(".h5"),
			   ".h5"))
		    png_fname[strlen(png_fname) - strlen(".h5")] = 0;
	       
	       /* add ".png": */
	       strcat(png_fname, ".png");
	  }
	  
	  {
	       double a_min, a_max;
	       arrayh5_getrange(a, &a_min, &a_max);
	       if (verbose)
		    printf("data ranges from %g to %g.\n", a_min, a_max);
	       if (!min_set)
		    min = a_min;
	       if (!max_set)
		    max = a_max;
	       if (ifile == optind || a_min < allmin)
		    allmin = a_min;
	       if (ifile == optind || a_max > allmax)
		    allmax = a_max;
	       if (min > max) {
		    invert = !invert;
		    a_min = min;
		    min = max;
		    max = a_min;
	       }
	       if (zero_center) {
		    max = fabs(max) > fabs(min) ? fabs(max) : fabs(min);
		    min = -max;
	       }
	  }
	  
	  if (!collect_range) {
	       nx = a.dims[0];
	       ny = a.rank < 2 ? 1 : a.dims[1];
	       
	       if (verbose)
		    printf("writing \"%s\" from %dx%d input data.\n",
			   png_fname, nx, ny);
	       
	       writepng(png_fname, nx, ny, transpose, skew,
			scaley, scalex, a.data, 
			contour_fname ? contour_data.data : NULL, mask_thresh,
			min, max, invert, cmap);
	  }
	  arrayh5_destroy(a);
	  free(png_fname); png_fname = NULL;
	  free(h5_fname);
     }
     if (verbose && optind < argc - 1)
	  printf("all data range from %g to %g.\n", allmin, allmax);
     if (collect_range) {
	  if (!min_set)
	       min = allmin;
	  if (!max_set)
	       max = allmax;
	  min_set = max_set = 1;
	  collect_range = 0;
	  goto process_files;
     }

     if (contour_fname)
	  arrayh5_destroy(contour_data);
     free(contour_fname);
     free(data_name);

     if (cmap.rgba != gray_colors)
	  free(cmap.rgba);
     free(cmap_fname);
     free(colormap);

     return EXIT_SUCCESS;
}
