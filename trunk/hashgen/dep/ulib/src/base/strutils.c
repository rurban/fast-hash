/* The MIT License

   Copyright (C) 2011 Zilong Tan (eric.zltan@gmail.com)

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#include <stdio.h>
#include "strutils.h"

char *nextline(char *buf, long size)
{
        char *end = buf + size;

        while (buf < end && *buf != '\n' && *buf != '\0')
		buf++;

        if (buf >= end)
		return NULL;

        *buf++ = '\0';

	return (buf >= end? NULL: buf);
}

const char *getfield(const char *line, long line_size, char *field,
		     long field_size, int fid, char delim)
{
        const char *end = line + line_size;
	const char *p;

        while (fid-- > 0 && line < end) {
		/* shift right one field */
		while (line < end && *line && *line != delim)
			line++;
		if (line < end) {
			if (*line == delim)
				line++;
			else if (*line == '\0')
				return NULL;
		}
        }

        if (fid == -1 && *line && line < end) {
		if (field) {
			for (p = line; p < end && *p && *p != delim; p++) {
				if (p - line >= field_size - 1)
					break;
				*field++ = *p;
			}
			*field = '\0';
		}
		return line;
        }

        return NULL;
}

const char *getlinefield(const char *line, long line_size, char *field,
			 long field_size, int fid, char delim)
{
        const char *end = line + line_size;
	const char *p;

        while (fid-- > 0 && line < end) {
		/* shift right one field */
		while (line < end && *line && *line != '\n' && *line != delim)
			line++;
		if (line < end) {
			if (*line == delim)
				line++;
			else if (*line == '\0' || *line == '\n')
				return NULL;
		}
        }

        if (fid == -1 && *line && line < end) {
		if (field) {
			for (p = line; p < end && *p && *p != '\n' && *p != delim; p++) {
				if (p - line >= field_size - 1)
					break;
				*field++ = *p;
			}
			*field = '\0';
		}
		return line;
        }

        return NULL;
}
