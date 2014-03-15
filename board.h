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

#ifndef	BOARD_H
#define	BOARD_H

#include <stdint.h>

enum {
	GAME_OVER = 1,
	GAME_OVER_CLEARED = 2
};

struct board_ctx {
	short		*elements;
	uint32_t	seed;
	int		height;
	int		width;
	short		colors;
	int32_t		score;
	bool		status;
};

struct board_ctx * board_create(int, int, short);
void	board_free(struct board_ctx *);
int	board_check_status(struct board_ctx *);
int	board_generate(struct board_ctx *);
int	board_compact(struct board_ctx *);
int	board_remove_elements(struct board_ctx *, int, int);

#endif /* BOARD_H */
