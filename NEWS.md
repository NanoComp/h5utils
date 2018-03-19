Here, we describe what has changed between releases of the [h5utils](README.md) package.

## h5utils-1.13.1 ##

*3/19/18*

* Fixed man page problems (#2 and #3), thanks to Bas Couwenberg.

## h5utils-1.13 ##

*7/18/17*

* Fixed `h5topng` compilation for modern libpng versions.  Thanks to
  Daisuke Fujimura (@fd00 on github) for posting patches.

* Moved hosting to Github and translated documentation to Markdown.

## h5utils 1.12.1 ##

*6/24/09*

* Use `octave-config`, if available, to detect octave-plugin installation path (thanks to Debian bug report #516453 for suggestion).

## h5utils 1.12 ##

*6/12/09*

* The vertical axis in `h5topng` is now reversed to correspond to what most people seem to expect: increasing coordinates correspond to "up" and "right" in the image, rather than "down" and "right" in the image as in previous versions.
* Fixed failure in `h5tovtk -2`; thanks to Karen Lee for the bug report.
* Fixed installation of `h5read.oct` for Octave 3.x.

## h5utils 1.11.1 ##

*4/28/08*

* Fixed failure to find colormap files in h5topng 1.11 (due to changes in autoconf 2.60); thanks to bug report from Jiangjun Zheng.

## h5utils 1.11 ##

*4/24/08*

* h5tovtk no longer reverses the dimensions; thanks to Andreas Wilde for the suggestion.
* Fix compilation failure with HDF5 1.8.

## h5utils 1.10.1 ##

*9/20/06*

* Fixed build problem on Cygwin due to missing "`.exe`" extension. Thanks to Ken Hill for the bug report.

## h5utils 1.10 ##

*9/2/05*

* Added `h4fromh5` utility.  (NCSA seems to be no longer shipping the HDF5/HDF4 conversion tools with the latest HDF5 release.)
* Added `dkbluered` color map, which is similar to `bluered` but uses a somewhat wider range of colors.

## h5utils 1.9.1 ##

*8/5/04*

* Fix `h5topng` compilation failure with some non-C99 compilers; thanks to Maarten van Reeuwijk for the bug report.

## h5utils 1.9 ##

*7/12/04*

* Added new `h5math` utility, which creates and combines HDF5 datasets using a user-specified mathematical expression. (Requires [GNU libmatheval](http://www.gnu.org/software/libmatheval/) to be installed.)
* `h5topng`: Matlab-like `start:end` or `start:step:end` notation for slice indices, to allow a whole sequence of slices to be output as a sequence of PNG images.
* `h5topng`: if contour/overlay dataset does not have same dimensions as output data, it is periodically "tiled" over the output.

## h5utils 1.8 ##

*5/22/04*

* New `-A` and `-a` options for `h5topng` to allow translucent overlays from one file onto another (an alternative or complement to the `-C` contour-overlay option).
* `h5topng` uses 24-bit direct color by default (use `-8` option for old 8-bit behavior).
* `h5topng` uses columns/rows for x/y by default (use `-T` to swap), the opposite of the old behavior.
* There is no default `-z 0` slice dimension in `h5topng`/`h5totxt` any more. You must specify a slice for 3+ dimensional data in `h5topng`. `h5totxt` dumps the whole data file by default unless one or more slices are specified.
* Support specifying multiple slice dimensions for 4+ dimensional datasets, with new `-t` option to indicate final dimension.
* Slices are also supported now in `h5tovtk` and `h5tov5d`.
* New `-.` option in `h5totxt` to specify number of significant digits; output 16 digits by default instead of 6, previously.

## h5utils 1.7.2 ##

*7/15/02*

* Fixed C++ compilation problem for `g++` 3.x in `h5read` Octave plugin; thanks to Josselin Mouette for the patch.

## h5utils 1.7.1 ##

*3/16/02*

* Fixed array overrun in `h5topng` that caused a floating-point exception on Alphas; thanks to Marin Soljacic for the bug report.

## h5utils 1.7 ##

*3/9/02*

* `h5topng` now supports multiple, user-definable color tables, a number of which are provided. '''Incompatible change''': the `-c` option now has the syntax: `-c ''colortable''`. The old behavior corresponds to the included "`bluered`" colortable, invoked via: `-c bluered`.
* New `-R` option for `h5topng` to use a consistent color scale for all specified files.
* New `-0` option for `h5topng` and `h5totxt` that shifts the origin of the slice coordinates to the dataset center.
* Added `h5tovtk` program to output VTK (Visualization ToolKit) data files.
* Support `-T` (transpose dimensions) option in `h5tov5d`.
* Fixed bug in `h5topng` that caused extra rows/columns of garbage pixels to be written at the edges of images when scaling was used.
* When compiling the h5read Octave plugin, respect the `CPPFLAGS` and `LDFLAGS` environment variables. Thanks to Max Colice for the bug report.
* Fixed problem when `--without-h5tov5d` and `--without-h5fromh4` are used. Thanks to Nikola Ivanov Nikolov for the fix.

## h5utils 1.6 ##

*1/17/01*

* Don't build `h5fromh4` if the superior `h4toh5` tool (from HDF5 1.4) is present. Also added `--{with,without}-h5fromh4` option to configure to force whether `h5fromh4` is built.

## h5utils 1.5.1 ##

*12/9/00*

* Support manually disabling Octave plugin support (`configure --without-octave`) in case of C++ problems.
* Support Vis5d+ and Debian HDF header file locations.

## h5utils 1.5 ##

*7/9/00*

* Added `h5fromh4` program to convert HDF4 datasets to HDF5 format.
* Added `-S ''s''` option to `h5topng` as a shortcut for `-X ''s'' -Y ''s''`.

## h5utils 1.4 ##

*5/28/00*

* Added `h5fromtxt` program to convert text input to an HDF5 dataset.

## h5utils 1.3.4 ##

*1/31/00*

* Improved -C contour plotting in h5topng.
* Fix in `h5topng` man page (thanks to Christoph Becher).

## h5utils 1.3.3 ##

*1/30/00*

* Bug fix in `h5topng` (would sometimes output solid black images). Thanks to Karl Koch for the bug report.

## h5utils 1.3.2 ##

*1/28/00*

* Added `h5topng -Z` option to center color scale on zero.
* Now support `h5topng -C ''filename:dataset''`.

## h5utils 1.3.1 ##

*1/27/00*

* Bug fixes in dataset name-handling, especially when using `h5tov5d` to join multiple datasets into one output file.

## h5utils 1.3 ##

*1/21/00*

* You can now specify individual datasets within a file by using ''filename'':''dataset'' instead of just ''filename'' with `h5topng`, `h5totxt`, and `h5tov5d`.

## h5utils 1.2.3 ##

*1/20/00*

* Fixed minor bug in '`h5totxt -h`'.

## h5utils 1.2.2 ##

*1/12/00*

* `Makefile` now includes `CPPFLAGS` in the compiler flags, making it easier to use header files in non-standard locations. (`CPPFLAGS` is the proper place to put `-I` flags for the configure script.)

## h5utils 1.2.1 ##

*1/1/00*

* Modified `-o` option of `h5tov5d` to join datasets into a single Vis5d file.

## h5utils 1.2 ##

*12/31/99*

* Added h5tov5d program for converting to Vis5d format.
* Improved installation; `h5read.oct` now goes into the sitewide Octave plugins directory, and things work correctly when only some of the utilities are compiled.

## h5utils 1.1 ##

*12/6/99*

* Added `h5totxt` program for exporting 2d slices of HDF5 files to text suitable for importing into a spreadsheet.

## h5utils 1.0 ##

*11/22/99*

* Initial release.
