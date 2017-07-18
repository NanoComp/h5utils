# h5math: combine/create HDF5 files with math expressions

## Synopsis

    h5math [OPTION]... OUTPUT-HDF5FILE [INPUT-HDF5FILES...]

## Description

`h5math` takes any number of HDF5 files as input, along with a mathematical expression, and combines them to produce a new HDF5 file.

HDF5 is a free, portable binary format and supporting library developed by the National Center for Supercomputing Applications at the University of Illinois in Urbana-Champaign. A single `.h5` file can contain multiple data sets; by default, `h5math` creates a dataset called "h5math", but this can be changed via the `-d` option, or by using the syntax `HDF5FILE:DATASET`. The `-a` option can be used to append new datasets to an existing HDF5 file. The same syntax is used to specify the dataset used in the input file(s); by default, the first dataset (alphabetically) is used.

A simple example of `h5math` usage is:

    h5math -e "d1 + 2*d2" out.h5 foo.h5 bar.h5:blah

which produces a new file, `out.h5`, by adding the first dataset in `foo.h5` with twice the `blah` dataset in `bar.h5`. In the expression (specified by `-e`), the first input dataset (from left to right) is referred to as `d1`, the second as `d2`, and so on.

In addition to input datasets, you can also use the x/y/z coordinates of each point in the expression, referenced by `x y` and `z` variables (for the first three dimensions) as well as a `t` variable that refers to the last dimension. By default, these are integers starting at 0 at the corner of the dataset, but the `-0` option will change the x/y/z origin to the center of the dataset (t is unaffected), and the `-r res` option will specify the "resolution", dividing the x/y/z coordinates by `res`.

All of the input datasets must have the same dimensions, which are also the dimensions of the output. If there are no input files, and you are defining the output purely by a mathematical formula, you can specify the dimensions of the output explicitly via the `-n size` option, where `size` is e.g. "2x2x2".

Sometimes, however, you want to use only a smaller-dimensional "slice" of multi-dimensional data. To do this, you specify coordinates in one (or more) slice dimension(s), via the `-xyzt` options.

## Options

* `-h` — Display help on the command-line options and usage.

* `-V` — Print the version number and copyright info for `h5math`.

* `-v` — Verbose output.

* `-a` — If the HDF5 output file already exists, append the data as a new dataset rather than overwriting the file (the default behavior). An existing dataset of the same name within the file is overwritten, however.

* `-e expression` — Specify the mathematical expression that is used to construct the output (generally in `"` quotes to group the expression as one item in the shell), in terms of the variables for the input datasets and the coordinates as described above.
 - Expressions use a C-like infix notation, with most standard operators and mathematical functions (`+`, `sin`, etc.) being supported. This functionality is provided (and its features determined) by [GNU libmatheval](https://www.gnu.org/software/libmatheval/).

* `-f filename` — Name of a text file to read the expression from, if no `-e` expression is specified. Defaults to stdin.

* `-x ix`, `-y iy`, `-z iz`, `-t it` — This tells `h5math` to use a particular slice of a multi-dimensional dataset. e.g. `-x` uses the subset (with one less dimension) at an x index of `ix` (where the indices run from zero to one less than the maximum index in that direction). Here, x/y/z correspond to the first/second/third dimensions of the HDF5 dataset. The `-t` option specifies a slice in the last dimension, whichever that might be. See also the `-0` option to shift the origin of the x/y/z slice coordinates to the dataset center.

* `-0` — Shift the origin of the x/y/z slice coordinates to the dataset center, so that e.g. -0 -x 0 (or more compactly -0x0) returns the central x plane of the dataset instead of the edge x plane. (`-t` coordinates are not affected.) This also shifts the origin of the x/y/z variables in the expression so that 0 is the center of the dataset.

* `-r res` — Use a resolution `res` for x/y/z (but not t) variables in the expression, so that the data "grid" coordinates are divided by `res`. The default `res` is 1.
 - For example, if the x dimension has 21 grid steps, setting a `res` of 20 will mean that x variables in the expression run from 0.0 to 1.0 (or -0.5 to 0.5 if `-0` is specified), instead of 0 to 20.
 - `-r` does not affect the coordinates used for slices, which are always integers.

* `-n size` — The output dataset must be the same size as the input datasets. If there are no input datasets (if you are defining the output purely by a formula), then you must specify the output size manually with this option: `size` is of the form MxNxLx… (with M, N, L being integers) and may be of any dimensionality.

* `-d name` — Write to dataset `name` in the output; otherwise, the output dataset is called "data" by default. Also use dataset `name` in the input; otherwise, the first input dataset (alphabetically) in a file is used. Alternatively, use the syntax `HDF5FILE:DATASET` (which overrides the `-d` option).

## Bugs

Report bugs by filing an issue at https://github.com/stevengj/h5utils

## Authors

Written by [Steven G. Johnson](http://math.mit.edu/~stevenj/). Copyright © 2017 by the Massachusetts Institute of Technology.
