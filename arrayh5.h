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
