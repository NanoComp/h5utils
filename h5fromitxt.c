/* Copyright (c) 2004 Massachusetts Institute of Technology
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
#include "h5utils.h"

#define CHECK(cond, msg) { if (!(cond)) { fprintf(stderr, "h5fromtxt error: %s\n", msg); exit(EXIT_FAILURE); } }

void usage(FILE *f)
{
     fprintf(f, "Usage: h5fromitxt [options] <hdf5-file>\n"
	     "Options:\n"
	     "         -h : this help message\n"
             "         -V : print version number and copyright\n"
	     "         -v : verbose output\n"
             "         -a : append to existing hdf5 file\n"
	     "   -f <val> : use <val> as missing filler [ default: 0 ]\n"
	     "  -n <size> : input array dimensions [ default: guessed ]\n"
	     "  -m <size> : input coordinate minimum [ default: guessed ]\n"
	     "  -M <size> : input coordinate maximum [ default: guessed ]\n"
	     "         -T : transpose the data [default: no]\n"
	     "  -d <name> : use dataset <name> in the output file (default: \"data\")\n"
	     "              -- you can also specify a dataset via <filename>:<name>\n"
	  );
}

#define MAX_RANK 10

int get_size_arg(int size[MAX_RANK], const char *arg)
{
     int pos = 0;
     int rank = 0;
     while (isdigit(arg[pos])) {
	  CHECK(rank < MAX_RANK,
		"Rank too big in -n argument!\n");
	  size[rank] = 0;
	  while (isdigit(arg[pos])) {
	       size[rank] = size[rank]*10 + arg[pos]-'0';
	       ++pos;
	  }
	  ++rank;
	  if (arg[pos] == 'x' || arg[pos] == 'X' || arg[pos] == '*')
	       ++pos;
     }
     CHECK(rank > 0 && !arg[pos], "Invalid <size> argument; should be e.g. 23x34 or 10x10x10\n");
     return rank;
}

int main(int argc, char **argv)
{
     arrayh5 a;
     char *dname, *h5_fname;
     char *data_name = NULL;
     extern char *optarg;
     extern int optind;
     int c;
     double *data;
     int idata = 0;
     int rank = -1, dims[MAX_RANK], N = 1, nrows = 0;
     int cmin_rank = -1, cmax_rank = -1, coord_min[MAX_RANK], coord_max[MAX_RANK];
     double fill_val = 0;
     int ncols = -1, cur_ncols = 0;
     int read_newline = 0;
     int verbose = 0;
     int transpose = 0;
     int append = 0;
     int i, j;

     while ((c = getopt(argc, argv, "hn:d:vTaVf:m:M:")) != -1)
	  switch (c) {
	      case 'h':
		   usage(stdout);
		   return EXIT_SUCCESS;
	      case 'V':
		   printf("h5fromtxt " PACKAGE_VERSION " by Steven G. Johnson\n" 
			  COPYRIGHT);
		   return EXIT_SUCCESS;
	      case 'v':
		   verbose = 1;
		   break;
	      case 'a':
		   append = 1;
		   break;
	      case 'T':
		   transpose = 1;
		   break;
	      case 'f':
		   fill_val = atof(optarg);
		   break;
	      case 'd':
		   free(data_name);
		   data_name = my_strdup(optarg);
		   break;		   
	      case 'n':
		   rank = get_size_arg(dims, optarg);
		   for (i = 0, N = 1; i < rank; ++i)
			N *= dims[i];
		   break;
	      case 'm':
		   cmin_rank = get_size_arg(coord_min, optarg);
		   break;
	      case 'M':
		   cmax_rank = get_size_arg(coord_max, optarg);
		   break;
	      default:
		   fprintf(stderr, "Invalid argument -%c\n", c);
		   usage(stderr);
		   return EXIT_FAILURE;
	  }
     if (optind + 1 != argc) {  /* should be exactly 1 parameter left */
	  usage(stderr);
	  return EXIT_FAILURE;
     }

     h5_fname = split_fname(argv[optind], &dname);
     if (!dname[0])
	  dname = data_name;
     if (!dname)
	  dname = my_strdup("data");

     data = (double *) malloc(sizeof(double) * N);
     CHECK(data, "out of memory");

     while (!feof(stdin)) {
	  read_newline = 0;

	  /* eat leading spaces */
	  while (isspace(c = getc(stdin)));
	  ungetc(c, stdin);
	  if (c == EOF)
	       break;

	  /* increase the size of the data array, if necessary */
	  if (idata >= N) {
	       CHECK(rank < 0, "more inputs in file than specified by -n");
	       N *= 2;
	       data = (double *) realloc(data, sizeof(double) * N);
	       CHECK(data, "out of memory");
	  }

	  CHECK(scanf("%lg", &data[idata++]) == 1,
		"error reading numeric input");

	  ++cur_ncols;
	  
	  /* eat characters until the next number: */
	  do { 
	       c = getc(stdin);
	       if (c == '\n')
		    read_newline = 1;
	  } while (!(isdigit(c) || c == '.' || c == '-' || c == '+'
		     || c == EOF));
	  ungetc(c, stdin);

	  if (read_newline) {
	       ++nrows;
	       CHECK(ncols < 0 || cur_ncols == ncols,
		     "the number of input columns is not constant.");
	       ncols = cur_ncols;
	       cur_ncols = 0;
	  }
     }

     if (!read_newline) { /* don't require a newline on the last line */
	  ++nrows;
	  CHECK(ncols < 0 || cur_ncols == ncols,
		"the number of input columns is not constant.");
     }

     CHECK(idata > 0, "no inputs read");
     CHECK(ncols > 1, "need at least one coordinate column");

     if (verbose)
	  printf("Read %d numbers in %d rows with rank-%d coordinates.\n", 
		 idata, nrows, ncols - 1);

     CHECK(ncols - 1 <= MAX_RANK, "rank is too large");

     if (cmin_rank < 0) {
	  cmin_rank = ncols - 1;
	  CHECK(cmin_rank == ncols - 1, "coordinate minima have wrong rank");
	  for (j = 0; j < ncols - 1; ++j) {
	       coord_min[j] = floor(data[j]);
	  }
	  for (i = 1; i < nrows; ++i)
	       for (j = 0; j < ncols - 1; ++j) {
		    int ij = i * ncols + j;
		    if (data[ij] < coord_min[j])
			 coord_min[j] = floor(data[ij]);
	       }
     }
     if (cmax_rank < 0) {
	  cmax_rank = ncols - 1;
	  CHECK(cmax_rank == ncols - 1, "coordinate maxima have wrong rank");
	  for (j = 0; j < ncols - 1; ++j) {
	       coord_max[j] = ceil(data[j]);
	  }
	  for (i = 1; i < nrows; ++i)
	       for (j = 0; j < ncols - 1; ++j) {
		    int ij = i * ncols + j;
		    if (data[ij] > coord_max[j])
			 coord_max[j] = ceil(data[ij]);
	       }
     }

     if (verbose) {
	  printf("Coordinates range from (%d", coord_min[0]);
	  for (j = 1; j < ncols - 1; ++j)
	       printf(",%d", coord_min[j]);
	  printf(") to (%d", coord_max[0]);
	  for (j = 1; j < ncols - 1; ++j)
	       printf(",%d", coord_max[j]);
	  printf(")\n");
     }

     if (rank < 0) {
	  rank = ncols - 1;
	  for (i = 0; i < rank; ++i)
	       dims[i] = coord_max[i] - coord_min[i] + 1;
     }
     CHECK(rank == ncols - 1, "number of coordinates does not match rank");

     a = arrayh5_create(rank, dims);
     for (i = 0; i < a.N; ++i)
	  a.data[i] = fill_val;

     for (i = 0; i < nrows; ++i) {
	  int idx = 0;
	  for (j = 0; j < rank; ++j) {
	       int id = data[i * ncols + j] + 0.5;
	       if (id < coord_min[j] || id > coord_max[j] ||
		   id - coord_min[j] >= dims[j]) {
		    idx = -1;
		    break;
	       }
	       idx = idx * dims[j] + id - coord_min[j];
	  }
	  if (idx >= 0 && idx < a.N)
	       a.data[idx] = data[i * ncols + ncols - 1];
     }

     if (transpose)
	  arrayh5_transpose(&a);

     if (verbose) {
	  double a_min, a_max;
	  arrayh5_getrange(a, &a_min, &a_max);
	  printf("data ranges from %g to %g.\n", a_min, a_max);
     }

     if (verbose) {
	  printf("Writing size %d", a.dims[0]);
	  for (i = 1; i < a.rank; ++i)
	       printf("x%d", a.dims[i]);
	  printf(" data to %s:%s\n", h5_fname, dname);
     }

     arrayh5_write(a, h5_fname, dname, append);
     arrayh5_destroy(a);

     return EXIT_SUCCESS;
}
