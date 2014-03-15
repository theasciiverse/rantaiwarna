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

#ifndef	RANTAIWARNA_H
#define	RANTAIWARNA_H

/*
 * select C99/POSIX:2004-compatible versions of vsnprintf/snprintf on UnixWare 7
 */
#if defined(__SCO_VERSION__) && !defined(__OPENSERVER__)
#define	vsnprintf	_xvsnprintf
#define	snprintf	_xsnprintf
#endif /* defined(__SCO_VERSION__) && !defined(__OPENSERVER__) */

extern char	*progname;
extern bool	curses_init;

#endif /* RANTAIWARNA_H */
