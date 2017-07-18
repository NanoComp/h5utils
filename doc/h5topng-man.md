# h5topng: generate PNG images from 2d slices of HDF5 files

# Synopsis

    h5topng [OPTION]... [HDF5FILE]...

## Description

`h5topng` is a utility to generate images in PNG (Portable Network Graphics) format from two-dimensional slices of datasets in HDF5 files. It is designed for quick-and-dirty visualization of scientific data, and for batch processing thereof via shell scripts.

HDF5 is a free, portable binary format and supporting library developed by the National Center for Supercomputing Applications at the University of Illinois in Urbana-Champaign. A single `.h5` file can contain multiple data sets; by default, `h5topng` takes the first dataset, but this can be changed via the `-d` option, or by using the syntax `HDF5FILE:DATASET`.

For a three- or four-dimensional dataset you must specify coordinates in one or two slice dimensions, respectively, to get a two-dimensional slice, via the `-xyzt` options. Yet more options control things like the colormap and magnification. Still, the most basic usage is something like `h5topng foo.h5`, which will output a file `foo.png` containing an image from the two-dimensional data in `foo.h5`.

## Options

* `-h` — Display help on the command-line options and usage.

* `-V` — Print the version number and copyright info for `h5topng`.

* `-v` — Verbose output. This output includes the minimum and maximum values encountered in the data, which is useful to know for the `-mM` options.

* `-o file` — Send PNG output to `file` rather than to the filename with .h5 replaced with .png (the default).

* `-x ix`, `-y iy`, `-z iz`, `-t it` — This tells `h5topng` to use a particular slice of a multi-dimensional dataset. e.g. `-x` causes a yz plane (of a 3d dataset) to be used, at an x index of `ix` (where the indices run from zero to one less than the maximum index in that direction). Here, x/y/z correspond to the first/second/third dimensions of the HDF5 dataset. The `-t` option specifies a slice in the last dimension, whichever that might be. See also the `-0` option to shift the origin of the x/y/z slice coordinates to the dataset center.
 - Instead of specifying a single index as an argument to these options, you can also specify a range of indices in a Matlab-like notation: `start:step:end` or `start:end` (`step` defaults to 1). This loops over that slice index, from `start` to `end` in steps of `step`, producing a sequence of output PNG files (with the slice index appended to the filename, before the `.png`).

* `-0` — Shift the origin of the x/y/z slice coordinates to the dataset center, so that e.g. `-0 -x 0` (or more compactly `-0x0`) returns the central x plane of the dataset instead of the edge x plane. (`-t` coordinates are not affected.)

* `-X scalex`, `-Y scaley`, `-S scale` — Scale the x and y dimensions of the image by `scalex` and `scaley` respectively. The `-S` option scales both x and y. The default is to use scale factors of 1.0; i.e. the image has the same dimensions (in pixels) as the data. Linear interpolation is used to fill in the pixels when the scale factors are not 1.0.

* `-s skewangle` — Skew the image by `skewangle` (in degrees) to the left or right. The result is a parallelogram, with the leftover space in the (square) image filled with either black or white pixels, depending upon the color map.

* `-T` — Transpose the data (interchange the image axes). By default, the first (x) coordinate of the data corresponds to the columns, and the second (y) coordinate corresponds to the rows; transposition reverses this convention.

* `-c colormap` — Use a color map `colormap` rather than the default `gray` color map (a grayscale ramp from white to black). `colormap` is normally the name of one of the color maps provided with `h5topng` (in the `/usr/local/share/h5utils/colormaps` directory), or can instead be the name of a color-map file.
 - Three useful included color maps are `inferno` (black-red-yellow, useful for intensity data), `RdBu` (red-white-blue, useful for signed data), and `viridis` (a blue-green-yellow color map), all of which are [adapted from Matplotlib](https://matplotlib.org/users/colormaps.html). If you use the `RdBu` color map for signed data, you may also want to use the `-Z` option so that the center of the color scale (white) corresponds to zero.
 - See [color tables in h5topng](h5topng-colors.md) for more information.

* `-r` — Reverse the ordering of the color map. You can also accomplish this by putting a `-` before the colormap name in the `-c` or `-a` option, e.g. `-c -RdBu`.

* `-Z` — Center the color scale on the value zero in the data.

* `-m min`, `-M max` — Normally, the bottom and top of the color map correspond to the minimum and maximum values in the data. Using these options, you can make the bottom and top of the color map correspond to `min` and `max` instead. Data values below or above this range will be treated as if they were `min` or `max` respectively. See also the `-Z` and `-R` options.

* `-R` — When multiple files are specified, set the bottom and top of the color maps according to the minimum and maximum over all the data. This is useful to process many files using a consistent color scale, since otherwise the scale is set for each file individually.

* `-C file`, `-b val` — Superimpose contour outlines from the first dataset in the `file` HDF5 file on all of the output images. (If the contour dataset does not have the same dimensions as the output data, it is periodically "tiled" over the output.) You can use the syntax `file:dataset` to specify a particular dataset within the file. The contour outlines are around a value of `val` (defaults to middle of value range in `file`).

* `-A file`, `-a colormap`:`opacity` — Translucently overlay the data from the first dataset in the `file` HDF5 file, which should have the same dimensions as the input dataset, on all of the output images, using the colormap `colormap` with opacity (from 0 for completely transparent to 1 for completely opaque) `opacity` multiplied by the opacity (alpha) values in the colormap. (If the overlay dataset does not have the same dimensions as the output data, it is periodically "tiled" over the output.) You can use the syntax `file:dataset` to specify a particular dataset within the file.

- Some predefined colormaps that work particularly well for this feature are `yellow` (transparent white to opaque yellow) `gray` (transparent white to opaque black), `yarg` (transparent black to opaque white), `green` (transparent white to opaque green), and `bluered` (opaque blue to transparent white to opaque red). You can prepend `-` to the colormap name to reverse the colormap order. (See also `-c`, above.) The default for `-a` is `yellow:0.3` (yellow colormap multiplied by 30% opacity).

* `-d name` — Use dataset `name` from the input files; otherwise, the first dataset from each file is used. Alternatively, use the syntax `HDF5FILE:DATASET`, which allows you to specify a different dataset for each file. You can use the `h5ls` command (included with hdf5) to find the names of datasets within a file.

* `-8` — Use 8-bit (indexed) color for the PNG output, instead of 24-bit (direct) color (the default). (This shrinks the image size slightly, with some degradation in quality.) Not supported in conjunction with the `-A` (translucent overlay) option.

## Bugs

Report bugs by filing an issue at https://github.com/stevengj/h5utils

## Authors

Written by [Steven G. Johnson](http://math.mit.edu/~stevenj/). Copyright © 2017 by the Massachusetts Institute of Technology.
