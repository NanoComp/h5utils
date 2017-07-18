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

#define CHECK(cond, msg) { if (!(cond)) { fprintf(stderr, "h5fromtxt error: %s\n", msg); exit(EXIT_FAILURE); } }

void usage(FILE *f)
{
     fprintf(f, "Usage: h5fromtxt [options] <hdf5-file>\n"
	     "Options:\n"
	     "         -h : this help message\n"
             "         -V : print version number and copyright\n"
	     "         -v : verbose output\n"
             "         -a : append to existing hdf5 file\n"
	     "  -n <size> : input row-major array dimensions [ default: guessed ]\n"
	     "         -T : transpose the data [default: no]\n"
	     "  -d <name> : use dataset <name> in the output file (default: \"data\")\n"
	     "              -- you can also specify a dataset via <filename>:<name>\n"
	  );
}

#define MAX_RANK 10

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
     int ncols = -1, cur_ncols = 0;
     int read_newline = 0;
     int verbose = 0;
     int transpose = 0;
     int append = 0;

     while ((c = getopt(argc, argv, "hn:d:vTaV")) != -1)
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
	      case 'd':
		   free(data_name);
		   data_name = my_strdup(optarg);
		   break;		   
	      case 'n':
	      {
		   int pos = 0;
		   rank = 0;
		   N = 1;
		   while (isdigit(optarg[pos])) {
			CHECK(rank < MAX_RANK,
			      "Rank too big in -n argument!\n");
			dims[rank] = 0;
			while (isdigit(optarg[pos])) {
			     dims[rank] = dims[rank]*10 + optarg[pos]-'0';
			     ++pos;
			}
			N *= dims[rank];
			++rank;
			if (optarg[pos] == 'x' || optarg[pos] == 'X'
			    || optarg[pos] == '*')
			     ++pos;
		   }
		   CHECK(rank > 0 && !optarg[pos], "Invalid -n argument; should be e.g. 23x34 or 10x10x10\n");
		   break;
	      }
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
	       if (rank < 0) {  /* we're trying to guess the input dims */
		    CHECK(ncols < 0 || cur_ncols == ncols,
			  "the number of input columns is not constant.");
	       }
	       ncols = cur_ncols;
	       cur_ncols = 0;
	  }
     }

     if (!read_newline) { /* don't require a newline on the last line */
	  ++nrows;
	  if (rank < 0) {  /* we're trying to guess the input dims */
	       CHECK(ncols < 0 || cur_ncols == ncols,
		     "the number of input columns is not constant.");
	  }
     }

     CHECK(idata > 0, "no inputs read");

     if (verbose)
	  printf("Read %d numbers in %d rows.\n", idata, nrows);

     if (rank < 0) {
	  N = idata;
	  CHECK(N % nrows == 0,
		"each row must have an equal number of columns");
	  if (nrows == 1 || nrows == N) {
	       rank = 1;
	       dims[0] = N;
	  }
	  else {
	       rank = 2;
	       dims[0] = nrows;
	       dims[1] = N / nrows;
	  }
     }
     else {
	  CHECK(idata == N, "number of inputs does not match -n");
     }

     a = arrayh5_create_withdata(rank, dims, data);

     if (transpose)
	  arrayh5_transpose(&a);

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
	  printf(" data to %s:%s\n", h5_fname, dname);
     }

     arrayh5_write(a, h5_fname, dname, append);
     arrayh5_destroy(a);

     return EXIT_SUCCESS;
}
