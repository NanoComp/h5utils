/* Copyright (c) 1999-2017 Massachusetts Institute of Technology
 *
 * [ IMPORTANT: Note that the following is DIFFERENT from the license
 *   for the rest of the h5utils package. ]
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include "config.h"
#include "arrayh5.h"
#include "h5utils.h"

/* Vis5d header files: */
#if defined(HAVE_VIS5Dp_V5D_H)
#  include <vis5d+/v5d.h>
#elif defined(HAVE_VIS5D_V5D_H)
#  include <vis5d/v5d.h>
#else
#  include <v5d.h>
#endif

#define CHECK(cond, msg) { if (!(cond)) { fprintf(stderr, "h5tov5d error: %s\n", msg); exit(EXIT_FAILURE); } }

void usage(FILE *f)
{
     fprintf(f, "Usage: h5tov5d [options] [<filenames>]\n"
	     "Options:\n"
	     "         -h : this help message\n"
             "         -V : print version number and copyright\n"
	     "         -v : verbose output\n"
	     "         -T : transposed output dimensions\n"
	     "    -x <ix> : take x=<ix> slice of data\n"
	     "    -y <iy> : take y=<iy> slice of data\n"
	     "    -z <iz> : take z=<iz> slice of data\n"
	     "    -t <it> : take t=<it> slice of data's last dimension\n"
	     "         -0 : use dataset center as origin for -x/-y/-z\n"
	     "  -o <file> : output datasets from all input files to <file>\n"
"   -1,-2,-4 : number of bytes per data point to use in output (default: 1)\n"
	     "              (fewer bytes is faster, but has less resolution)\n"
	     "  -d <name> : use dataset <name> in the input files (default: first dataset)\n"
	     "              -- you can also specify a dataset via <filename>:<name>\n"
	  );
}

/* The following routine was adapted from convert/foo2_to_v5d.c from
   Vis5D 4.2, which is Copyright (C) 1990-1997 Bill Hibbard, Johan
   Kellum, Brian Paul, Dave Santek, and Andre Battaiola, and is
   distributed under the GNU General Public License. */
void output_v5d(char *v5d_fname, char *data_label,
		int nslicedim, const int *slicedim,
		const int *islice, const int *center_slice,
		int store_bytes, int transpose,
		char **h5_fnames, int num_h5, int join)
{
     char *data_name;
     char *fname;
     arrayh5 a;
     int it, iv, firstdim, ifile;
     float *g = 0;

     /** Parameters to v5dCreate: */
     int NumTimes;                      /* number of time steps */
     int NumVars;                       /* number of variables */
     int Nr, Nc, Nl[MAXVARS];           /* size of 3-D grids */
     char VarName[MAXVARS][10];         /* names of variables */
     int TimeStamp[MAXTIMES];           /* real times for each time step */
     int DateStamp[MAXTIMES];           /* real dates for each time step */
     int CompressMode;                  /* number of bytes per grid */
     int Projection;                    /* a projection number */
     float ProjArgs[100];               /* the projection parameters */
     int Vertical;                      /* a vertical coord system number */
     float VertArgs[MAXLEVELS];         /* the vertical coord sys parameters */

     if (num_h5 <= 0)
	  return;

     for (ifile = 0; ifile < num_h5; ++ifile) {
	  int err;
	  fname = split_fname(h5_fnames[ifile], &data_name);
	  if (!data_name[0]) data_name = data_label;
	  err = arrayh5_read(&a, fname, data_name, NULL, 
			     nslicedim, slicedim, islice, center_slice);
	  free(fname);
	  CHECK(!err, arrayh5_read_strerror[err]);
	  CHECK(a.rank >= 1, "data must have at least one dimension");
	  CHECK(a.rank <= 5, "data cannot have more than 5 dimensions");


	  /* if the data is 4 dimensional, express that by using
	     different times and/or variables */
	  NumTimes = a.rank < 4 ? 1 : a.dims[a.rank - 1];
	  CHECK(NumTimes <= MAXTIMES, "too many time steps");
	  firstdim = a.rank <= 4 ? 0 : a.rank - 4;

	  /* If the data is 5 dimensional, express that by using different
	     variables.  Alternatively, if we are joining, the different
	     variables are the different files; in that case, the data
	     cannot be 5d. */
	  NumVars = a.rank < 5 ? 1 : a.dims[0];
	  if (join) {
	       CHECK(NumVars == 1, "cannot join 5d datasets");
	       NumVars = num_h5;
	  }
	  CHECK(NumVars <= MAXVARS, "too many vars");

	  if (!transpose) {
	       /* we will need to transpose the data, since HDF5 gives
		  us the data in row-major order, while Vis5D expects
		  it in column-major order (we could avoid physically
		  transposing the data by passing Vis5d transposed
		  dimensions, but that seems ugly). */
	       Nr = firstdim >= a.rank ? 1 : a.dims[firstdim];
	       Nc = firstdim+1 >= a.rank ? 1 : a.dims[firstdim+1];
	       Nl[0] = firstdim+2 >= a.rank ? 1 : a.dims[firstdim+2];
	  }
	  else {
	       Nr = firstdim+2 >= a.rank ? 1 : a.dims[firstdim+2];
	       Nc = firstdim+1 >= a.rank ? 1 : a.dims[firstdim+1];
	       Nl[0] = firstdim >= a.rank ? 1 : a.dims[firstdim];
	  }

	  if (!v5d_fname) {
	       fname = split_fname(h5_fnames[ifile], &data_name);
	       v5d_fname = replace_suffix(fname, ".h5", ".v5d");
	       free(fname);
	  }

	  if (join && ifile == 0) {
	       arrayh5_destroy(a); /* destroy while we check other datasets */

	       /* loop to assign VarName[] and Nl[] arrays: */
	       for (iv = 0; iv < NumVars; ++iv) {
		    char *name;
		    int numTimes, nr, nc, fdim;

		    fname = split_fname(h5_fnames[iv], &data_name);
		    name =  replace_suffix(fname, ".h5", 
					   data_name[0] ? data_name - 1 : "");
		    if (!data_name[0]) data_name = data_label;

		    for (it = 0; it < 9 && name[it]; ++it)
			 VarName[iv][it] = name[it];
		    VarName[iv][it] = 0;
		    free(name);

		    /* we don't really have to read the whole array;
		       if we called HDF5 routines directly, we could just
		       get the dimensions...oh well */
		    err = arrayh5_read(&a, fname, data_name, NULL, 
				       nslicedim, slicedim,
				       islice, center_slice);
		    CHECK(!err, arrayh5_read_strerror[err]);
		    free(fname);
		    
		    numTimes = a.rank < 4 ? 1 : a.dims[a.rank - 1];
		    fdim = a.rank <= 4 ? 0 : a.rank - 4;
		    if (!transpose) {
			 nr = fdim >= a.rank ? 1 : a.dims[fdim];
			 nc = fdim+1 >= a.rank ? 1 : a.dims[fdim+1];
			 Nl[iv] = fdim+2 >= a.rank ? 1 : a.dims[fdim+2];
		    }
		    else {
			 nr = fdim+2 >= a.rank ? 1 : a.dims[fdim+2];
			 nc = fdim+1 >= a.rank ? 1 : a.dims[fdim+1];
			 Nl[iv] = fdim >= a.rank ? 1 : a.dims[fdim];
		    }
		    CHECK(numTimes == NumTimes && nr == Nr && nc == Nc, 
			  "datasets to be joined must have same dimensions");

		    arrayh5_destroy(a);
	       }

	       /* read first array back in */
	       fname = split_fname(h5_fnames[ifile], &data_name);
	       if (!data_name[0]) data_name = data_label;
	       err = arrayh5_read(&a, fname, data_name, NULL,
				  nslicedim, slicedim, islice, center_slice);
	       CHECK(!err, arrayh5_read_strerror[err]);
	       free(fname);
	  }
	  else if (!join) {
	       if (data_label) {
		    for (it = 0; it < 9 && data_label[it]; ++it)
			 VarName[0][it] = data_label[it];
		    VarName[0][it] = 0;
	       }
	       else {  /* use file name, minus ".v5d" suffix, for var name */
		    int suff = strlen(v5d_fname) - 4;
		    if (suff < 0 || strcmp(v5d_fname + suff, ".v5d"))
			 suff += 4;  /* no ".v5d"; suff = end of string */
		    
		    for (it = 0; it < 9 && it < suff; ++it)
			 VarName[0][it] = v5d_fname[it];
		    VarName[0][it] = 0;
	       }

	       for (iv = 1; iv < NumVars; ++iv) {
		    Nl[iv] = Nl[0];  /* all variables have the same dims */
		    if (iv <= 999999999) /* paranoia: ensure sprintf is safe */
			 sprintf(VarName[iv], "%d", iv);
		    else
			 strcpy(VarName[iv], "Infinity");
	       }
	  }

	  for (it = 0; it < NumTimes; ++it) {
	       TimeStamp[it] = it;
	       DateStamp[it] = 0; /* don't bother to make up a real date */
	  }

	  CompressMode = store_bytes;
	  CHECK(CompressMode == 1 || CompressMode == 2 || CompressMode == 4,
		"can only store v5d data as 1, 2, or 4 bytes!");
	  
	  Projection = 0;  /* linear, rectangular, generic units */
	  ProjArgs[0] = ProjArgs[1] = 0; /* origin of row/col coord system */
	  ProjArgs[2] = ProjArgs[3] = 1; /* coord increment between rows/cols*/

	  Vertical = 0;  /* equally spaced levels in generic units */
	  VertArgs[0] = 0.0;  /* position of bottom level */
	  VertArgs[1] = 1.0;  /* spacing between levels */

	  /* use v5dCreate call to create the v5d file and write the header */
	  if (!join || ifile == 0) {
	       CHECK(v5dCreate(v5d_fname, NumTimes, NumVars, Nr, Nc, Nl,
			       VarName, TimeStamp, DateStamp, CompressMode,
			       Projection, ProjArgs, Vertical, VertArgs),
		     "couldn't create v5d file");
	  }
	  
	  /* may call v5dSetLowLev() or v5dSetUnits() here; see Vis5d README */

	  /* allocate array for copying transpose of data: */
	  g = (float *) malloc(sizeof(float) * Nr * Nc * Nl[0]);
	  CHECK(g, "out of memory!");

	  for (iv = join ? ifile : 0; iv < (join ? ifile + 1 : NumVars); ++iv)
	       for (it = 0; it < NumTimes; ++it) {
		    double *d = a.data + it +
			 (join ? 0 : iv) * NumTimes * Nr * Nc * Nl[0];

		    if (!transpose) {
			 int ir, ic, il;
			 for (ir = 0; ir < Nr; ++ir)
			      for (ic = 0; ic < Nc; ++ic)
				   for (il = 0; il < Nl[0]; ++il)
					g[ir + Nr * (ic + Nc * il)] =
					     d[(il + Nl[0] * (ic + Nc * ir))
					       * NumTimes];
		    }
		    else {
			 int id;
			 for (id = 0; id < Nr * Nc * Nl[0]; ++id)
			      g[id] = d[id * NumTimes];
		    }
		    CHECK(v5dWrite(it + 1, iv + 1, g),
			  "error writing v5d output"); 
	       }
	  
	  free(g);
	  
	  if (!join || ifile == num_h5 - 1)
	       v5dClose();

	  arrayh5_destroy(a);
	  if (v5d_fname)
	       free(v5d_fname);
	  v5d_fname = NULL;
     }
}

int main(int argc, char **argv)
{
     char *v5d_fname = NULL, *data_name = NULL;
     extern char *optarg;
     extern int optind;
     int c;
     int verbose = 0, transpose = 0;
     int slicedim[4] = {NO_SLICE_DIM,NO_SLICE_DIM,NO_SLICE_DIM,NO_SLICE_DIM};
     int islice[4], center_slice[4] = {0,0,0,0};
     int store_bytes = 1;

     while ((c = getopt(argc, argv, "ho:d:vTV124x:y:z:t:0")) != -1)
	  switch (c) {
	      case 'h':
		   usage(stdout);
		   return EXIT_SUCCESS;
	      case 'V':
		   printf("h5tov5d " PACKAGE_VERSION " by Steven G. Johnson\n" 
       "Copyright (c) 1999-2017 Massachusetts Institute of Technology\n"
       "\n"
       "This program is free software; you can redistribute it and/or modify\n"
       "it under the terms of the GNU General Public License as published by\n"
       "the Free Software Foundation; either version 2 of the License, or\n"
       "(at your option) any later version.\n"
       "\n"
       "This program is distributed in the hope that it will be useful,\n"
       "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
       "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
       "GNU General Public License for more details.\n"
			);
		   return EXIT_SUCCESS;
	      case 'v':
		   verbose = 1;
		   break;
	      case 'T':
		   transpose = 1;
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
	      case '1':
		   store_bytes = 1;
		   break;
	      case '2':
		   store_bytes = 2;
		   break;
	      case '4':
		   store_bytes = 4;
		   break;
	      case 'o':
		   v5d_fname = my_strdup(optarg);
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

     output_v5d(v5d_fname, data_name, 
		4, slicedim, islice, center_slice,
		store_bytes, transpose,
		argv + optind, argc - optind, v5d_fname != NULL);

     if (data_name)
	  free(data_name);

     return EXIT_SUCCESS;
}
