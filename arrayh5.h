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

extern arrayh5 arrayh5_create_withdata(int rank, const int *dims,double *data);
extern arrayh5 arrayh5_create(int rank, const int *dims);
extern arrayh5 arrayh5_clone(arrayh5 a);
extern void arrayh5_transpose(arrayh5 *a);
extern void arrayh5_destroy(arrayh5 a);
extern int arrayh5_conformant(arrayh5 a, arrayh5 b);
extern void arrayh5_getrange(arrayh5 a, double *min, double *max);

extern const char arrayh5_read_strerror[][100];
extern int arrayh5_read(arrayh5 *a, const char *fname, const char *datapath,
			char **dataname,
			int nslicedims,
			const int *slicedim, const int *islice,
			const int *center_slice);
extern void arrayh5_write(arrayh5 a, char *filename, char *dataname,
			  short append_data);

int arrayh5_read_rank(const char *fname, const char *datapath, int *rank);

#define NO_SLICE_DIM -1
#define LAST_SLICE_DIM -2

/***********************************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* ARRAYH5_H */
