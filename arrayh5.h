/* Copyright (c) 1999 Massachusetts Institute of Technology
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

#ifndef ARRAYH5_H
#define ARRAYH5_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***********************************************************************/

typedef struct {
     int rank, *dims, N;
     double *data;
} arrayh5;

extern arrayh5 arrayh5_create(int rank, const int *dims);
extern arrayh5 arrayh5_clone(arrayh5 a);
extern void arrayh5_destroy(arrayh5 a);
extern int arrayh5_conformant(arrayh5 a, arrayh5 b);

extern const char arrayh5_read_strerror[][100];
extern int arrayh5_read(arrayh5 *a, const char *fname, const char *datapath,
			int slicedim, int islice);

/***********************************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* ARRAYH5_H */
