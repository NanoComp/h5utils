/* Copyright (c) 1999, 2000, 2001, 2002 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <octave/oct.h>

#include "arrayh5.h"

DEFUN_DLD(h5read, args, , 
"h5read(filename [, slicedim, islice, dataname])\n"
"Read a 1d or 2d array slice from an HDF5 file.\n\n"
"slicedim and islice are optional parameters indicating a \"slice\" of a\n"
"multidimensional dataset, where slicedim is \"x\", \"y\", or \"z\", and\n"
"islice is the index in that dimension.  The default is slicedim=\"z\" and\n"
"islice=0, meaning the xy plane at z index 0 is read.\n\n"
"The optional parameter dataname indicates the name of the dataset to read\n"
"within the HDF5 file.  The default is to read the first dataset.\n"
)
{
     std::string fname, dataname;
     octave_value retval;
     arrayh5 a;
     int readerr;
     int slicedim = 2, islice = 0, center_slice = 0;
     
     if (args.length() < 1 || args.length() > 4 || !args(0).is_string()
	 || (args.length() >= 2 && !args(1).is_string())
	 || (args.length() >= 3 && !args(2).is_real_scalar())
	 || (args.length() >= 4 && !args(3).is_string())) {
	  print_usage("h5read");
	  return retval;
     }
     
     fname = args(0).string_value();
     if (args.length() >= 2)
	  slicedim = tolower(*(args(1).string_value().c_str())) - 'x';
     if (args.length() >= 3)
	  islice = (int) (args(2).double_value() + 0.5);
     
     readerr = arrayh5_read(&a, fname.c_str(),
			    args.length() >= 4 ? 
			    args(3).string_value().c_str() : NULL, 
			    NULL, 1, &slicedim, &islice, &center_slice);
     if (readerr) {
	  fprintf(stderr, "error in h5read: %s\n",
		  arrayh5_read_strerror[readerr]);
	  return retval;
     }

     if (a.rank >= 2) {
	  Matrix m(a.dims[0], a.dims[1]);

	  for (int i = 0; i < a.dims[0]; ++i)
	       for (int j = 0; j < a.dims[1]; ++j)
		    m(i,j) = a.data[i*a.dims[1] + j];

	  retval = m;
     }
     else if (a.rank == 1) {
	  ColumnVector v(a.dims[0]);

	  for (int i = 0; i < a.dims[0]; ++i)
	       v(i) = a.data[i];

	  retval = v;
     }
     else {
	  retval = a.data[0];  /* scalar (rank = 0) */
     }
     
     arrayh5_destroy(a);
     
     return retval;
}
