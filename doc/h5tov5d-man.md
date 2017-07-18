# h5tov5d: convert datasets in HDF5 files to Vis5d format

## Synopsis

    h5tov5d [OPTION]... [HDF5FILE]...

## Description

`h5tov5d` is a program to generate Vis5d data files from multidimensional datasets in HDF5 files. Vis5d is a free volumetric visualization program capable of displaying 3, 4, or even 5 dimensional datasets (using time for the 4th dimension and different variables for the 5th dimension).

HDF5 is a free, portable binary format and supporting library developed by the National Center for Supercomputing Applications at the University of Illinois in Urbana-Champaign. A single `h5` file can contain multiple data sets; by default, `h5tov5d` takes the first dataset, but this can be changed via the `-d` option, or by using the syntax `HDF5FILE:DATASET`.

1d/2d/3d datasets are converted into 3d Vis5d datasets. 4d datasets are converted into a time series of 3d datasets, with the first dimension marking the time. 5d datasets are converted into several variables of time series of 3d datasets, with the first dimension as the variable index and the second dimension as the time. Often, however, you want only a three-dimensional "slice" of four (or more) dimensional data. To do this, you specify coordinates in one (or more) slice dimension(s), via the `-xyzt` options.

A typical invocation is of the form `h5tov5d foo.h5`, which will output a Vis5d data file `foo.v5d` from the data in `foo.h5`.

## Options

* `-h` — Display help on the command-line options and usage.

* `-V` — Print the version number and copyright info for `h5tov5d`.

* `-v` — Verbose output.

* `-T` — Transpose the output dimensions (reverse their order).

* `-o` `file` — Save the datasets from all of the input files to a single Vis5d `file` with each dataset being expressed as a separate Vis5d variable. In this way, you can use Vis5d to superimpose and compare the plots from the different datasets. The first two dimensions (or three, for 4d datasets) must be the same for all of the input datasets.

 - Otherwise, the default behavior is to save each dataset to a separate Vis5d file, with the `.h5` suffix of the input filename replaced by `.v5d` in the output filename.

* `-1`, `-2`, `-4` — Use 1 (the default), 2, or 4 bytes to store each data point in the output file. Fewer bytes will cause Vis5d to be faster (as well as requiring less storage and memory), but will decrease the resolution in the values. `-1` will break up the data values into one of 256 possible values (on a linear scale from the minimum to the maximum value in your data), `-2` will allow 65536 possible values, and `-4` will use 4-byte floating-point numbers for an "exact" representation. In most circumstances, `-1` is more than adequate for data visualization purposes.

* `-x` `ix`, `-y` `iy`, `-z` `iz`, `-t` `it` — This tells `h5tov5d` to use a particular slice of a multi-dimensional dataset. e.g. `-x` uses the subset (with one less dimension) at an x index of `ix` (where the indices run from zero to one less than the maximum index in that direction). Here, x/y/z correspond to the first/second/third dimensions of the HDF5 dataset. The `-t` option specifies a slice in the last dimension, whichever that might be. See also the `-0` option to shift the origin of the x/y/z slice coordinates to the dataset center.

* `-0` — Shift the origin of the x/y/z slice coordinates to the dataset center, so that e.g. -0 -x 0 (or more compactly -0x0) returns the central x plane of the dataset instead of the edge x plane. (`-t` coordinates are not affected.)

* `-d` `name` — Use dataset `name` from the input files; otherwise, the first dataset from each file is used. Alternatively, use the syntax `HDF5FILE:DATASET`, which allows you to specify a different dataset for each file. You can use the `h5ls` command (included with hdf5) to find the names of datasets within a file.

## Bugs

Report bugs by filing an issue at https://github.com/stevengj/h5utils

## Authors

Written by [Steven G. Johnson](http://math.mit.edu/~stevenj/). Copyright © 2017 by the Massachusetts Institute of Technology.
