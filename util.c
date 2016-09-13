/*
 * Copyright (C) 2014 Guido Berhoerster <guido+rantaiwarna@berhoerster.name>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
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
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <curses.h>
#include <term.h>
#include <stdarg.h>

#include "util.h"
#include "compat.h"
#include "rantaiwarna.h"

void
restore_term(void) {
	if (curses_init) {
		endwin();
		curses_init = FALSE;
		curs_set(1);
	}
}

void
rantaiwarna_err(int eval, const char *fmt, ...)
{
	int	saved_errno = errno;
	va_list	args;

	restore_term();

	fprintf(stderr, "%s: ", progname);
	va_start(args, fmt);
	if (fmt != NULL) {
		vfprintf(stderr, fmt, args);
		fprintf(stderr, ": ");
	}
	va_end(args);
	fprintf(stderr, "%s\n", strerror(saved_errno));

	exit(eval);
}

void
rantaiwarna_errx(int eval, const char *fmt, ...)
{
	va_list	args;

	restore_term();

	fprintf(stderr, "%s: ", progname);
	va_start(args, fmt);
	if (fmt != NULL) {
		vfprintf(stderr, fmt, args);
	}
	va_end(args);
	fputc('\n', stderr);

	exit(eval);
}

void *
rantaiwarna_malloc(size_t size)
{
	void	*ptr;

	ptr = malloc(size);
	if (ptr == NULL) {
		rantaiwarna_err(1, NULL);
	}
	return (ptr);
}

void *
rantaiwarna_calloc(size_t nelem, size_t elsize)
{
	void	*ptr;

	ptr = calloc(nelem, elsize);
	if (ptr == NULL) {
		rantaiwarna_err(1, NULL);
	}
	return (ptr);
}

uint32_t
rantaiwarna_rand(uint32_t *seed)
{
	uint32_t	x = *seed;

	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	*seed = x;

	return (x);
}
