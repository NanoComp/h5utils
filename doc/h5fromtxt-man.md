# h5fromtxt: convert text input to an HDF5 file

## Synopsis

    h5fromtxt [OPTION]... [HDF5FILE]

## Description

`h5fromtxt` takes a series of numbers from standard input and outputs a multi-dimensional numeric dataset in an HDF5 file.

HDF5 is a free, portable binary format and supporting library developed by the National Center for Supercomputing Applications at the University of Illinois in Urbana-Champaign. A single `.h5` file can contain multiple data sets; by default, `h5fromtxt` creates a dataset called "data", but this can be changed via the `-d` option, or by using the syntax `HDF5FILE:DATASET`. The `-a` option can be used to append new datasets to an existing HDF5 file.

All characters besides the numbers (and associated decimal points, etcetera) in the input are ignored. By default, the data is assumed to be a two-dimensional MxN dataset where M is the number of rows (delimited by newlines) and N is the number of columns. In this case, it is an error for the number of columns to vary between rows. If M or N is 1 then the data is written as a one-dimensional dataset.

Alternatively, you can specify the dimensions of the data explicitly via the `-n` `size` option, where `size` is e.g. "2x2x2". In this case, newlines are ignored and the data is taken as an array of the given size stored in row-major ("C") order (where the last index varies most quickly as you step through the data). e.g. a 2x2x2 array would be have the elements listed in the order: (0,0,0), (0,0,1), (0,1,0), (0,1,1), (1,0,0), (1,0,1), (1,1,0), (1,1,1).

A simple example is:

```
h5fromtxt foo.h5 <<EOF — 
1 2 3 4
5 6 7 8
EOF
```

which reads in a 2x4 space-delimited array from standard input.

## Options

* `-h` — Display help on the command-line options and usage.

* `-V` — Print the version number and copyright info for `h5fromtxt`.

* `-v` — Verbose output.

* `-a` — If the HDF5 output file already exists, append the data as a new dataset rather than overwriting the file (the default behavior). An existing dataset of the same name within the file is overwritten, however.

* `-n size` — Instead of trying to infer the dimensions of the array from the rows and columns of the input, treat the data as a sequence of numbers in row-major order forming an array of dimensions `size`. `size` is of the form MxNxLx... (with M, N, L being numbers) and may be of any dimensionality.

* `-T` — Transpose the input when it is written, reversing the dimensions.

* `-d name` — Write to dataset `name` in the output; otherwise, the output dataset is called "data" by default. Alternatively, use the syntax `HDF5FILE:DATASET`.

## Bugs

Report bugs by filing an issue at https://github.com/stevengj/h5utils

## Authors

Written by [Steven G. Johnson](http://math.mit.edu/~stevenj/). Copyright © 2017 by the Massachusetts Institute of Technology.
