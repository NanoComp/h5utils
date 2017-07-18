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

#ifndef ARRAYH4_H
#define ARRAYH4_H

#if defined(HAVE_HDF_H)
#  include <hdf.h>
#elif defined(HAVE_HDF_HDF_H)
#  include <hdf/hdf.h>
#endif

#define ARRAYH4_MAX_RANK 10

typedef struct {
  int32 numtype;
  intn rank;
  int32 dims[ARRAYH4_MAX_RANK];
  int N;
  union {
    float32 *f;
    float64 *d;
  } p;
  union {
    float32 *f[ARRAYH4_MAX_RANK];
    float64 *d[ARRAYH4_MAX_RANK];
  } scale;
} arrayh4;

extern int arrayh4_create(arrayh4 *b, int32 numtype,
			  intn rank, const int32 *dims);
extern int arrayh4_clone(arrayh4 *b, arrayh4 a);
extern void arrayh4_destroy(arrayh4 a);
extern int arrayh4_read(char *fname, arrayh4 *a, int require_rank);
extern int arrayh4_write(char *fname, arrayh4 a);
extern short arrayh4_conformant(arrayh4 a, arrayh4 b);

#endif
