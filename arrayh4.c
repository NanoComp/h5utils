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

#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "arrayh4.h"

#ifdef VERBOSE
#  define WHEN_VERBOSE(x) x
#else
#  define WHEN_VERBOSE(x) 
#endif

int arrayh4_create(arrayh4 *b, int32 numtype, intn rank, const int32 *dims)
{
     int i;
     
     b->rank = rank;
     b->N = 1;
     for (i = 0; i < rank; ++i) {
	  b->dims[i] = dims[i];
	  b->N *= dims[i];
	  if (numtype == DFNT_FLOAT64)
	       b->scale.d[i] = NULL;
	  else
	       b->scale.f[i] = NULL;
     }
     b->numtype = numtype;
     if (numtype == DFNT_FLOAT64) {
	  b->p.d = (float64 *) malloc(b->N * sizeof(float64));
	  if (!b->p.d)
	       return 0;
     } else if (numtype == DFNT_FLOAT32) {
	  b->p.f = (float32 *) malloc(b->N * sizeof(float32));
	  if (!b->p.f)
	       return 0;
     }
     return 1;
}

int arrayh4_clone(arrayh4 *b, arrayh4 a)
{
     int dim, i;

     i = arrayh4_create(b, a.numtype, a.rank, a.dims);

     if (!i)
	  return 0;

     /* copy the axes scales: */

     if (a.numtype == DFNT_FLOAT64) {
	  for (dim = 0; dim < a.rank; ++dim)
	       if (a.scale.d[dim]) {
		    b->scale.d[dim] = (float64 *) malloc(a.dims[dim] * 
							 sizeof(float64));
		    if (!b->scale.d[dim])
			 return 0;
		    for (i = 0; i < a.dims[dim]; ++i)
			 b->scale.d[dim][i] = a.scale.d[dim][i];
	       }
     }
     else if (a.numtype == DFNT_FLOAT32) {
	  for (dim = 0; dim < a.rank; ++dim)
	       if (a.scale.f[dim]) {
		    b->scale.f[dim] = (float32 *) malloc(a.dims[dim] * 
							 sizeof(float32));
		    if (!b->scale.f[dim])
			 return 0;
		    for (i = 0; i < a.dims[dim]; ++i)
			 b->scale.f[dim][i] = a.scale.f[dim][i];
	       }
     }

     return 1;
}

void arrayh4_destroy(arrayh4 a)
{
     int i;

     if (a.numtype == DFNT_FLOAT64) {
	  free(a.p.d);
	  for (i = 0; i < a.rank; ++i)
	       free(a.scale.d[i]);
     }
     else if (a.numtype == DFNT_FLOAT32) {
	  free(a.p.f);
	  for (i = 0; i < a.rank; ++i)
	       free(a.scale.f[i]);
     }
}

int arrayh4_read(char *fname, arrayh4 *a, int require_rank)
{
     int32 numtype;
     intn rank;
     int32 dims[ARRAYH4_MAX_RANK];
     int dim;
     
     WHEN_VERBOSE(printf("Reading arrayh4 from \"%s\"...\n", fname));
     
     if (require_rank < 0 || require_rank > ARRAYH4_MAX_RANK)
	  return 0;
     
     DFSDclear();
     DFSDgetdims(fname, &rank, dims, ARRAYH4_MAX_RANK);
     
     WHEN_VERBOSE(printf("   rank = %d", rank));
     
     if (require_rank && require_rank != rank)
	  return 0;
     
     WHEN_VERBOSE(printf(", dimensions are "));
     
     for (dim = 0; dim < rank; ++dim) {
	  WHEN_VERBOSE(printf("%s%d", dim ? "x" : "", (int) dims[dim]));
     }
     
     WHEN_VERBOSE(printf("\n"));
     
     DFSDgetNT(&numtype);
     
     arrayh4_create(a, numtype, rank, dims);
     
     if (a->numtype == DFNT_FLOAT64) {
	  WHEN_VERBOSE(printf("   double precision\n"));
	  DFSDgetdata(fname, a->rank, a->dims, (VOIDP) a->p.d);
	  
	  for (dim = 0; dim < a->rank; ++dim) {
	       a->scale.d[dim] = (float64 *) malloc(a->dims[dim] * 
						    sizeof(float64));
	       if (FAIL == DFSDgetdimscale(dim+1, a->dims[dim], 
					   (VOIDP) a->scale.d[dim])) {
		    free(a->scale.d[dim]);
		    a->scale.d[dim] = NULL;
	       }
	  }
     } else if (a->numtype == DFNT_FLOAT32) {
	  WHEN_VERBOSE(printf("   single precision\n"));
	  DFSDgetdata(fname, a->rank, a->dims, (VOIDP) a->p.f);
	  
	for (dim = 0; dim < a->rank; ++dim) {
	     a->scale.f[dim] = (float32 *) malloc(a->dims[dim] * 
						  sizeof(float32));
	     if (FAIL == DFSDgetdimscale(dim+1, a->dims[dim], 
					 (VOIDP) a->scale.f[dim])) {
		  free(a->scale.f[dim]);
		  a->scale.f[dim] = NULL;
	     }
	}
     } else
	  return 0;
     
     return 1;
}

#define MIN2(a,b) ((a) < (b) ? (a) : (b))
#define MAX2(a,b) ((a) > (b) ? (a) : (b))

int arrayh4_write(char *fname, arrayh4 a)
{
     int i;
     WHEN_VERBOSE(printf("Writing arrayh4 to \"%s\"...\n", fname));
     
     remove(fname);
     
     DFSDclear();
     DFSDsetdims(a.rank, a.dims);
     DFSDsetNT(a.numtype);
     
     if (a.numtype == DFNT_FLOAT64) {
	  float64 minval = 1e40, maxval = -1e40;
	  
	  for (i = 0; i < a.N; ++i) {
	       minval = MIN2(minval, a.p.d[i]);
	       maxval = MAX2(maxval, a.p.d[i]);
	  }
	  DFSDsetrange(&maxval, &minval);
	  DFSDadddata(fname, a.rank, a.dims, (VOIDP) a.p.d);
	  
	  for (i = 0; i < a.rank; ++i)
	       if (a.scale.d[i])
		    DFSDgetdimscale(i+1, a.dims[i], (VOIDP) a.scale.d[i]);
     } else if (a.numtype == DFNT_FLOAT32) {
	  float32 minval = 1e20, maxval = -1e20;
	  
	  for (i = 0; i < a.N; ++i) {
	       minval = MIN2(minval, a.p.f[i]);
	       maxval = MAX2(maxval, a.p.f[i]);
	  }
	  DFSDsetrange(&maxval, &minval);
	  DFSDadddata(fname, a.rank, a.dims, (VOIDP) a.p.f);
	  
	  for (i = 0; i < a.rank; ++i)
	       if (a.scale.f[i])
		    DFSDgetdimscale(i+1, a.dims[i], (VOIDP) a.scale.f[i]);
     } else
	  return 0;
     
     return 1;
}

short arrayh4_conformant(arrayh4 a, arrayh4 b)
{
     int dim;
     
     if (a.numtype != b.numtype || a.N != b.N || a.rank != b.rank)
	  return 0;
     for (dim = 0; dim < a.rank; ++dim)
	  if (a.dims[dim] != b.dims[dim])
	       return 0;
     return 1;
}
