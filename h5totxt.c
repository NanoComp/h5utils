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

#include <unistd.h>

#include "config.h"
#include "arrayh5.h"
#include "copyright.h"
#include "h5utils.h"

#define CHECK(cond, msg) { if (!(cond)) { fprintf(stderr, "h5totxt error: %s\n", msg); exit(EXIT_FAILURE); } }

void usage(FILE *f)
{
     fprintf(f, "Usage: h5totxt [options] [<filenames>]\n"
	     "Options:\n"
	     "         -h : this help message\n"
             "         -V : print version number and copyright\n"
	     "         -v : verbose output\n"
	     "   -s <sep> : use <sep> to separate columns [ default: \",\" ]\n"
	     "  -o <file> : output to <file> (first input file only)\n"
	     "    -x <ix> : take x=<ix> slice of data\n"
	     "    -y <iy> : take y=<iy> slice of data\n"
	     "    -z <iz> : take z=<iz> slice of data [ default: z=0 ]\n"
	     "         -0 : use dataset center as origin for -x/-y/-z\n"
	     "         -T : transpose the data [default: no]\n"
	     "  -d <name> : use dataset <name> in the input files (default: first dataset)\n"
	     "              -- you can also specify a dataset via <filename>:<name>\n"
	  );
}

int main(int argc, char **argv)
{
     arrayh5 a;
     char *txt_fname = NULL, *data_name = NULL;
     extern char *optarg;
     extern int optind;
     int c;
     int slicedim = 2, islice = 0, center_slice = 0;
     int err;
     int nx, ny;
     int verbose = 0;
     int transpose = 0;
     char *sep;
     int ifile;

     sep = (char*) malloc(sizeof(char) * 2);
     CHECK(sep, "out of memory");
     strcpy(sep, ",");

     while ((c = getopt(argc, argv, "ho:x:y:z:0d:vTs:V")) != -1)
	  switch (c) {
	      case 'h':
		   usage(stdout);
		   return EXIT_SUCCESS;
	      case 'V':
		   printf("h5totxt " PACKAGE_VERSION " by Steven G. Johnson\n" 
			  COPYRIGHT);
		   return EXIT_SUCCESS;
	      case 'v':
		   verbose = 1;
		   break;
              case '0':
                   center_slice = 1;
                   break;
	      case 'T':
		   transpose = 1;
		   break;
	      case 'o':
		   free(txt_fname);
                   txt_fname = (char*) malloc(sizeof(char) *
                                              (strlen(optarg) + 1));
                   CHECK(txt_fname, "out of memory");
                   strcpy(txt_fname, optarg);
                   break;
	      case 's':
		   free(sep);
		   sep = (char*) malloc(sizeof(char) *
					(strlen(optarg) + 1));
		   CHECK(sep, "out of memory");
		   strcpy(sep, optarg);
		   break;
	      case 'd':
		   free(data_name);
		   data_name = (char*) malloc(sizeof(char) *
					      (strlen(optarg) + 1));
		   CHECK(data_name, "out of memory");
		   strcpy(data_name, optarg);
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
	  
	  {
	       double a_min, a_max;
	       arrayh5_getrange(a, &a_min, &a_max);
	       if (verbose)
		    printf("data ranges from %g to %g.\n", a_min, a_max);
	  }
	  
	  nx = a.dims[0];
	  ny = a.rank < 2 ? 1 : a.dims[1];
	  
	  if (verbose)
	       printf("writing %s from %dx%d input data.\n",
		      txt_fname ? txt_fname : "to stdout", nx, ny);

	  {
	       FILE *f;
	       int i, j;

	       if (txt_fname) {
		    f = fopen(txt_fname, "w");
		    CHECK(f, "error creating file");
	       }
	       else
		    f = stdout;
	       
	       if (transpose)
		    for (j = 0; j < ny; ++j) {
			 if (nx > 0)
			      fprintf(f, "%g", a.data[0*ny + j]);
			 for (i = 1; i < nx; ++i)
			      fprintf(f, "%s%g", sep, a.data[i*ny + j]);
			 fprintf(f, "\n");
		    }
	       else
		    for (i = 0; i < nx; ++i) {
			 if (ny > 0)
			      fprintf(f, "%g", a.data[i*ny + 0]);
			 for (j = 1; j < ny; ++j)
			      fprintf(f, "%s%g", sep, a.data[i*ny + j]);
			 fprintf(f, "\n");
		    }

	       if (txt_fname)
		    fclose(f);
	  }

	  arrayh5_destroy(a);
	  if (txt_fname)
	       free(txt_fname);
	  txt_fname = NULL;
	  free(h5_fname);
     }
     free(sep);
     free(data_name);

     return EXIT_SUCCESS;
}
