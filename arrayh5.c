/* Copyright (c) 1999, 2000 Massachusetts Institute of Technology
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
#include <string.h>

#include <hdf5.h>

#include "arrayh5.h"

#define CHECK(cond, msg) { if (!(cond)) { fprintf(stderr, "arrayh5 error: %s\n", msg); exit(EXIT_FAILURE); } }

arrayh5 arrayh5_create(int rank, const int *dims)
{
     arrayh5 a;
     int i;

     CHECK(rank >= 0, "non-positive rank");
     a.rank = rank;
     
     a.dims = (int *) malloc(sizeof(int) * rank);
     CHECK(a.dims, "out of memory");

     a.N = 1;
     for (i = 0; i < rank; ++i) {
	  a.dims[i] = dims[i];
	  a.N *= dims[i];
     }

     a.data = (double *) malloc(sizeof(double) * a.N);
     CHECK(a.data, "out of memory");
     return a;
}

arrayh5 arrayh5_clone(arrayh5 a)
{
     return arrayh5_create(a.rank, a.dims);
}

void arrayh5_destroy(arrayh5 a)
{
     free(a.dims);
     free(a.data);
}

int arrayh5_conformant(arrayh5 a, arrayh5 b)
{
     int i;

     if (a.rank != b.rank)
	  return 0;
     for (i = 0; i < a.rank; ++i)
	  if (a.dims[i] != b.dims[i])
	       return 0;
     return 1;
}

void arrayh5_getrange(arrayh5 a, double *min, double *max)
{
     int i;

     CHECK(a.N > 0, "no elements in array");
     *min = *max = a.data[0];
     for (i = 1; i < a.N; ++i) {
	  if (a.data[i] < *min)
	       *min = a.data[i];
	  if (a.data[i] > *max)
	       *max = a.data[i];
     }
}

static herr_t find_dataset(hid_t group_id, const char *name, void *d)
{
     char **dname = (char **) d;
     H5G_stat_t info;

     H5Gget_objinfo(group_id, name, 1, &info);
     if (info.type == H5G_DATASET) {
	  *dname = malloc(sizeof(char) * (strlen(name) + 1));
	  strcpy(*dname, name);
	  return 1;
     }
     return 0;
}

const char arrayh5_read_strerror[][100] = {
     "no error",
     "error opening HD5 file",
     "couldn't find data set in HDF5 file",
     "error reading data from HDF5",
     "error reading data slice from HDF5",
     "invalid slice of HDF5 data",
     "non-positive rank in HDF file",
     "error opening data set in HDF file",
};

int arrayh5_read(arrayh5 *a, const char *fname, const char *datapath,
		 int slicedim, int islice)
{
     hid_t file_id = -1, data_id = -1, space_id = -1;
     char *dname = NULL;
     int err = 0, i;
     hsize_t rank, *dims_copy, *maxdims;
     int *dims;

     CHECK(a, "NULL array passed to arrayh5_read");
     a->dims = NULL;
     a->data = NULL;

     file_id = H5Fopen(fname, H5F_ACC_RDONLY, H5P_DEFAULT);
     if (file_id < 0) {
	  err = 1;
	  goto done;
     }
 
     if (datapath && datapath[0]) {
	  dname = (char*) malloc(sizeof(char) * (strlen(datapath) + 1));
	  CHECK(dname, "out of memory");
	  strcpy(dname, datapath);
     }
     else {
	  if (H5Giterate(file_id, "/", NULL, find_dataset, &dname) <= 0) {
	       err = 2;
	       goto done;
	  }
     }

     data_id = H5Dopen(file_id, dname);
     if (data_id < 0) {
	  err = 7;
	  goto done;
     }

     space_id = H5Dget_space(data_id);
     rank = H5Sget_simple_extent_ndims(space_id);
     if (rank <= 0) {
	  err = 6;
	  goto done;
     }
     
     dims = (int *) malloc(sizeof(int) * rank);
     dims_copy = (hsize_t *) malloc(sizeof(hsize_t) * rank);
     maxdims = (hsize_t *) malloc(sizeof(hsize_t) * rank);
     CHECK(dims_copy && maxdims, "out of memory!");

     H5Sget_simple_extent_dims(space_id, dims_copy, maxdims);
     for (i = 0; i < rank; ++i)
	  dims[i] = dims_copy[i];

     free(maxdims);
     free(dims_copy);

     if (slicedim < 0 || (slicedim >= rank && islice == 0)) {
	  *a = arrayh5_create(rank, dims);
	  free(dims);
	  
	  if (H5Dread(data_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL,
		      H5P_DEFAULT, (void *) a->data) < 0) {
	       err = 3;
	       goto done;
	  }
     }
     else if (slicedim < rank && islice >= 0 && islice < dims[slicedim]) {
	  hssize_t *start;
	  hsize_t *count, *count2;
	  hid_t mem_space_id;
	  herr_t readerr;

	  start = (hssize_t *) malloc(sizeof(hssize_t) * rank);
	  count = (hsize_t *) malloc(sizeof(hsize_t) * rank);
	  count2 = (hsize_t *) malloc(sizeof(hsize_t) * rank);
	  CHECK(start && count && count2, "out of memory");

	  for (i = 0; i < rank; ++i) {
	       count[i] = dims[i];
	       count2[i] = dims[i];
	       start[i] = 0;
	  }
	  start[slicedim] = islice;
	  count[slicedim] = 1;

	  H5Sselect_hyperslab(space_id, H5S_SELECT_SET,
			      start, NULL, count, NULL);

	  for (i = slicedim; i + 1 < rank; ++i)
	       count2[i] = dims[i] = dims[i + 1];
	  start[slicedim] = 0;
	  rank = rank - 1;
	  if (rank == 0) {
	       rank = 1;
	       count2[0] = dims[0] = 1;
	       *a = arrayh5_create(0, dims);
	  }
	  else
	       *a = arrayh5_create(rank, dims);
	  free(dims);

	  mem_space_id = H5Screate_simple(rank, count2, NULL);
	  H5Sselect_hyperslab(mem_space_id, H5S_SELECT_SET,
			      start, NULL, count2, NULL);

	  readerr = H5Dread(data_id, H5T_NATIVE_DOUBLE, 
			    mem_space_id, space_id, 
			    H5P_DEFAULT, (void *) a->data);

	  H5Sclose(mem_space_id);
	  free(count2);
	  free(count);
	  free(start);
	  
	  if (readerr < 0) {
	       err = 4;
	       goto done;
	  }
     }
     else {
	  free(dims);
	  err = 5;
	  goto done;
     }

 done:
     if (err >= 3 && err <= 4)
	  arrayh5_destroy(*a);
     if (space_id >= 0)
	  H5Sclose(space_id);
     if (data_id >= 0)
	  H5Dclose(data_id);
     free(dname);
     if (file_id >= 0)
	  H5Fclose(file_id);

     return err;
}
