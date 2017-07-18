/* Copyright (c) 1999-2008 Massachusetts Institute of Technology
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

/* don't use new HDF5 1.8 API (which isn't even fully documented yet, grrr) */
#define H5_USE_16_API 1

#include <hdf5.h>

#include "arrayh5.h"

#define CHECK(cond, msg) { if (!(cond)) { fprintf(stderr, "arrayh5 error: %s\n", msg); exit(EXIT_FAILURE); } }

#define CHK_MALLOC(p, t, n) CHECK(p = (t *) malloc(sizeof(t) * (n)), "out of memory")

/* Normally, HDF5 prints out all sorts of error messages, e.g. if a dataset
   can't be found, in addition to returning an error code.  The following
   macro can be wrapped around code to temporarily suppress error messages. */

#define SUPPRESS_HDF5_ERRORS(statements) { \
     H5E_auto_t xxxxx_err_func; \
     void *xxxxx_err_func_data; \
     H5Eget_auto(&xxxxx_err_func, &xxxxx_err_func_data); \
     H5Eset_auto(NULL, NULL); \
     { statements; } \
     H5Eset_auto(xxxxx_err_func, xxxxx_err_func_data); \
}

arrayh5 arrayh5_create_withdata(int rank, const int *dims, double *data)
{
     arrayh5 a;
     int i;

     CHECK(rank >= 0, "non-positive rank");
     a.rank = rank;

     CHK_MALLOC(a.dims, int, rank);

     a.N = 1;
     for (i = 0; i < rank; ++i) {
	  a.dims[i] = dims[i];
	  a.N *= dims[i];
     }

     if (data)
	  a.data = data;
     else {
	  CHK_MALLOC(a.data, double, a.N);
     }
     return a;
}

arrayh5 arrayh5_create(int rank, const int *dims)
{
     return arrayh5_create_withdata(rank, dims, NULL);
}

arrayh5 arrayh5_clone(arrayh5 a)
{
     arrayh5 b = arrayh5_create(a.rank, a.dims);
     if (a.data) memcpy(b.data, a.data, sizeof(double) * a.N);
     return b;
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

static void rtranspose(int curdim, int rank, const int *dims,
		       int curindex, int curindex_t,
		       const double *data, double *data_t)
{
     int prod_before = 1, prod_after = 1;
     int i;

     if (rank == 0) {
	  *data_t = *data;
	  return;
     }

     for (i = 0; i < curdim; ++i)
	  prod_before *= dims[i];
     for (i = curdim + 1; i < rank; ++i)
	  prod_after *= dims[i];

     if (curdim == rank - 1) {
	  for (i = 0; i < dims[curdim]; ++i)
	       data_t[curindex_t + i * prod_before] = data[curindex + i];
     }
     else {
	  for (i = 0; i < dims[curdim]; ++i)
	       rtranspose(curdim + 1, rank, dims,
			  curindex + i * prod_after,
			  curindex_t + i * prod_before,
			  data, data_t);
     }
}

void arrayh5_transpose(arrayh5 *a)
{
     double *data_t;
     int i;

     CHK_MALLOC(data_t, double, a->N);
     rtranspose(0, a->rank, a->dims, 0, 0, a->data, data_t);
     free(a->data);
     a->data = data_t;

     for (i = 0; i < a->rank - 1 - i; ++i) {
	  int dummy = a->dims[i];
	  a->dims[i] = a->dims[a->rank - 1 - i];
	  a->dims[a->rank - 1 - i] = dummy;
     }
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
	  CHK_MALLOC(*dname, char, strlen(name) + 1);
	  strcpy(*dname, name);
	  return 1;
     }
     return 0;
}

typedef enum { NO_ERROR = 0, OPEN_FAILED, NO_DATA, READ_FAILED, SLICE_FAILED,
	     INVALID_SLICE, INVALID_RANK, OPEN_DATA_FAILED } arrayh5_err;

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
		 char **dataname,
		 int nslicedims_, const int *slicedim_, const int *islice_,
		 const int *center_slice)
{
     hid_t file_id = -1, data_id = -1, space_id = -1;
     char *dname = NULL;
     int err = NO_ERROR;
     hsize_t i, rank, *dims_copy, *maxdims, *slicedim = 0;
     int *islice = 0;
     int *dims = 0;
     hsize_t nslicedims = (hsize_t) nslicedims_;

     CHECK(a, "NULL array passed to arrayh5_read");
     a->dims = NULL;
     a->data = NULL;

     file_id = H5Fopen(fname, H5F_ACC_RDONLY, H5P_DEFAULT);
     if (file_id < 0) {
	  err = OPEN_FAILED;
	  goto done;
     }

     if (datapath && datapath[0]) {
	  CHK_MALLOC(dname, char, strlen(datapath) + 1);
	  strcpy(dname, datapath);
     }
     else {
	  if (H5Giterate(file_id, "/", NULL, find_dataset, &dname) <= 0) {
	       err = NO_DATA;
	       goto done;
	  }
     }

     data_id = H5Dopen(file_id, dname);
     if (data_id < 0) {
	  err = OPEN_DATA_FAILED;
	  goto done;
     }

     space_id = H5Dget_space(data_id);
     rank = H5Sget_simple_extent_ndims(space_id);
     if (rank <= 0) {
	  err = INVALID_RANK;
	  goto done;
     }

     CHK_MALLOC(dims, int, rank);
     CHK_MALLOC(dims_copy, hsize_t, rank);
     CHK_MALLOC(maxdims, hsize_t, rank);

     H5Sget_simple_extent_dims(space_id, dims_copy, maxdims);
     for (i = 0; i < rank; ++i)
	  dims[i] = dims_copy[i];

     free(maxdims);
     free(dims_copy);

     for (i = 0; i < nslicedims && slicedim_[i] == NO_SLICE_DIM; ++i)
	  ;

     if (i == nslicedims) { /* no slices */
	  *a = arrayh5_create(rank, dims);

	  if (H5Dread(data_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL,
		      H5P_DEFAULT, (void *) a->data) < 0) {
	       err = READ_FAILED;
	       goto done;
	  }
     }
     else if (nslicedims > 0) {
	  int j, rank2 = rank;
	  hsize_t *start;
	  hsize_t *count;
	  hid_t mem_space_id;
	  herr_t readerr;

	  CHK_MALLOC(slicedim, hsize_t, nslicedims);
	  CHK_MALLOC(islice, int, nslicedims);

	  for (i = j = 0; i < nslicedims; ++i)
	       if (slicedim_[i] != NO_SLICE_DIM) {
		    if (slicedim_[i] == LAST_SLICE_DIM)
			 slicedim[j] = rank - 1;
		    else
			 slicedim[j] = (hsize_t) slicedim_[i];
		    if (slicedim[j] >= rank) {
			 err = INVALID_SLICE;
			 goto done;
		    }
		    islice[j] = islice_[i];
		    if (center_slice[i])
			 islice[j] += dims[slicedim[j]] / 2;
		    if (islice[j] < 0 || islice[j] >= dims[slicedim[j]]) {
			 err = INVALID_SLICE;
			 goto done;
		    }
		    j++;
	       }
	  nslicedims = j;

	  CHK_MALLOC(start, hsize_t, rank);
	  CHK_MALLOC(count, hsize_t, rank);

	  for (i = 0; i < rank; ++i) {
	       count[i] = dims[i];
	       start[i] = 0;
	  }
	  for (i = 0; i < nslicedims; ++i) {
	       start[slicedim[i]] = islice[i];
	       count[slicedim[i]] = 1;
	  }

	  H5Sselect_hyperslab(space_id, H5S_SELECT_SET,
			      start, NULL, count, NULL);

	  for (i = j = 0; i < rank; ++i)
	       if (count[i] > 1)
		    dims[j++] = count[i];
	  rank2 = j;

	  *a = arrayh5_create(rank2, dims);

	  mem_space_id = H5Screate_simple(rank, count, NULL);
	  H5Sselect_all(mem_space_id);

	  readerr = H5Dread(data_id, H5T_NATIVE_DOUBLE,
			    mem_space_id, space_id,
			    H5P_DEFAULT, (void *) a->data);

	  H5Sclose(mem_space_id);
	  free(count);
	  free(start);

	  if (readerr < 0) {
	       err = SLICE_FAILED;
	       goto done;
	  }
     }
     else {
	  err = INVALID_SLICE;
	  goto done;
     }

 done:
     if (err != NO_ERROR)
	  arrayh5_destroy(*a);
     free(islice);
     free(slicedim);
     free(dims);
     if (space_id >= 0)
	  H5Sclose(space_id);
     if (data_id >= 0)
	  H5Dclose(data_id);
     if (dataname)
	  *dataname = dname;
     else
	  free(dname);
     if (file_id >= 0)
	  H5Fclose(file_id);

     return err;
}

static int dataset_exists(hid_t id, const char *name)
{
     hid_t data_id;
     SUPPRESS_HDF5_ERRORS(data_id = H5Dopen(id, name));
     if (data_id >= 0)
          H5Dclose(data_id);
     return (data_id >= 0);
}

void arrayh5_write(arrayh5 a, char *filename, char *dataname,
		   short append_data)
{
     int i;
     hid_t file_id, space_id, type_id, data_id;
     hsize_t *dims_copy;

     if (append_data)
	  file_id = H5Fopen(filename, H5F_ACC_RDWR, H5P_DEFAULT);
     else
	  file_id = H5Fcreate(filename, H5F_ACC_TRUNC,
			      H5P_DEFAULT, H5P_DEFAULT);
     CHECK(file_id >= 0, "error opening HDF5 output file");

     if (dataset_exists(file_id, dataname))
	  H5Gunlink(file_id, dataname);  /* delete it */

     CHECK(a.rank > 0, "non-positive rank");
     CHK_MALLOC(dims_copy, hsize_t, a.rank);
     for (i = 0; i < a.rank; ++i)
	  dims_copy[i] = a.dims[i];
     space_id = H5Screate_simple(a.rank, dims_copy, NULL);
     free(dims_copy);

     type_id = H5T_NATIVE_DOUBLE;
     data_id = H5Dcreate(file_id, dataname, type_id, space_id, H5P_DEFAULT);
     H5Sclose(space_id);

     H5Dwrite(data_id, type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, a.data);

     H5Dclose(data_id);
     H5Fclose(file_id);
}

int arrayh5_read_rank(const char *fname, const char *datapath, int *rank)
{
     hid_t file_id = -1, data_id = -1, space_id = -1;
     char *dname = NULL;
     int err = NO_ERROR;

     file_id = H5Fopen(fname, H5F_ACC_RDONLY, H5P_DEFAULT);
     if (file_id < 0) {
	  err = OPEN_FAILED;
	  goto done;
     }

     if (datapath && datapath[0]) {
	  CHK_MALLOC(dname, char, strlen(datapath) + 1);
	  strcpy(dname, datapath);
     }
     else {
	  if (H5Giterate(file_id, "/", NULL, find_dataset, &dname) <= 0) {
	       err = NO_DATA;
	       goto done;
	  }
     }

     data_id = H5Dopen(file_id, dname);
     if (data_id < 0) {
	  err = OPEN_DATA_FAILED;
	  goto done;
     }

     space_id = H5Dget_space(data_id);
     *rank = H5Sget_simple_extent_ndims(space_id);

 done:
     if (space_id >= 0)
	  H5Sclose(space_id);
     if (data_id >= 0)
	  H5Dclose(data_id);
     free(dname);
     if (file_id >= 0)
	  H5Fclose(file_id);

     return err;
}
