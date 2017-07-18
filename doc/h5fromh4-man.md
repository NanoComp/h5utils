# h5fromh4: convert HDF4 scientific datasets to an HDF5 file

## Synopsis

    h5fromh4 [OPTION]... [HDF4FILE]...

## Description

`h5fromh4` takes one or more files in HDF4 format and outputs files in HDF5 format containing the datasets from the HDF4 files. (Currently, only a single dataset per HDF4 file is converted.)

HDF4 and HDF5 are free, portable binary formats and supporting libraries developed by the National Center for Supercomputing Applications at the University of Illinois in Urbana-Champaign.

A single `.h5` file can contain multiple data sets; by default, `h5fromh4` creates a dataset called `data`, but this can be changed via the `-d` option, or by using the syntax `HDF5FILE:DATASET` with the `-o` option. The `-a` option can be used to append new datasets to an existing HDF5 file. If the `-o` option is used and multiple HDF4 files are specified, all the HDF4 datasets are output into that HDF5 file with the input filenames (minus the `.hdf` suffix) used as the dataset names.

The most basic usage is something like `h5fromh4 foo.hdf`, which will output a file `foo.h5` containing the scientific dataset from `foo.hdf`.`

## Options

* `-h` — Display help on the command-line options and usage.

* `-V` — Print the version number and copyright info for `h5fromh4`

* `-v` — Verbose output.

* `-a` — If the HDF5 output file already exists, append the data as a new dataset rather than overwriting the file (the default behavior). An existing dataset of the same name within the file is overwritten, however.

* `-o` `file` — Send HDF5 output to `file` rather than to the input filename with .hdf replaced with .h5 (the default). If multiple input files were specified, this causes all input datasets to be stored in `file` (rather than in separate files), with the input filenames (minus the .hdf suffix) as the dataset names.

* `-d` `name` — Write to dataset `name` in the output; otherwise, the output dataset is called "data" by default. Alternatively, use the syntax `HDF5FILE:DATASET` with the `-o` option.

## Bugs

Report bugs by filing an issue at https://github.com/stevengj/h5utils

## Authors

Written by [Steven G. Johnson](http://math.mit.edu/~stevenj/). Copyright © 2017 by the Massachusetts Institute of Technology.
