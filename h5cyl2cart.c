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

#include <unistd.h>

#include "config.h"
#include "arrayh5.h"
#include "copyright.h"
#include "h5utils.h"

#define CHECK(cond, msg) { if (!(cond)) { fprintf(stderr, "h5totxt error: %s\n", msg); exit(EXIT_FAILURE); } }

void usage(FILE *f)
{
     fprintf(f, "Usage: h5cyl2cart [options] [<filenames>]\n"
	     "Options:\n"
	     "         -h : this help message\n"
             "         -V : print version number and copyright\n"
	     "         -v : verbose output\n"
	     "     -m <m> : for complex data, multiply by exp(i m phi)\n"
	     "  -o <file> : output to <file> (first input file only)\n"
	     "     -r <r> : radial coordinate starts at <r> (default: 0)\n"
	     "  -d <name> : use dataset <name> in the input files (default: first dataset)\n"
	     "              -- you can also specify a dataset via <filename>:<name>\n"
	     "              -- nonzero <m> implies complex data <name>.[ri]\n"
	     "                 or alternatively -i can be used\n"
	     "  -i <name> : imaginary dataset name\n"
	  );
}

void cyl2cart(arrayh5 ar, arrayh5 ai, int m, arrayh5 *cr_, arrayh5 *ci_)
{
     arrayh5 cr, ci;
     int nx,ny,nz,nr, dims[3], ix,iy,iz;
     double *dcr, *dci, *dar, *dai;
     
     nz = ar.rank < 1 ? 1 : ar.dims[0];
     nr = ar.rank < 2 ? 1 : ar.dims[1];
     nx = ny = 2*nr - 1;
     dims[0] = nx; dims[1] = ny; dims[2] = nz;
     *cr_ = cr = arrayh5_create(3, dims);
     *ci_ = ci = arrayh5_create(3, dims);
     dcr = cr.data; dci = ci.data;
     dar = ar.data; dai = ai.data;

     for (ix = 0; ix < nx; ++ix)
      for (iy = 0; iy < ny; ++iy) 
       for (iz = 0; iz < nz; ++iz) {
	    double x = ix - (nr - 1);
	    double y = iy - (nr - 1);
	    double p = atan2(y, x), r = sqrt(x*x + y*y);
	    int ir = (int) r; /* round down */
	    double cm = cos(m*p), sm = sin(m*p);
	    double re, im;
	    if (ir == nr-1 && r-ir < 1e-8) {
		 re = dar[ir*nz+iz];
		 im = dai[ir*nz+iz];
	    }
	    else if (ir >= nr-1) { /* r is outside the a array */
		 re = im = 0.0;
	    }
	    else { /* linearly interpolate between ir and ir+1 */
		 double dr = r - ir;
		 re = dar[ir+nr*iz]*(1-dr) + dr*dar[(ir+1)+nr*iz];
		 im = dai[ir+nr*iz]*(1-dr) + dr*dai[(ir+1)+nr*iz];
	    }
	    /* (re + i*im) * (cm + i*sm) */
	    dcr[(ix*ny+iy)*nz+iz] = re*cm - im*sm;
	    dci[(ix*ny+iy)*nz+iz] = re*sm + im*cm;
       }
}


int main(int argc, char **argv)
{
     arrayh5 ar, ai, cr, ci;
     char *out_fname = NULL, *data_name = NULL, *data_name_i = NULL;
     extern char *optarg;
     extern int optind;
     int c;
     int err;
     int verbose = 0;
     int m = 0;
     int ifile;

     while ((c = getopt(argc, argv, "hVvm:o:r:d:i:")) != -1)
	  switch (c) {
	      case 'h':
		   usage(stdout);
		   return EXIT_SUCCESS;
	      case 'V':
		   printf("h5cyl2cart " PACKAGE_VERSION 
			  " by Steven G. Johnson\n" 
			  COPYRIGHT);
		   return EXIT_SUCCESS;
	      case 'v':
		   verbose = 1;
		   break;
	      case 'm':
		   m = atoi(optarg);
		   break;
	      case 'o':
		   free(out_fname);
                   out_fname = my_strdup(optarg);
                   break;
	      case 'd':
		   free(data_name);
		   data_name = my_strdup(optarg);
		   break;		   
	      case 'i':
		   free(data_name_i);
		   data_name_i = my_strdup(optarg);
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
	  short append_data = 0;
	  char *dname, *dnamei, *h5_fname;
	  h5_fname = split_fname(argv[ifile], &dname);
	  if (!dname[0]) dname = data_name;
	  if (dname && !dname[0]) dname = NULL;
	  if (dname && m != 0 && !data_name_i) { /* append ".r" if needed */
	       size_t len = strlen(dname);
	       if (dname[len-2] != '.' ||
		   (dname[len-1] != 'r' && dname[len-1] != 'i')) {
		    char *tmp = (char*) malloc(sizeof(char) * (len+3));
		    strcpy(tmp, dname);
		    strcat(tmp, ".r");
		    dname = tmp;
	       }
	       else {
		    dname = my_strdup(dname);
		    if (dname[len-1] == 'i') 
			 dname[len-1] = 'r'; /* read real part first */
	       }
	  }
	  else if (dname)
	       dname = my_strdup(dname);

     read_ar:
	  if (verbose)
	       printf("reading from %s in \"%s\"\n", dname?dname:"?",h5_fname);
	  
	  err = arrayh5_read(&ar, h5_fname, dname, &dnamei,
			     0, NULL, NULL, NULL);
	  if (!dname) {
	       dname = strdup(dnamei);
	       if (verbose) printf("found dataset %s\n", dname);
	       if (!data_name_i && m != 0) {
		    size_t len = strlen(dname);
		    if (dname[len-2] == '.' && dname[len-1] == 'i') {
			 dname[len-1] = 'r';
			 goto read_ar;
		    }
	       }
	  }
	  CHECK(!err, arrayh5_read_strerror[err]);
	  CHECK(ar.rank <= 2, "input data must be < 3 dimensional");

	  if (m == 0)
	       ai = arrayh5_clone(ar);
	  else {
	       if (data_name_i) {
		    free(dnamei);
		    dnamei = strdup(data_name_i);
	       }
	       else {
		    size_t len = strlen(dnamei);
		    CHECK(len >= 2 && dnamei[len-2] == '.' 
			  && dnamei[len-1] == 'r',
			  "require .r and .i datanames for nonzero m");
		    dnamei[len-1] = 'i';
	       }
	       
	       if (verbose)
		    printf("reading from %s in \"%s\"\n", dnamei, h5_fname);
	  
	       err = arrayh5_read(&ai, h5_fname, dnamei, NULL,
				  0, NULL, NULL, NULL);
	       CHECK(!err, arrayh5_read_strerror[err]);
	       CHECK(arrayh5_conformant(ar, ai),
		     "real and imaginary data sets must be the same size");

	  }

	  if (!out_fname) {
	       char *tmp;
	       out_fname = strdup(h5_fname);
	       append_data = 1;

	       tmp = (char*) malloc(sizeof(char) * (strlen(dname)+6));
	       strcpy(tmp, "cart-"); strcat(tmp, dname);
	       free(dname); dname = tmp;

	       tmp = (char*) malloc(sizeof(char) * (strlen(dnamei)+6));
	       strcpy(tmp, "cart-"); strcat(tmp, dnamei);
	       free(dnamei); dnamei = tmp;
	  }

	  cyl2cart(ar, ai, m, &cr, &ci);

	  if (verbose)
	       printf("writing %s from %dx%d input data.\n",
		      out_fname, (cr.dims[0]+1)/2, cr.dims[2]);

	  arrayh5_write(cr, out_fname, dname, append_data);
	  if (m != 0) arrayh5_write(ci, out_fname, dnamei, 1);

	  arrayh5_destroy(ar);
	  arrayh5_destroy(ai);
	  arrayh5_destroy(cr);
	  arrayh5_destroy(ci);
	  free(h5_fname);
	  free(out_fname); out_fname = NULL;
	  free(dname); free(dnamei);
     }
     free(data_name);
     free(data_name_i);

     return EXIT_SUCCESS;
}
