/* Copyright (c) 1999-2023 Massachusetts Institute of Technology
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

#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#endif

#if defined(HAVE_ARPA_INET_H)
#  include <arpa/inet.h>
#elif defined(HAVE_NETINET_IN_H) 
#  include <netinet/in.h>
#endif

#include "arrayh5.h"
#include "copyright.h"
#include "h5utils.h"

#ifdef HAVE_UINT16_T
typedef uint16_t my_uint16_t;
#else
typedef unsigned short my_uint16_t;
#endif
#ifdef HAVE_UINT32_T
typedef uint32_t my_uint32_t;
#else
typedef unsigned long my_uint32_t;
#endif

#define CHECK(cond, msg) { if (!(cond)) { fprintf(stderr, "h5tovtk error: %s\n", msg); exit(EXIT_FAILURE); } }

void usage(FILE *f)
{
     fprintf(f, "Usage: h5tovtk [options] [<filenames>]\n"
	     "Options:\n"
	     "         -h : this help message\n"
             "         -V : print version number and copyright\n"
	     "         -v : verbose output\n"
	     "  -o <file> : output datasets from all input files to <file>;\n"
	     "              combines 3 datasets to a vector, and 2 or 4+ to a field\n"
	     "         -4 : 4-byte floating-point binary output (default)\n"
	     "         -a : ASCII floating-point output\n"
	     "         -1 : 1-byte (0-255) binary output\n"
	     "         -2 : 2-byte (0-65535) binary output\n"
	     "         -n : don't convert binary output to bigendian\n"
	     "   -m <min> : set bottom of scale for 1/2 byte encoding\n"
	     "   -M <max> : set top of scale for 1/2 byte encoding\n"
	     "         -Z : center scale at zero for 1/2 byte encoding\n"
	     "         -r : invert scale & data values\n"
	     "    -x <ix> : take x=<ix> slice of data\n"
	     "    -y <iy> : take y=<iy> slice of data\n"
	     "    -z <iz> : take z=<iz> slice of data\n"
	     "    -t <it> : take t=<it> slice of data's last dimension\n"
	     "         -0 : use dataset center as origin for -x/-y/-z\n"
	     "  -d <name> : use dataset <name> in the input files (default: first dataset)\n"
	     "              -- you can also specify a dataset via <filename>:<name>\n"
	  );
}

static void whitespace_to_underscores(char *s)
{
     while (*s) {
	  if (isspace(*s))
	       *s = '-';
	  ++s;
     }
}

static const char vtk_datatype[][20] = { 
     "float", "unsigned_char", "unsigned_short", "none", "float"
};

static void write_vtk_header(FILE *f, int is_binary,
			     int nx, int ny, int nz,
			     double ox, double oy, double oz,
			     double sx, double sy, double sz)
{
     fprintf(f, 
	     "# vtk DataFile Version 2.0\n"
	     "Generated by h5tovtk.\n"
	     "%s\n"
	     "DATASET STRUCTURED_POINTS\n"
	     "DIMENSIONS %d %d %d\n"
	     "ORIGIN %g %g %g\n"
	     "SPACING %g %g %g\n",
	     is_binary ? "BINARY" : "ASCII",
	     nx, ny, nz,
	     ox, oy, oz, sx, sy, sz);
}

static void write_vtk_value(FILE *f, double v, int store_bytes, int fix_bytes,
			    double min, double max, int invert)
{
     if (invert)
	  v = max - (v - min);
     switch (store_bytes) {
	 case 0:
	      fprintf(f, "%g ", v);
	      break;
	 case 1:
	 {
	      unsigned char c;
	      c = floor((v - min) * 255.0 / (max - min) + 0.5);
	      fwrite(&c, 1, 1, f);
	      break;
	 }
	 case 2:
	 {
	      my_uint16_t i;
	      i = floor((v - min) * 65535.0 / (max - min) + 0.5);
	      if (fix_bytes) {
#if defined(HAVE_HTONS)
		   i = htons(i);
#elif ! defined(WORDS_BIGENDIAN)
		   unsigned char swap, *bytes;
		   bytes = (unsigned char *) &i;
		   swap = bytes[0]; bytes[0] = bytes[1]; bytes[1] = swap;
#endif
	      }
	      fwrite(&i, 2, 1, f);
	      break;
	 }
	 case 4:
	 {
	      float fv = v;
	      if (fix_bytes) {
#if defined(HAVE_HTONL) && (SIZEOF_FLOAT == 4)
		   my_uint32_t *i = (my_uint32_t *) &fv;
		   *i = htonl(*i);
#elif ! defined(WORDS_BIGENDIAN)
		   unsigned char swap, *bytes;
		   bytes = (unsigned char *) &fv;
		   swap = bytes[0]; bytes[0] = bytes[3]; bytes[3] = swap;
		   swap = bytes[1]; bytes[1] = bytes[2]; bytes[2] = swap;
#endif
	      }
	      fwrite(&fv, 4, 1, f);
	      break;
	 }
     }
}

int main(int argc, char **argv)
{
     arrayh5 *a = NULL;
     char *vtk_fname = NULL, *data_name = NULL;
     extern char *optarg;
     extern int optind;
     double ox = 0, oy = 0, oz = 0, sx = 1, sy = 1, sz = 1;
     int c, ifile;
     int zero_center = 0;
     int invert = 0;
     double min = 0, max = 0;
     int min_set = 0, max_set = 0;
     int verbose = 0, combine = 0;
     int slicedim[4] = {NO_SLICE_DIM,NO_SLICE_DIM,NO_SLICE_DIM,NO_SLICE_DIM};
     int islice[4], center_slice[4] = {0,0,0,0};
     int nx = 0, ny = 0, nz = 0, na;
     int store_bytes = 4, fix_byte_order = 1;

     while ((c = getopt(argc, argv, "ho:d:vV124mMZranx:y:z:t:0")) != -1)
	  switch (c) {
	      case 'h':
		   usage(stdout);
		   return EXIT_SUCCESS;
	      case 'V':
		   printf("h5tovtk " PACKAGE_VERSION " by Steven G. Johnson\n" 
			  COPYRIGHT);
		   return EXIT_SUCCESS;
	      case 'v':
		   verbose = 1;
		   break;
	      case 'x':
		   islice[0] = atoi(optarg);
		   slicedim[0] = 0;
		   break;
	      case 'y':
		   islice[1] = atoi(optarg);
		   slicedim[1] = 1;
		   break;
	      case 'z':
		   islice[2] = atoi(optarg);
		   slicedim[2] = 2;
		   break;
	      case 't':
		   islice[3] = atoi(optarg);
		   slicedim[3] = LAST_SLICE_DIM;
		   break;
              case '0':
                   center_slice[0] = center_slice[1] = center_slice[2] = 1;
                   break;
	      case 'n':
		   fix_byte_order = 0;
		   break;
	      case 'a':
		   store_bytes = 0; /* ascii */
		   break;
	      case '1':
		   store_bytes = 1;
		   break;
	      case '2':
		   store_bytes = 2;
		   break;
	      case '4':
		   store_bytes = 4;
		   break;
	      case 'm':
		   min = atof(optarg);
		   min_set = 1;
		   break;
	      case 'M':
		   max = atof(optarg);
		   max_set = 1;
		   break;
	      case 'Z':
		   zero_center = 1;
		   break;
	      case 'r':
		   invert = 1;
		   break;
	      case 'o':
		   vtk_fname = my_strdup(optarg);
		   combine = 1;
		   break;
	      case 'd':
		   data_name = my_strdup(optarg);
		   break;		   
	      default:
		   fprintf(stderr, "Invalid argument -%c\n", c);
		   usage(stderr);
		   return EXIT_FAILURE;
	  }
     if (optind == argc) {  /* no parameters left */
	  usage(stderr);
	  return EXIT_FAILURE;
     }

     CHECK(store_bytes != 4 || sizeof(float) == 4, 
	   "'float' is wrong size for -4");
     CHECK(store_bytes != 4 || sizeof(my_uint32_t) == 4, 
	   "missing 4-byte integer type for -4");
     CHECK(store_bytes != 2 || sizeof(my_uint16_t) == 2, 
	   "missing 2-byte integer type for -2");
     
     a = (arrayh5*) malloc(sizeof(arrayh5) * (na = argc - optind));
     CHECK(a, "out of memory");

     combine = combine && (na > 1);

     for (ifile = optind; ifile < argc; ++ifile) {
          char *dname, *found_dname, *h5_fname;
	  int err, ia = ifile - optind;
          h5_fname = split_fname(argv[ifile], &dname);
          if (!dname[0])
               dname = data_name;

	  err = arrayh5_read(&a[ia], h5_fname, dname, &found_dname,
			     4, slicedim, islice, center_slice);
	  CHECK(!err, arrayh5_read_strerror[err]);
	  CHECK(a[ia].rank >= 1, "data must have at least one dimension");
	  CHECK(a[ia].rank <= 3, "data can have at most 3 dimensions (try taking a slice");
	  
	  CHECK(!combine || !ia || arrayh5_conformant(a[ia], a[0]),
		"all arrays must be conformant to combine them");
	  
	  if (!vtk_fname)
	       vtk_fname = replace_suffix(h5_fname, ".h5", ".vtk");

	  {
	       double a_min, a_max;
	       arrayh5_getrange(a[ia], &a_min, &a_max);
	       if (verbose)
		    printf("data in %s ranges from %g to %g.\n", 
			   h5_fname, a_min, a_max);
	       if (!min_set)
		    min = (!combine || !ia || a_min < min) ? a_min : min;
	       if (!max_set)
		    max = (!combine || !ia || a_max > max) ? a_max : max;
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
	  
	  nx = a[ia].dims[0];
	  ny = a[ia].rank < 2 ? 1 : a[ia].dims[1];
	  nz = a[ia].rank < 3 ? 1 : a[ia].dims[2];
	  
	  if (!combine) {
	       FILE *f;
	       int ix, iy, iz, N = nx * ny * nz;

	       if (verbose)
		    printf("writing \"%s\" from %dx%dx%d input data.\n",
			   vtk_fname, nx, ny, nz);
	       
	       if (strcmp(vtk_fname, "-")) {
		    f = fopen(vtk_fname, "w");
		    CHECK(f, "error creating file");
	       }
	       else
		    f = stdout;

	       write_vtk_header(f, store_bytes, 
				nx, ny, nz, ox, oy, oz, sx, sy, sz);
	       whitespace_to_underscores(found_dname);
	       fprintf(f, "POINT_DATA %d\n"
		       "SCALARS %s %s 1\n"
		       "LOOKUP_TABLE default\n",
		       N, found_dname, vtk_datatype[store_bytes]);
	       
	       for (iz = 0; iz < nz; ++iz)
	       for (iy = 0; iy < ny; ++iy)
	       for (ix = 0; ix < nx; ++ix) {
		    int i = (ix*ny + iy)*nz + iz;
		    write_vtk_value(f, a[ia].data[i], store_bytes,
				    fix_byte_order, min, max, invert);
	       }
	  
	       if (f != stdout)
		    fclose(f);
	       arrayh5_destroy(a[ia]);
	       free(vtk_fname); vtk_fname = NULL;
	  }
	  free(found_dname);
	  free(h5_fname);
     }

     if (combine) {
	  FILE *f;
	  int ix, iy, iz, N = nx * ny * nz;

	  if (verbose)
	       printf("writing \"%s\" from %dx%dx%d input data.\n",
		      vtk_fname, nx, ny, nz);
	  
	  if (strcmp(vtk_fname, "-")) {
	       f = fopen(vtk_fname, "w");
	       CHECK(f, "error creating file");
	  }
	  else
	       f = stdout;
	  
	  write_vtk_header(f, store_bytes, 
			   nx, ny, nz, ox, oy, oz, sx, sy, sz);
	  fprintf(f, "POINT_DATA %d\n", N);
	  switch (na) {
	      case 1:
		   fprintf(f, "SCALARS scalars %s 1\nLOOKUP_TABLE default\n", 
			   vtk_datatype[store_bytes]);
		   break;
	      case 3:
		   fprintf(f, "VECTORS vectors %s\n", 
			   vtk_datatype[store_bytes]);
		   break;
	      default:
		   fprintf(f, "FIELD fields 1\narray %d %d %s\n", 
			   na, N, vtk_datatype[store_bytes]);
	  }
	  
	  for (iz = 0; iz < nz; ++iz)
	  for (iy = 0; iy < ny; ++iy)
	  for (ix = 0; ix < nx; ++ix) {
               int ia, i = (ix*ny + iy)*nz + iz;
	       for (ia = 0; ia < na; ++ia)
		    write_vtk_value(f, a[ia].data[i], store_bytes, 
				    fix_byte_order, min, max, invert);
	  }
	  if (f != stdout)
	       fclose(f);
	  {
	       int ia;
	       for (ia = 0; ia < na; ++ia)
		    arrayh5_destroy(a[ia]);
	  }
     }

     free(a);

     if (data_name)
	  free(data_name);

     return EXIT_SUCCESS;
}
