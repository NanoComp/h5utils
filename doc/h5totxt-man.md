# h5totxt: generate comma-delimited text from 2d slices of HDF5 files

## Synopsis

    h5totxt [OPTION]... [HDF5FILE]...

## Description

`h5totxt` is a utility to generate comma-delimited text (and similar formats) from one-, two-, or more-dimensional slices of numeric datasets in HDF5 files. This way, the data can easily be imported into spreadsheets and similar programs for analysis and visualization.

HDF5 is a free, portable binary format and supporting library developed by the National Center for Supercomputing Applications at the University of Illinois in Urbana-Champaign. A single `.h5` file can contain multiple data sets; by default, `h5totxt` takes the first dataset, but this can be changed via the `-d` option, or by using the syntax `HDF5FILE:DATASET`.

By default, the entire dataset is dumped to the output. in row-major order. For 3d datasets, this corresponds to a sequence of yz slices, in order of increasing x, separated by blank lines. If `-T` is specified, outputs in the transposed (column-major) order instead

Often, however, you want only a one- or two-dimensional slice of multi-dimensional data. To do this, you specify coordinates in one or more slice dimensions, via the `-xyzt` options.

The most basic usage is something like `h5totxt foo.h5`, which will output comma-delimited text to stdout from the data in `foo.h5`.

## Options

* `-h` — Display help on the command-line options and usage.

* `-V` — Print the version number and copyright info for `h5totxt`.

* `-v` — Verbose output.

* `-o file` — Send text output to `file` rather than to stdout (the default).

* `-s sep` — Use the string `sep` to separate columns of the output rather than a comma (the default).

* `-x ix`, `-y iy`, `-z iz`, `-t it` — This tells `h5totxt` to use a particular slice of a multi-dimensional dataset. e.g. `-x` causes a yz plane (of a 3d dataset) to be used, at an x index of `ix` (where the indices run from zero to one less than the maximum index in that direction). Here, x/y/z correspond to the first/second/third dimensions of the HDF5 dataset. The `-t` option specifies a slice in the last dimension, whichever that might be. See also the `-0` option to shift the origin of the x/y/z slice coordinates to the dataset center.

* `-0` — Shift the origin of the x/y/z slice coordinates to the dataset center, so that e.g. -0 -x 0 (or more compactly -0x0) returns the central x plane of the dataset instead of the edge x plane. (`-t` coordinates are not affected.)

* `-T` — Transpose the data (interchange the dimension ordering). By default, no transposition is done.

* `-. numdigits` — Output `numdigits` digits after the decimal point (defaults to 16).

* `-d name` — Use dataset `name` from the input files; otherwise, the first dataset from each file is used. Alternatively, use the syntax `HDF5FILE:DATASET`, which allows you to specify a different dataset for each file. You can use the `h5ls` command (included with hdf5) to find the names of datasets within a file.

## Bugs

Report bugs by filing an issue at https://github.com/stevengj/h5utils

## Authors

Written by [Steven G. Johnson](http://math.mit.edu/~stevenj/). Copyright © 2017 by the Massachusetts Institute of Technology.
