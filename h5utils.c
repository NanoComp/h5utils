/* Copyright (c) 1999-2017 Massachusetts Institute of Technology
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
#include <string.h>

#include "config.h"
#include "h5utils.h"

#define CHECK(cond, msg) { if (!(cond)) { fprintf(stderr, "h5utils error: %s\n", msg); exit(EXIT_FAILURE); } }

char *my_strdup(const char *s)
{
     char *sd = (char *) malloc(sizeof(char) * (strlen(s) + 1));
     CHECK(sd, "out of memory");
     strcpy(sd, s);
     return sd;
}

char *replace_suffix(const char *s, const char *old_suff, const char *new_suff)
{
     char *new_s;
     int s_len, old_suff_len, new_suff_len;

     s_len = strlen(s);
     old_suff_len = strlen(old_suff);
     new_suff_len = strlen(new_suff);

     new_s = (char*) malloc(sizeof(char) * (s_len + new_suff_len + 1));
     CHECK(new_s, "out of memory");

     strcpy(new_s, s);
     if (s_len >= old_suff_len && !strcmp(new_s + s_len - old_suff_len,
                                          old_suff))
          new_s[s_len - old_suff_len] = 0;  /* delete old suffix */
     strcat(new_s, new_suff);
     return new_s;
}

/* given an fname of the form <filename>:<data_name>, return a pointer
   to a newly-allocated string containing <filename>, and point data_name
   to the position of <data_name> in fname.  The user must free() the
   <filename> string. */
char *split_fname(char *fname, char **data_name)
{
     int fname_len;
     char *colon, *filename;

     fname_len = strlen(fname);
     colon = strchr(fname, ':');
     if (colon) {
          int colon_len = strlen(colon);
          filename = (char*) malloc(sizeof(char) * (fname_len-colon_len+1));
          CHECK(filename, "out of memory");
          strncpy(filename, fname, fname_len-colon_len+1);
	  filename[fname_len-colon_len] = 0;
          *data_name = colon + 1;
     }
     else { /* treat as if ":" were at the end of fname */
          filename = (char*) malloc(sizeof(char) * (fname_len + 1));
          CHECK(filename, "out of memory");
          strcpy(filename, fname);
          *data_name = fname + fname_len;
     }
     return filename;
}

