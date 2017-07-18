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
#include <ctype.h>

#include <unistd.h>

#include "config.h"
#include "arrayh5.h"
#include "copyright.h"
#include "h5utils.h"

#include <matheval.h>

#define CHECK(cond, msg) { if (!(cond)) { fprintf(stderr, "h5math error: %s\n", msg); exit(EXIT_FAILURE); } }

const char default_data_name[] = "h5math";

void usage(FILE *f)
{
     fprintf(f, "Usage: h5math [options] <output-filename> [<filenames>]\n"
	     "Options:\n"
	     "         -h : this help message\n"
             "         -V : print version number and copyright\n"
	     "         -v : verbose output\n"
	     "         -a : append to existing hdf5 file\n"
	     "  -n <size> : output array dimensions [ default: from input ]\n"
	     "  -f <file> : read expression to evaluate from file [ default: stdin ]\n"
	     "  -e <expr> : evaluate <expr> to output\n"
	     "    -x <ix> : take x=<ix> slice of data\n"
	     "    -y <iy> : take y=<iy> slice of data\n"
	     "    -z <iz> : take z=<iz> slice of data\n"
	     "    -t <it> : take t=<it> slice of data's last dimension\n"
	     "         -0 : use dataset center as origin for -x/-y/-z\n"
	     "     -r <r> : use resolution <r> for xyz coordinate units in expression\n"
	     "  -d <name> : use dataset <name> in the input/output files\n"
	     "              [ default: first dataset/%s ]\n"
	     "              -- you can also specify a dataset via <filename>:<name>\n",
	     default_data_name
	  );
}

#define MAX_RANK 10

int main(int argc, char **argv)
{
     arrayh5 *a, ao;
     int i, n;
     int rank = -1, dims[MAX_RANK];
     extern char *optarg;
     extern int optind;
     int c;
     int slicedim[4] = {NO_SLICE_DIM,NO_SLICE_DIM,NO_SLICE_DIM,NO_SLICE_DIM};
     int islice[4], center_slice[4] = {0,0,0,0};
     int verbose = 0;
     int append = 0;
     char *expr_string = 0, *expr_filename = 0;
     char *data_name = 0;
     char *out_fname, *out_dname;
     char **eval_vars;
     int eval_nvars;
     char **vars;
     double *vals;
     void *evaluator;
     double res = 1.0;
     int nx, ny, nz, nt, nr, ix, iy, iz, it, ir;
     double cx, cy, cz;

     while ((c = getopt(argc, argv, "hVvan:f:e:x:y:z:t:0d:r:")) != -1)
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
	      case 'a':
		   append = 1;
		   break;
	      case 'n':
	      {
		   int pos = 0;
		   rank = 0;
		   while (isdigit(optarg[pos])) {
			CHECK(rank < MAX_RANK,
			      "Rank too big in -n argument!\n");
			dims[rank] = 0;
			while (isdigit(optarg[pos])) {
			     dims[rank] = dims[rank]*10 + optarg[pos]-'0';
			     ++pos;
			}
			++rank;
			if (optarg[pos] == 'x' || optarg[pos] == 'X'
			    || optarg[pos] == '*')
			     ++pos;
		   }
		   CHECK(rank > 0 && !optarg[pos], "Invalid -n argument; should be e.g. 23x34 or 10x10x10\n");
		   break;
	      }
	      case 'f':
		   free(expr_filename);
		   expr_filename = my_strdup(optarg);
		   break;
	      case 'e':
		   free(expr_string);
		   expr_string = my_strdup(optarg);
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
	      case 'r':
		   res = atof(optarg);
		   break;
	      case 'd':
		   free(data_name);
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

     out_fname = split_fname(argv[optind], &out_dname);
     if (!out_dname[0]) {
	  if (data_name)
	       out_dname = data_name;
	  else
	       out_dname = (char *) default_data_name;
     }
     optind++;

     n = argc - optind;
     a = (arrayh5 *) malloc(sizeof(arrayh5) * n);
     CHECK(a, "out of memory");

     for (i = 0; i < n; ++i) {
	  int err;
	  char *fname, *dname;

          fname = split_fname(argv[i + optind], &dname);
          if (!dname[0])
               dname = data_name;

	  err = arrayh5_read(&a[i], fname, dname, NULL,
                             4, slicedim, islice, center_slice);
          CHECK(!err, arrayh5_read_strerror[err]);

	  CHECK(!i || arrayh5_conformant(a[i], a[i-1]),
		"all input arrays must have the same dimensions");

	  if (verbose)
	       printf("read variable d%d: dataset \"%s\" in file \"%s\"\n",
		      i + 1, dname ? dname : "<first>", fname);
	  
	  free(fname);
     }

     if (rank >= 0) {
	  ao = arrayh5_create(rank, dims);
	  CHECK(!n || arrayh5_conformant(ao, a[0]),
		"-n dimensions must be same as those of input arrays");
     }
     else if (n)
	  ao = arrayh5_clone(a[0]);
     else
	  CHECK(0, "output size must be specified with -n if no input arrays");

     if (verbose) {
	  printf("rank-%d array dimensions: ", ao.rank);
	  if (!ao.rank) printf("1\n");
	  for (i = 0; i < ao.rank; ++i)
	       printf("%s%d", i ? "x" : "", ao.dims[i]);
	  printf("\n");
     }

     nx = ao.rank >= 1 ? ao.dims[0] : 1;
     ny = ao.rank >= 2 ? ao.dims[1] : 1;
     nz = ao.rank >= 3 ? ao.dims[2] : 1;
     nt = ao.rank >= 4 ? ao.dims[3] : 1;
     for (nr = 1, i = 4; i < ao.rank; ++i)
	  nr *= ao.dims[i];
     cx = center_slice[0] ? (nx - 1) * 0.5 : 0.0;
     cy = center_slice[1] ? (ny - 1) * 0.5 : 0.0;
     cz = center_slice[2] ? (nz - 1) * 0.5 : 0.0;

     vars = (char **) malloc(sizeof(char *) * (n + 4));
     CHECK(vars, "out of memory");
     vals = (double *) malloc(sizeof(double) * (n + 4));
     CHECK(vals, "out of memory");
     for (i = 0; i < n; ++i) {
	  vars[i] = my_strdup("dxxxxxxxxxxxx");
#ifdef HAVE_SNPRINTF
	  snprintf(vars[i], 14, "d%d", i + 1);
#else
	  sprintf(vars[i], "d%d", i + 1);
#endif
	  vals[i] = 0.0;
     }
     vars[n+0] = strdup("x"); vals[n+0] = 0.0;
     vars[n+1] = strdup("y"); vals[n+1] = 0.0;
     vars[n+2] = strdup("z"); vals[n+2] = 0.0;
     vars[n+3] = strdup("t"); vals[n+3] = 0.0;
     
     if (!expr_string) {
	  char buf[1024] = "";
	  int len;
	  FILE *f = expr_filename ? fopen(expr_filename, "r") : stdin;
	  CHECK(f, "unable to open expression file");

	  if (verbose && f == stdin)
	       printf("Enter expression to write to %s:\n", out_fname);
	  
	  fgets(buf, 1024, f);
	  expr_string = my_strdup(buf);
	  len = strlen(buf) + 1;
	  while (fgets(buf, 1024, f)) {
	       len += strlen(buf);
	       expr_string = (char *) realloc(expr_string, len);
	       strcat(expr_string, buf);
	  }
	  for (ix = 0; ix < len; ++ix)
	       if (expr_string[ix] == '\n')
		    expr_string[ix] = ' '; /* matheval chokes on newlines */

	  if (expr_filename) fclose(f);
     }

     CHECK(evaluator = evaluator_create(expr_string),
	   "error parsing symbolic expression");

     evaluator_get_variables(evaluator, &eval_vars, &eval_nvars);
     for (ix = 0; ix < eval_nvars; ++ix) {
	  for (iy = 0; iy < n + 4 && strcmp(eval_vars[ix], vars[iy]); ++iy)
	       ;
	  if (iy == n + 4) {
	       fprintf(stderr, "h5math error: unrecognized variable \"%s\"\n",
		       eval_vars[ix]);
	       exit(EXIT_FAILURE);
	  }
     }
     
     if (verbose) {
	  char *buf = evaluator_get_string(evaluator);
	  printf("Evaluating expression: %s\n", buf);
     }

     for (ix = 0; ix < nx; ++ix)
     for (iy = 0; iy < ny; ++iy)
     for (iz = 0; iz < nz; ++iz)
     for (it = 0; it < nt; ++it)
     for (ir = 0; ir < nr; ++ir) {
	  int idx = ir + nr * (it + nt * (iz + nz * (iy + ny * ix)));
	  for (i = 0; i < n; ++i)
	       vals[i] = a[i].data[idx];
	  vals[n+0] = (ix - cx) / res;
	  vals[n+1] = (iy - cy) / res;
	  vals[n+2] = (iz - cz) / res;
	  vals[n+3] = ao.rank >= 4 ? it : 
	       (ao.rank >= 3 ? iz : (ao.rank >= 2 ? iy : ix));
	  ao.data[idx] = evaluator_evaluate(evaluator, n+4, vars, vals);
     }

     if (verbose)
	  printf("Writing data to \"%s\" in \"%s\"...\n", 
		 out_dname ? out_dname : "<first>", out_fname);
     arrayh5_write(ao, out_fname, out_dname, append);

     free(vals);
     for (i = 0; i < n+4; ++i) free(vars[i]);
     free(vars);
     arrayh5_destroy(ao);
     for (i = 0; i < n; ++i) arrayh5_destroy(a[i]);
     free(a);
     free(out_fname);
     free(expr_filename);
     free(expr_string);
     free(data_name);

     return EXIT_SUCCESS;
}
