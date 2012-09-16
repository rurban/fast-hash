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

#ifndef __ULIB_STRUTILS_H
#define __ULIB_STRUTILS_H

#ifdef __cplusplus
extern "C" {
#endif

        /**
	 * nextline - extracts the next line from memory
	 * returns the start of next line or NULL
	 * @buf:  buffer
	 * @size: buffer size
	 * Note:  the end of current line will be set to '\0', quite
	 * useful especially for concurrent line parsing.
	 */
	char *nextline(char *buf, long size);

	/**
	 * getfield - retrieves a field from arbitrary string
	 * @line:         line buffer
	 * @line_size:    line buffer size
	 * @field:        field buffer
	 * @field_size:   field buffer size
	 * @fid:          id of field to retrieve
	 * @delim:        delimiter character
	 * @Note:  '\0' is also a delimiter character
	 */
	const char *getfield(const char *line, long line_size, char *field,
			     long field_size, int fid, char delim);
	
	/**
	 * getlinefield - retrieves a field from line
	 * @line:         line buffer
	 * @line_size:    line buffer size
	 * @field:        field buffer
	 * @field_size:   field buffer size
	 * @fid:          id of field to retrieve
	 * @delim:        delimiter character
	 * @Note:  '\0' and '\n' are also delimiter characters
	 */
	const char *getlinefield(const char *line, long line_size, char *field,
				 long field_size, int fid, char delim);

#ifdef __cplusplus
}
#endif

#endif  /* __ULIB_STRUTILS_H */
