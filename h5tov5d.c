/* Copyright (c) 1999 Massachusetts Institute of Technology
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

/* Vis5d header files: */
#include <binio.h>
#include <v5d.h>

#define CHECK(cond, msg) { if (!(cond)) { fprintf(stderr, "h5tov5d error: %s\n", msg); exit(EXIT_FAILURE); } }

void usage(FILE *f)
{
     fprintf(f, "Usage: h5tov5d [options] [<filenames>]\n"
	     "Options:\n"
	     "         -h : this help message\n"
             "         -V : print version number and copyright\n"
	     "         -v : verbose output\n"
	     "  -o <file> : output to <file> (first input file only)\n"
"   -1,-2,-4 : number of bytes per data point to use in output (default: 1)\n"
	     "              (fewer bytes is faster, but has less resolution)\n"
	     "  -d <name> : use dataset <name> in the input file (default: first dataset)\n"
	  );
}

/* The following routine was adapted from convert/foo2_to_v5d.c from
   Vis5D 4.2, which is Copyright (C) 1990-1997 Bill Hibbard, Johan
   Kellum, Brian Paul, Dave Santek, and Andre Battaiola, and is
   distributed under the GNU General Public License. */
void output_v5d(char *v5d_fname, char *data_label, arrayh5 a,
		int store_bytes)
{
     int it, iv, firstdim;
     float *g;

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

     NumVars = 1;

     /* if the data is 4 or 5 dimensional, express that by using
	different times and/or variables */
     NumTimes = a.rank < 4 ? 1 : a.dims[a.rank - 4];
     NumVars = a.rank < 5 ? 1 : a.dims[a.rank - 5];
     CHECK(NumTimes <= MAXTIMES, "too many time steps");
     CHECK(NumVars <= MAXVARS, "too many vars");
     firstdim = a.rank <= 3 ? 0 : a.rank - 3;

     /* we will need to transpose the data, since HDF5 gives us the
	data in row-major order, while Vis5D expects it in
	column-major order (we could avoid physically transposing the
	data by passing Vis5d transposed dimensions, but that seems
	ugly. */
     
     Nr = firstdim >= a.rank ? 1 : a.dims[firstdim];
     Nc = firstdim+1 >= a.rank ? 1 : a.dims[firstdim+1];
     Nl[0] = firstdim+2 >= a.rank ? 1 : a.dims[firstdim+2];

     if (data_label) {
	  for (it = 0; it < 9 && data_label[it]; ++it)
	       VarName[0][it] = data_label[it];
	  VarName[0][it] = 0;
     }
     else {  /* use file name, minus ".v5d" suffix, for var name */
	  int suff = strlen(v5d_fname) - 4;
	  if (suff < 0 || strcmp(v5d_fname + suff, ".v5d"))
	       suff += 4;  /* didn't find ".v5d"; suff = end of string */

	  for (it = 0; it < 9 && it < suff; ++it)
	       VarName[0][it] = v5d_fname[it];
	  VarName[0][it] = 0;
     }

     for (iv = 1; iv < NumVars; ++iv) {
	  Nl[iv] = Nl[0];  /* all variables have the same dimensions */
	  if (iv <= 999999999) /* paranoia: ensure sprintf is safe */
	       sprintf(VarName[iv], "%d", iv);
	  else
	       strcpy(VarName[iv], "Infinity");
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
     ProjArgs[2] = ProjArgs[3] = 1; /* coord increment between rows/cols */

     Vertical = 0;  /* equally spaced levels in generic units */
     VertArgs[0] = 0.0;  /* position of bottom level */
     VertArgs[1] = 1.0;  /* spacing between levels */

     /* use the v5dCreate call to create the v5d file and write the header */
     CHECK(v5dCreate(v5d_fname, NumTimes, NumVars, Nr, Nc, Nl,
		     VarName, TimeStamp, DateStamp, CompressMode,
		     Projection, ProjArgs, Vertical, VertArgs),
	   "couldn't create v5d file");

     /* may call v5dSetLowLev() or v5dSetUnits() here; see Vis5d README */

     /* allocate array for copying transpose of data: */
     g = (float *) malloc(sizeof(float) * Nr * Nc * Nl[0]);
     CHECK(g, "out of memory!");

     for (iv = 0; iv < NumVars; ++iv)
	  for (it = 0; it < NumTimes; ++it) {
	       double *d = a.data + (iv * NumTimes + it) * Nr * Nc * Nl[0];
	       int ir, ic, il;

	       for (ir = 0; ir < Nr; ++ir)
		    for (ic = 0; ic < Nc; ++ic)
			 for (il = 0; il < Nl[0]; ++il)
			      g[ir + Nr * (ic + Nc * il)] =
				   d[il + Nl[0] * (ic + Nc * ir)];
	       CHECK(v5dWrite(it + 1, iv + 1, g), "error writing v5d output"); 
	  }

     free(g);

     v5dClose();
}

int main(int argc, char **argv)
{
     arrayh5 a;
     char *v5d_fname = NULL, *data_name = NULL;
     extern char *optarg;
     extern int optind;
     int c;
     int err;
     int verbose = 0;
     int ifile;
     int store_bytes = 1;

     while ((c = getopt(argc, argv, "ho:d:vV124")) != -1)
	  switch (c) {
	      case 'h':
		   usage(stdout);
		   return EXIT_SUCCESS;
	      case 'V':
		   printf("h5tov5d " VERSION " by Steven G. Johnson\n" 
       "Copyright (c) 1999 Massachusetts Institute of Technology\n"
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
		   v5d_fname = (char*) malloc(sizeof(char) *
					      (strlen(optarg) + 1));
		   CHECK(v5d_fname, "out of memory");
		   strcpy(v5d_fname, optarg);
		   break;
	      case 'd':
		   data_name = (char*) malloc(sizeof(char) *
					      (strlen(optarg) + 1));
		   CHECK(data_name, "out of memory");
		   strcpy(data_name, optarg);
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
	  if (verbose)
	       printf("reading from \"%s\"\n", argv[ifile]);
	  
	  err = arrayh5_read(&a, argv[ifile], data_name, -1, 0);
	  CHECK(!err, arrayh5_read_strerror[err]);
	  CHECK(a.rank >= 1, "data must have at least one dimension");
	  
	  if (!v5d_fname) {
	       v5d_fname = (char *) malloc(sizeof(char) * 
					   (strlen(argv[ifile]) + 5));
	       strcpy(v5d_fname, argv[ifile]);
	       
	       /* remove ".h5" from filename: */
	       if (strlen(v5d_fname) >= strlen(".h5") &&
		   !strcmp(v5d_fname + strlen(v5d_fname) - strlen(".h5"),
			   ".h5"))
		    v5d_fname[strlen(v5d_fname) - strlen(".h5")] = 0;
	       
	       /* add ".v5d": */
	       strcat(v5d_fname, ".v5d");
	  }

	  {
	       double a_min, a_max;
	       arrayh5_getrange(a, &a_min, &a_max);
	       if (verbose)
		    printf("data ranges from %g to %g.\n", a_min, a_max);
	  }
	  
	  if (verbose)
	       printf("writing to %s.\n", v5d_fname);

	  output_v5d(v5d_fname, data_name, a, store_bytes);

	  arrayh5_destroy(a);
	  if (v5d_fname)
	       free(v5d_fname);
	  v5d_fname = NULL;
     }

     if (data_name)
	  free(data_name);

     return EXIT_SUCCESS;
}
