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

#define	_XOPEN_SOURCE	600

#include <stdlib.h>
#include <unistd.h>
#include <curses.h>	/* for OK, ERR */

#include "board.h"
#include "compat.h"
#include "util.h"

struct stack_element {
	struct stack_element	*next;
	int	x;
	int	y;
};

struct board_ctx *
board_create(int height, int width, short colors)
{
	struct board_ctx *board;

	board = rantaiwarna_malloc(sizeof (struct board_ctx));
	board->elements = rantaiwarna_calloc((size_t)(width * height),
	    sizeof (short));
	board->seed = (uint32_t)getpid();
	board->height = height;
	board->width = width;
	board->colors = colors;
	board->status = GAME_OVER;
	board->score = 0;
	if (board_generate(board) == ERR) {
		board_free(board);
		return (NULL);
	}

	return (board);
}

void
board_free(struct board_ctx *board)
{
	free(board->elements);
	free(board);
}

int
board_check_status(struct board_ctx *board)
{
	int	status = GAME_OVER | GAME_OVER_CLEARED;
	int	y;
	int	x;

	for (y = board->height - 1; y >= 0; y--) {
		for (x = 0; x < board->width; x++) {
			if (board->elements[y * board->width + x] != 0) {
				status &= ~GAME_OVER_CLEARED;
				if (((y - 1 >= 0) &&
				    (board->elements[y * board->width + x] ==
				    board->elements[(y - 1) * board->width +
				    x])) ||
				    ((x + 1 < board->width) &&
				    (board->elements[y * board->width + x] ==
				    (board->elements[y * board->width + x +
				    1])))) {
					status &= ~GAME_OVER;
					goto out;
				}
			}
		}
	}

out:
	board->status = status;
	return (status);
}

int
board_generate(struct board_ctx *board)
{
	int	i;
	int	j;

	/* make up to 1000 attempts to create a board that is playable */
	for (i = 0; i < 1000; i++) {
		for (j = 0; j < board->width * board->height; j++) {
			board->elements[j] =
			    (short)(rantaiwarna_rand(&board->seed) %
			    board->colors) + 1;
		}
		if (!(board_check_status(board) & GAME_OVER)) {
			break;
		}
	}

	board->score = 0;

	return (!(board->status & GAME_OVER) ? OK : ERR);
}

int
board_compact(struct board_ctx *board)
{
	int	changes = 0;
	int	x;
	int	y;
	int	top;
	int	left;

	/* close vertical gaps iby moving elements down */
	for (x = 0; x < board->width; x++) {
		for (y = top = board->height - 1; y >= 0; y--) {
			if (board->elements[y * board->width + x] != 0) {
				board->elements[top * board->width + x] =
				    board->elements[y * board->width + x];
				top--;
				changes++;
			}
		}
		for (y = top; y >= 0; y--) {
			board->elements[y * board->width + x] = 0;
		}
	}

	/* close column gaps by moving columns to the left */
	for (x = left = 0; x < board->width; x++) {
		if (board->elements[(board->height - 1) * board->width + x] !=
		    0) {
			if (x > left) {
				for (y = 0; y < board->height; y++) {
					board->elements[y * board->width +
					    left] = board->elements[y *
					    board->width + x];
				}
				changes++;
			}
			left++;
		}
	}
	for (x = left; x < board->width; x++) {
		for (y = 0; y < board->height; y++) {
			board->elements[y * board->width + x] = 0;
		}
	}

	return (changes);
}

int
board_remove_elements(struct board_ctx *board, int start_y, int start_x)
{
	int	removed = 0;
	int	color;
	int	x;
	int	y;
	int	west;
	int	east;
	int	n = 0;
	int	i;
	int	save_x;
	int	save_y;
	struct stack_element *stack_start;
	struct stack_element *stack_tmp;
	struct stack_element *stack_new;

	color = board->elements[start_y * board->width + start_x];
	if (color == 0) {
		return (0);
	}

	stack_new = rantaiwarna_malloc(sizeof (struct stack_element));
	stack_new->next = NULL;
	stack_new->y = start_y;
	stack_new->x = start_x;
	stack_start = stack_new;

	while (stack_start != NULL) {
		x = stack_start->x;
		y = stack_start->y;
		stack_tmp = stack_start;
		stack_start = stack_start->next;
		free(stack_tmp);

		if (board->elements[y * board->width + x] != color) {
			continue;
		}

		for (west = x; (west - 1 >= 0) &&
		    (board->elements[y * board->width + west - 1] == color);
		    west--);
		for (east = x; (east + 1 < board->width) &&
		    (board->elements[y * board->width + east + 1] == color);
		    east++);
		for (i = west; i <= east; i++) {
			/*
			 * only really start removing elements if there are
			 * more than two adjacent elements of the same color
			 */
			n++;
			if (n < 2) {
				save_x = i;
				save_y = y;
			} else {
				if (n == 2) {
					board->elements[save_y * board->width +
					    save_x] = 0;
					removed++;
				}
				board->elements[y * board->width + i] = 0;
				removed++;
			}

			if ((y - 1 >= 0) &&
			    (board->elements[(y - 1) * board->width + i] ==
			    color)) {
				stack_new = rantaiwarna_malloc(
				    sizeof (struct stack_element));
				stack_new->next = stack_start;
				stack_new->y = y - 1;
				stack_new->x = i;
				stack_start = stack_new;
			}
			if ((y + 1 < board->height) &&
			    (board->elements[(y + 1) * board->width + i] ==
			    color)) {
				stack_new = rantaiwarna_malloc(
				    sizeof (struct stack_element));
				stack_new->next = stack_start;
				stack_new->y = y + 1;
				stack_new->x = i;
				stack_start = stack_new;
			}
		}
	}
	if (removed > 0) {
		board_compact(board);
		board->score += (removed - 1) * (removed - 1);
		board_check_status(board);
		if (board->status & GAME_OVER) {
			if (board->status & GAME_OVER_CLEARED) {
				board->score += 1000;
			}
		}
	}

	return (removed);
}
