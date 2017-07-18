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
#include "h5utils.h"

#define CHECK(cond, msg) { if (!(cond)) { fprintf(stderr, "h5topng error: %s\n", msg); exit(EXIT_FAILURE); } }

#define CMAP_DEFAULT "gray"
#define OVERLAY_CMAP_DEFAULT "yellow"
#define OVERLAY_OPACITY_DEFAULT 0.2
#define CMAP_DIR DATADIR "/" PACKAGE_NAME "/colormaps/"

void usage(FILE *f)
{
     fprintf(f, "Usage: h5topng [options] [<filenames>]\n"
	     "Options:\n"
	     "         -h : this help message\n"
             "         -V : print version number and copyright\n"
	     "         -v : verbose output\n"
	     "  -o <file> : output to <file> (first input file only)\n"
	     "    -x <ix> : take x=<ix> slice of data (or <min>:<inc>:<max>)\n"
	     "    -y <iy> : take y=<iy> slice of data\n"
	     "    -z <iz> : take z=<iz> slice of data\n"
	     "    -t <it> : take t=<it> slice of data's last dimension\n"
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
	     "  -A <file> : overlay data from <file>, as specified by -y\n"
"  -a <c>:<o>: overlay colormap <c>, opacity <o> (0-1) [default: %s:%g]\n"
"         -8 : use an 8-bit color table, instead of 24-bit direct color\n"
	     "  -d <name> : use dataset <name> in the input files (default: first dataset)\n"
	     "              -- you can also specify a dataset via <filename>:<name>\n",
	  OVERLAY_CMAP_DEFAULT, OVERLAY_OPACITY_DEFAULT);
}

rgba_t gray_colors[2] = { {1,1,1,0}, {0,0,0,1} };
colormap_t gray_cmap = { 2, gray_colors };

rgba_t yellow_colors[2] = { {1,1,1,0}, {1,1,0,1} };
colormap_t yellow_cmap = {2, yellow_colors};

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

colormap_t copy_colormap(const colormap_t c0)
{
     colormap_t c;
     int i;
     c.n = c0.n;
     c.rgba = (rgba_t *) malloc(c.n * sizeof(rgba_t));
     CHECK(c.rgba, "out of memory");
     for (i = 0; i < c.n; ++i)
	  c.rgba[i] = c0.rgba[i];
     return c;
}

colormap_t get_cmap(const char *colormap, int invert, double scale_alpha,
		    int verbose)
{
     int i;
     colormap_t cmap = {0, NULL};
     FILE *cmap_f = NULL;
     char *cmap_fname = (char *) malloc(sizeof(char) *
					(strlen(CMAP_DIR)
					 + strlen(colormap) + 1));
     CHECK(cmap_fname, "out of memory");
     if (colormap[0] == '-') {
	  invert = 1;
	  colormap++;
     }
     strcpy(cmap_fname, CMAP_DIR); strcat(cmap_fname, colormap);
     if (colormap[0] == '.' || colormap[0] == '/'
	 || !(cmap_f = fopen(cmap_fname, "r"))) {
	  free(cmap_fname);
	  cmap_fname = my_strdup(colormap);
	  if (!(cmap_f = fopen(cmap_fname, "r"))) {
	       if (!strcmp(colormap, "gray"))
		    cmap = gray_cmap;
	       if (!strcmp(colormap, "yellow"))
		    cmap = yellow_cmap;
	       else {
		    fprintf(stderr, "Could not find colormap \"%s\"\n",
			    colormap);
		    exit(EXIT_FAILURE);
	       }
	  }
     }
     if (cmap.rgba == gray_colors) {
	  if (verbose)
	       printf("Using built-in gray colormap%s.\n",
		      invert ? " (inverted)" : "");
	  cmap = copy_colormap(cmap);
     }
     else if (cmap.rgba == yellow_colors) {
	  if (verbose)
	       printf("Using built-in yellow colormap%s.\n",
		      invert ? " (inverted)" : "");
	  cmap = copy_colormap(cmap);
     }
     else {
	  if (verbose)
	       printf("Using colormap \"%s\" in file \"%s\"%s.\n",
		      colormap, cmap_fname, invert ? " (inverted)" : "");
	  cmap = load_colormap(cmap_f, verbose);
	  fclose(cmap_f);
     }
     free(cmap_fname);
     if (invert)
	  for (i = 0; i < cmap.n - 1 - i; ++i) {
	       rgba_t rgba = cmap.rgba[i];
	       cmap.rgba[i] = cmap.rgba[cmap.n - 1 - i];
	       cmap.rgba[cmap.n - 1 - i] = rgba;
	  }
     if (verbose) printf("Scaling opacity by %g\n", scale_alpha);
     for (i = 0; i < cmap.n; ++i)
	  cmap.rgba[i].a *= scale_alpha;
     return cmap;
}

static int get_islice(const char *s, int *min, int *max, int *step)
{
     int num_read = sscanf(s, "%d:%d:%d", min, step, max);
     if (num_read == 1) {
	  *max = *min;
	  *step = 1;
     }
     else if (num_read == 2) {
	  *max = *step;
	  *step = 1;
     }
     CHECK(num_read, "invalid slice argument");
     return num_read;
}

static int iabs(int x) { return x < 0 ? -x : x; }
static int imax(int x, int y) { return x > y ? x : y; }
static int ilog10(int x) {
     int lg = 0, prod = 1;
     while (prod < x) {
	  ++lg;
	  prod *= 10;
     }
     return lg - 1;
}

int main(int argc, char **argv)
{
     arrayh5 a, contour_data, overlay_data;
     char *png_fname = NULL, *contour_fname = NULL, *data_name = NULL;
     char *overlay_fname = NULL;
     REAL mask_thresh = 0;
     int mask_thresh_set = 0;
     double min = 0, max = 0, allmin = 0, allmax = 0;
     int min_set = 0, max_set = 0, collect_range = 0;
     extern char *optarg;
     extern int optind;
     int c;
     int slicedim[4] = {NO_SLICE_DIM,NO_SLICE_DIM,NO_SLICE_DIM,NO_SLICE_DIM};
     int islice[4], center_slice[4] = {0,0,0,0};
     int islice_min[4] = {0,0,0,0}, islice_max[4] = {0,0,0,0}, islice_step[4] = {1,1,1,1};
     int err;
     int nx, ny;
     char *colormap = NULL, *overlay_colormap = NULL;
     int overlay_invert = 0;
     colormap_t cmap = { 0, NULL };
     colormap_t overlay_cmap = { 0, NULL };
     double overlay_opacity = OVERLAY_OPACITY_DEFAULT;
     int verbose = 0;
     int transpose = 0;
     int zero_center = 0;
     double scalex = 1.0, scaley = 1.0;
     int invert = 0;
     double skew = 0.0;
     int eight_bit = 0;
     int ifile, num_processed;
     int data_rank, slicedim3;

     colormap = my_strdup(CMAP_DEFAULT);
     overlay_colormap = my_strdup(OVERLAY_CMAP_DEFAULT);

     while ((c = getopt(argc, argv, "ho:x:y:z:t:0c:m:M:RC:b:d:vX:Y:S:TrZs:Va:A:8")) != -1)
	  switch (c) {
	      case 'h':
		   usage(stdout);
		   return EXIT_SUCCESS;
	      case 'V':
		   printf("h5topng " PACKAGE_VERSION " by Steven G. Johnson\n"
			  COPYRIGHT);
		   return EXIT_SUCCESS;
	      case 'v':
		   verbose = 1;
		   break;
	      case 'T':
		   transpose = 1;
		   break;
	      case 'r':
		   invert = 1;
		   break;
	      case '8':
		   eight_bit = 1;
		   break;
	      case 'Z':
		   zero_center = 1;
		   break;
	      case 'R':
		   collect_range = 1;
		   break;
	      case 'o':
		   free(png_fname);
		   png_fname = my_strdup(optarg);
		   break;
	      case 'd':
		   free(data_name);
		   data_name = my_strdup(optarg);
		   break;
	      case 'C':
		   free(contour_fname);
		   contour_fname = my_strdup(optarg);
		   break;
	      case 'A':
		   free(overlay_fname);
		   overlay_fname = my_strdup(optarg);
		   break;
	      case 'x':
		   get_islice(optarg, &islice_min[0], &islice_max[0],
			      &islice_step[0]);
		   slicedim[0] = 0;
		   break;
	      case 'y':
		   get_islice(optarg, &islice_min[1], &islice_max[1],
			      &islice_step[1]);
		   slicedim[1] = 1;
		   break;
	      case 'z':
		   get_islice(optarg, &islice_min[2], &islice_max[2],
			      &islice_step[2]);
		   slicedim[2] = 2;
		   break;
	      case 't':
		   get_islice(optarg, &islice_min[3], &islice_max[3],
			      &islice_step[3]);
		   slicedim[3] = LAST_SLICE_DIM;
		   break;
              case '0':
                   center_slice[0] = center_slice[1] = center_slice[2] = 1;
                   break;
	      case 'c':
		   free(colormap);
		   colormap = my_strdup(optarg);
		   break;
	      case 'a':
		   free(overlay_colormap);
		   overlay_colormap = my_strdup(optarg);
		   if (strchr(optarg, ':')) {
			*(strchr(overlay_colormap, ':')) = 0;
			sscanf(strchr(optarg, ':')+1, "%lg", &overlay_opacity);
			CHECK(overlay_opacity >= 0 && overlay_opacity <= 1,
			      "invalid opacity in -a: must be from 0 to 1");
		   }
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

     CHECK(!overlay_fname || !eight_bit,
	   "-8 option is not currently supported with -A");

     cmap = get_cmap(colormap, invert, 1.0, verbose);
     if (overlay_fname)
	  overlay_cmap = get_cmap(overlay_colormap, overlay_invert,
				  overlay_opacity, verbose);

     if (optind == argc) {  /* no parameters left */
	  usage(stderr);
	  return EXIT_FAILURE;
     }

     contour_data.data = overlay_data.data = NULL;

     slicedim3 = slicedim[3];
     {
          char *dname, *h5_fname;
          h5_fname = split_fname(argv[optind], &dname);
          if (!dname[0])
               dname = data_name;
	  err = arrayh5_read_rank(h5_fname, dname, &data_rank);
	  CHECK(!err, arrayh5_read_strerror[err]);
	  free(h5_fname);
	  if (verbose)
	       printf("data rank = %d\n", data_rank);
     }

 process_files:

     num_processed = 0;

     for (islice[0] = islice_min[0]; islice[0] <= islice_max[0];
	  islice[0] += islice_step[0])
     for (islice[1] = islice_min[1]; islice[1] <= islice_max[1];
	  islice[1] += islice_step[1])
     for (islice[2] = islice_min[2]; islice[2] <= islice_max[2];
	  islice[2] += islice_step[2])
     for (islice[3] = islice_min[3]; islice[3] <= islice_max[3];
	  islice[3] += islice_step[3]) {

     int onx = 1, ony = 1;
     int cnx = 1, cny = 1;

     if (contour_fname && !collect_range) {
	  int rank;
	  char *fname, *dname;

	  fname = split_fname(contour_fname, &dname);
	  if (!dname[0])
	       dname = NULL;

	  if (verbose)
	       printf("reading contour data from \"%s\".\n", fname);

	  err = arrayh5_read_rank(fname, dname, &rank);
          CHECK(!err, arrayh5_read_strerror[err]);
	  if (slicedim3 == LAST_SLICE_DIM && data_rank > rank)
	       slicedim[3] = NO_SLICE_DIM;

	  err = arrayh5_read(&contour_data, fname, dname, NULL,
			     4, slicedim, islice, center_slice);
	  slicedim[3] = slicedim3;
	  CHECK(!err, arrayh5_read_strerror[err]);
	  CHECK(contour_data.rank == 1 || contour_data.rank == 2,
	       "contour slice must be one or two dimensional");

	  cnx = contour_data.dims[0];
	  cny = contour_data.rank >= 2 ? contour_data.dims[1] : 1;

	  if (!mask_thresh_set) {
               double c_min, c_max;
               arrayh5_getrange(contour_data, &c_min, &c_max);
	       mask_thresh = (c_min + c_max) * 0.5;
	  }

	  free(fname);
     }

     if (overlay_fname && !collect_range) {
	  int rank;
	  char *fname, *dname;

	  fname = split_fname(overlay_fname, &dname);
	  if (!dname[0])
	       dname = NULL;

	  if (verbose)
	       printf("reading overlay data from \"%s\".\n", fname);

	  err = arrayh5_read_rank(fname, dname, &rank);
          CHECK(!err, arrayh5_read_strerror[err]);
	  if (slicedim3 == LAST_SLICE_DIM && data_rank > rank)
	       slicedim[3] = NO_SLICE_DIM;

	  err = arrayh5_read(&overlay_data, fname, dname, NULL,
			     4, slicedim, islice, center_slice);
	  slicedim[3] = slicedim3;
	  CHECK(!err, arrayh5_read_strerror[err]);
	  CHECK(overlay_data.rank == 1 || overlay_data.rank == 2,
	       "overlay slice must be one or two dimensional");

	  onx = overlay_data.dims[0];
	  ony = overlay_data.rank >= 2 ? overlay_data.dims[1] : 1;

	  free(fname);
     }

     if (verbose)
	  printf("------\n");
     for (ifile = optind; ifile < argc; ++ifile) {
          char *dname, *h5_fname;
          h5_fname = split_fname(argv[ifile], &dname);
          if (!dname[0])
               dname = data_name;

          if (verbose) {
               int i;
               printf("reading from \"%s\"", h5_fname);
               for (i = 0; i < 4; ++i)
                    if (slicedim[i] != NO_SLICE_DIM)
                         printf(", slice at %d in %c dimension", islice[i],
                                slicedim[i] == LAST_SLICE_DIM ? 't'
                                : slicedim[i] + 'x');
               printf(".\n");
          }

	  err = arrayh5_read(&a, h5_fname, dname, NULL,
			     4, slicedim, islice, center_slice);
	  CHECK(!err, arrayh5_read_strerror[err]);
	  CHECK(a.rank >= 1, "data must have at least one dimension");
	  CHECK(a.rank <= 2, "data can have at most two dimensions (try specifying a slice)");

	  if (!png_fname) {
	       char dimname[] = "xyzt", suff[1024] = "";
	       int dim;
	       for (dim = 0; dim < 4; ++dim)
		    if (islice_max[dim] >= islice_min[dim]+islice_step[dim]) {
			 char s[128];
			 sprintf(s, ".%c%0*d", dimname[dim],
				 1 + ilog10(imax(iabs(islice_min[dim]),
						 iabs(islice_max[dim]))),
				 islice[dim]);
			 strcat(suff, s);
		    }
	       strcat(suff, ".png");
	       png_fname = replace_suffix(h5_fname, ".h5", suff);
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
	       if (!num_processed || a_min < allmin)
		    allmin = a_min;
	       if (!num_processed || a_max > allmax)
		    allmax = a_max;
	       if (min > max) {
		    a_min = min;
		    min = max;
		    max = a_min;
	       }
	       if (zero_center) {
		    if (!max_set || min_set || max <= 0)
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

	       writepng(png_fname, nx, ny, !transpose, skew,
			scaley, scalex, a.data,
			contour_fname ? contour_data.data : NULL, mask_thresh,
			cnx, cny,
			overlay_fname ? overlay_data.data : NULL,overlay_cmap,
			onx, ony,
			min, max, cmap, eight_bit);
	  }
	  arrayh5_destroy(a);
	  free(png_fname); png_fname = NULL;
	  free(h5_fname);
	  ++num_processed;
     }

     if (contour_data.data)
	  arrayh5_destroy(contour_data);
     if (overlay_data.data)
	  arrayh5_destroy(overlay_data);
     contour_data.data = overlay_data.data = NULL;

     } /* islice loop */

     if (verbose && num_processed)
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

     free(contour_fname);
     free(overlay_fname);
     free(data_name);

     if (cmap.rgba != gray_colors)
	  free(cmap.rgba);
     free(colormap);

     return EXIT_SUCCESS;
}
