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

#ifndef __ULIB_CONSOLE_H
#define __ULIB_CONSOLE_H

#define DEF_PROMPT "> "

/*
 * cmdlet function prototype
 */
typedef int (*console_fcn_t) (int argc, const char *argv[]);

typedef struct {
	void * idx;
	char * pmpt;
	char * rbuf;
	int    rfd;
	int    rbuflen;
} console_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * console_init - initializes console context
 * @ctx: console context
 */
	int console_init(console_t *ctx);

/**
 * console_pmpt - sets console prompt
 * @ctx:  console context
 * @pmpt: new prompt to use
 */
	int console_pmpt(console_t *ctx, const char *pmpt);

/**
 * console_bind - binds cmdlet to function f
 * @ctx:    console context
 * @cmdlet: cmdlet string
 * @f:      console function for @cmdlet
 */
	int console_bind(console_t *ctx, const char *cmdlet, console_fcn_t f);

/**
 * console_exec - executes one command line
 * @ctx: console context
 * @cmd: command to exec
 */
	int console_exec(console_t *ctx, const char *cmd);

/**
 * console_loop - enters execution loop
 * @ctx:   console context
 * @count: number of command to exec, -1 for infinite
 * @term:  the terminating cmdlet, can be NULL
 */
	int console_loop(console_t *ctx, int count, const char *term);

/**
 * console_destroy - destroys console context members
 * @ctx: console context to free
 * Note: frees members in @ctx not @ctx itself
 */
	void console_destroy(console_t *ctx);

#ifdef __cplusplus
}
#endif

#endif  /* __ULIB_CONSOLE_H */
