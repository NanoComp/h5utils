# h5tovtk: convert datasets in HDF5 files to VTK format

# Synopsis

    h5tovtk [OPTION]... [HDF5FILE]...

## Description

`h5tovtk` is a program to generate VTK data files from multidimensional datasets in HDF5 files. VTK, the Visualization ToolKit, is an open-source, freely available software system for 3D computer graphics, image processing, and visualization. VTK itself is a programming library, but it is also the basis for a number of end-user graphical visualization programs.

HDF5 is a free, portable binary format and supporting library developed by the National Center for Supercomputing Applications at the University of Illinois in Urbana-Champaign. A single `h5` file can contain multiple datasets; by default, `h5tovtk` takes the first dataset, but this can be changed via the `-d` option, or by using the syntax `HDF5FILE:DATASET`.

1d/2d/3d datasets are converted into 3d VTK datasets. Normally, a single scalar VTK dataset is output, but vectors and fields can be output via the `-o` option below.

A typical invocation is of the form `h5tovtk foo.h5`, which will output a VTK data file `foo.vtk` from the data in `foo.h5`.

## Options

* `-h` — Display help on the command-line options and usage.

* `-V` — Print the version number and copyright info for `h5tovtk`.

* `-v` — Verbose output.

* `-o file` — Save all the input datasets to a single VTK `file`. If there is only one dataset, it is output to a VTK scalar dataset; if there are three datasets, they are output as a VTK vector dataset; all other numbers of datasets are combined into a VTK field dataset.

 - Otherwise, the default behavior is to save each dataset to a separate VTK file, with the `.h5` suffix of the input filename replaced by `.vtk` in the output filename.

 - Only three-dimensional datasets may be written to the VTK file. If you have a four (or more) dimensional data set, then you must take a three-dimensional "slice" of the multi-dimensional data. To do this, you specify coordinates in one (or more) slice dimension(s), via the `-xyzt` options.

* `-1`, `-2`, `-4` — Use 1 , 2, or 4 bytes to store each data point in the output file. Fewer bytes require less storage and memory, but will decrease the resolution in the values. `-1` will break up the data values into one of 256 possible values (on a linear scale from the minimum to the maximum value in your data), `-2` will allow 65536 possible values, and `-4` (the default) will use 4-byte floating-point numbers for an "exact" representation.

* `-a` — Output in ASCII format; otherwise, VTK’s more compact, but less readable and somewhat less portable binary format is used.

* `-n` — For binary output (see `-a` above), by default the data is written in bigendian byte order, which is normally the order that VTK expects. However, some external tools and a few VTK classes use the native byte ordering instead (which may not be bigendian), and the `-n` option causes `h5tovtk` to output binary data in the native ordering.

* `-m min`, `-M max` — When `-1` or `-2` are used, the input data are converted to a linear integer scale. Normally, the bottom and top of this scale correspond to the minimum and maximum values in the data. Using the `-m` and `-M` options, you can make the bottom and top of the scale correspond to `min` and `max` instead, respectively. Data values below or above this range will be treated as if they were `min` or `max` respectively. See also the `-Z` option.

* `-Z` — For `-1` or `-2` output, center the linear integer scale on the value zero in the data.

* `-r` — Invert the output values (map the minimum to the maximum and vice versa).

* `-x ix`, `-y iy`, `-z iz`, `-t it` — This tells `h5tovtk` to use a particular slice of a multi-dimensional dataset. e.g. `-x` uses the subset (with one less dimension) at an x index of `ix` (where the indices run from zero to one less than the maximum index in that direction). Here, x/y/z correspond to the first/second/third dimensions of the HDF5 dataset. The `-t` option specifies a slice in the last dimension, whichever that might be. See also the `-0` option to shift the origin of the x/y/z slice coordinates to the dataset center.

* `-0` — Shift the origin of the x/y/z slice coordinates to the dataset center, so that e.g. -0 -x 0 (or more compactly -0x0) returns the central x plane of the dataset instead of the edge x plane. (`-t` coordinates are not affected.)

* `-d name` — Use dataset `name` from the input files; otherwise, the first dataset from each file is used. Alternatively, use the syntax `HDF5FILE:DATASET`, which allows you to specify a different dataset for each file. You can use the `h5ls` command (included with hdf5) to find the names of datasets within a file.

## Bugs

Report bugs by filing an issue at https://github.com/stevengj/h5utils

## Authors

Written by [Steven G. Johnson](http://math.mit.edu/~stevenj/). Copyright © 2017 by the Massachusetts Institute of Technology.
