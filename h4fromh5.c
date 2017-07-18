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

#include <unistd.h>

#include "config.h"
#include "arrayh5.h"
#include "arrayh4.h"
#include "copyright.h"
#include "h5utils.h"

#define CHECK(cond, msg) { if (!(cond)) { fprintf(stderr, "h4fromh5 error: %s\n", msg); exit(EXIT_FAILURE); } }

void usage(FILE *f)
{
     fprintf(f, "Usage: h4fromh5 [options] [<hdf4-files>]\n"
	     "Options:\n"
	     "         -h : this help message\n"
             "         -V : print version number and copyright\n"
	     "         -v : verbose output\n"
	     "         -T : transposed output\n"
	     "  -o <file> : output to HDF4 file <file>\n"
	     "  -d <name> : use dataset <name> in the input file\n"
	     "              -- you can also specify a dataset via <file>:<name>\n"
	  );
}

int main(int argc, char **argv)
{
     char *h4_fname = NULL;
     char *data_name = NULL;
     extern char *optarg;
     extern int optind;
     int c;
     int ifile;
     int verbose = 0, transpose = 0;

     while ((c = getopt(argc, argv, "hd:vTo:V")) != -1)
	  switch (c) {
	      case 'h':
		   usage(stdout);
		   return EXIT_SUCCESS;
	      case 'V':
		   printf("h4fromh5 " PACKAGE_VERSION " by Steven G. Johnson\n" 
			  COPYRIGHT);
		   return EXIT_SUCCESS;
	      case 'v':
		   verbose = 1;
		   break;
	      case 'T':
		   transpose = 1;
		   break;
	      case 'd':
		   free(data_name);
		   data_name = my_strdup(optarg);
		   break;		   
	      case 'o':
		   free(h4_fname);
		   h4_fname = my_strdup(optarg);
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

     if (h4_fname && optind + 1 < argc) {
	  fprintf(stderr, "h4fromh5: only one .h5 file can be used with -o\n");
	  return EXIT_FAILURE;
     }

     for (ifile = optind; ifile < argc; ++ifile) {
	  char *h5_fname, *dname;
	  arrayh4 a4;
	  int i, err;
	  int32 dims_copy[ARRAYH4_MAX_RANK];
	  char *cur_h4_fname = h4_fname;
	  arrayh5 a;

	  h5_fname = split_fname(argv[ifile], &dname);
	  if (!dname[0])
               dname = data_name;

	  if (!cur_h4_fname)
	       cur_h4_fname = replace_suffix(h5_fname, ".h5", ".hdf");

	  if (verbose)
	       printf("Reading HDF5 input file \"%s\"...\n", h5_fname);
	  err = arrayh5_read(&a, h5_fname, dname, NULL, 0, 0, 0, 0);
	  CHECK(!err, arrayh5_read_strerror[err]);

	  if (transpose)
	       arrayh5_transpose(&a);

	  CHECK(a.rank <= ARRAYH4_MAX_RANK, "HDF5 rank is too big");
	  for (i = 0; i < a.rank; ++i)
	       dims_copy[i] = a.dims[i];

	  CHECK(arrayh4_create(&a4, DFNT_FLOAT64, a.rank, dims_copy),
		"error allocating HDF4 data");
	  
	  for (i = 0; i < a.N; ++i)
	       a4.p.d[i] = a.data[i];

	  if (verbose) {
	       double a_min, a_max;
	       arrayh5_getrange(a, &a_min, &a_max);
	       printf("data ranges from %g to %g.\n", a_min, a_max);
	  }
	  
	  if (verbose) {
	       int i;
	       printf("Writing size %d", a.dims[0]);
	       for (i = 1; i < a.rank; ++i)
		    printf("x%d", a.dims[i]);
	       printf(" data to %s\n", cur_h4_fname);
	  }

	  arrayh5_destroy(a);

	  arrayh4_write(cur_h4_fname, a4);
	  arrayh4_destroy(a4);

	  if (h4_fname != cur_h4_fname)
	       free(cur_h4_fname);
	  free(h5_fname);
     }

     return EXIT_SUCCESS;
}
