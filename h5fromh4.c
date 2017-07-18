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

#define CHECK(cond, msg) { if (!(cond)) { fprintf(stderr, "h5fromh4 error: %s\n", msg); exit(EXIT_FAILURE); } }

void usage(FILE *f)
{
     fprintf(f, "Usage: h5fromh4 [options] [<hdf4-files>]\n"
	     "Options:\n"
	     "         -h : this help message\n"
             "         -V : print version number and copyright\n"
	     "         -v : verbose output\n"
	     "  -o <file> : output to HDF5 file <file>\n"
             "         -a : append to existing hdf5 file\n"
	     "  -d <name> : use dataset <name> in the output file (default: \"data\")\n"
	     "              -- you can also specify a dataset via <file>:<name>\n"
	  );
}

int main(int argc, char **argv)
{
     char *dname, *h5_fname = NULL;
     char *data_name = NULL;
     extern char *optarg;
     extern int optind;
     int c;
     int ifile;
     int verbose = 0;
     int append = 0;

     while ((c = getopt(argc, argv, "hd:vo:aV")) != -1)
	  switch (c) {
	      case 'h':
		   usage(stdout);
		   return EXIT_SUCCESS;
	      case 'V':
		   printf("h5fromh4 " PACKAGE_VERSION " by Steven G. Johnson\n" 
			  COPYRIGHT);
		   return EXIT_SUCCESS;
	      case 'v':
		   verbose = 1;
		   break;
	      case 'a':
		   append = 1;
		   break;
	      case 'd':
		   free(data_name);
		   data_name = my_strdup(optarg);
		   break;		   
	      case 'o':
		   free(h5_fname);
		   h5_fname = split_fname(optarg, &dname);
		   if (dname[0]) {
			free(data_name);
			data_name = dname;
		   }
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

     for (ifile = optind; ifile < argc; ++ifile) {
	  char *h4_fname = argv[ifile];
	  arrayh4 a4;
	  int i, dims_copy[ARRAYH4_MAX_RANK];
	  char *cur_h5_fname = h5_fname;
	  arrayh5 a;

	  if (!cur_h5_fname)
	       cur_h5_fname = replace_suffix(h4_fname, ".hdf", ".h5");

	  /* If we specified -o (to concatenate several HDF4 files into
	     a single HDF5 file) and if there is more than one filename
	     argument, use the filename (minus ".hdf") as the dataset name. */
	  if (h5_fname && optind + 1 < argc) {
	       dname = my_strdup(h4_fname);
	       /* remove ".hdf" from dataset name: */
	       if (strlen(dname) >= strlen(".hdf") &&
		   !strcmp(dname + strlen(dname)-strlen(".hdf"),
			   ".hdf"))
		    dname[strlen(dname) - strlen(".hdf")] = 0;
	  }
	  else {
	       dname = data_name;
	       if (!dname)
		    dname = my_strdup("data");
	  }
	  
	  if (verbose)
	       printf("Reading HDF4 input file \"%s\"...\n", h4_fname);
	  CHECK(arrayh4_read(h4_fname, &a4, 0), "error reading HDF4 file");

	  for (i = 0; i < a4.rank; ++i)
	       dims_copy[i] = a4.dims[i];

	  a = arrayh5_create(a4.rank, dims_copy);
	  
	  if (a4.numtype == DFNT_FLOAT64) {
	       for (i = 0; i < a4.N; ++i)
		    a.data[i] = a4.p.d[i];
	  }
	  else if (a4.numtype == DFNT_FLOAT32) {
	       for (i = 0; i < a4.N; ++i)
		    a.data[i] = a4.p.f[i];
	  }
	  else {
	       CHECK(0, "unknown HDF4 numeric type");
	  }

	  arrayh4_destroy(a4);
	  
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
	       printf(" data to %s:%s\n", cur_h5_fname, dname);
	  }

	  arrayh5_write(a, cur_h5_fname, dname, 
			append || (h5_fname && ifile > optind));
	  arrayh5_destroy(a);

	  if (h5_fname != cur_h5_fname)
	       free(cur_h5_fname);
	  if (dname != data_name)
	       free(dname);
     }

     return EXIT_SUCCESS;
}
