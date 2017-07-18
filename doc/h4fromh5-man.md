# h4fromh5: convert HDF5 scientific dataset to an HDF4 file

## Synopsis

    h4fromh5[OPTION]... [HDF4FILE]...

## Description

`h4fromh5` takes one or more files in HDF5 format and outputs files in HDF4 format containing the datasets from the HDF5 files. (Currently, only a single dataset per HDF5 file is converted.)

HDF4 and HDF5 are free, portable binary formats and supporting libraries developed by the National Center for Supercomputing Applications at the University of Illinois in Urbana-Champaign.

A single
HDF5 (`.h5`) file can contain multiple data sets; by default, `h4fromh5` converts the first dataset, but this can be changed via the `-d` option, or by using the syntax `HDF5FILE:DATASET`.

The most basic usage is something like `h4fromh5 foo.h5`, which will output a file `foo.hdf` containing the scientific dataset from `foo.h5`.

## Options

* `-h` — Display help on the command-line options and usage.
* `-V` — Print the version number and copyright info for `h4fromh5`.
* `-v` — Verbose output.
* `-T` — Transpose the output dataset (e.g. LxMxN becomes NxMxL). This is often useful because HDF5 programs typically follow C (row-major) conventions while HDF4 programs often follow Fortran (column-major, transposed) conventions for array ordering.
* `-o file` — Send HDF output to `file` rather than to the input filename with `.h5` replaced with `.hdf` (the default).
* `-d name` — Read from dataset `name` in the input; otherwise, the first dataset in the input file is used. Alternatively, use the syntax `HDF5FILE:DATASET` when the input file names are specified.

## Bugs

Report bugs by filing an issue at https://github.com/stevengj/h5utils

## Authors

Written by [Steven G. Johnson](http://math.mit.edu/~stevenj/). Copyright © 2017 by the Massachusetts Institute of Technology.
