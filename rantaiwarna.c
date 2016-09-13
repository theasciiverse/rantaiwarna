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
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <locale.h>
#include <curses.h>
#include <term.h>

#include "rantaiwarna.h"
#include "compat.h"
#include "util.h"
#include "board.h"
#include "highscore.h"

#define	EXIT_USAGE	2
#define	HEIGHT_MAX	INT16_MAX
#define	HEIGHT_MIN	5
#define	WIDTH_MAX	INT16_MAX
#define	WIDTH_MIN	5
#define	DEFAULT_WIDTH	20
#define	DEFAULT_HEIGHT	10
#define	COLORS_MAX	6
#define	DEFAULT_COLORS	4
#define	ELEMENT_WIDTH	2
#define	SCORE_WIN_WIDTH	6
#define	HIGHSCORE_FMT	"%2d) %s %6" PRId32
#define	TIMESTR_MAX_SIZE 64
#define	MESSAGE_MAX_SIZE 64

enum {
	PIPE_R_FD = 0,
	PIPE_W_FD
};

enum {
	GAME_SCREEN,
	HIGHSCORE_SCREEN,
	HELP_SCREEN
};

struct rantaiwarna_ctx {
	struct board_ctx *board;
	struct highscore_entry *highscore;
	WINDOW		*game_header_win;
	WINDOW		*message_win;
	WINDOW		*score_win;
	WINDOW		*board_container_win;
	WINDOW		*board_win;
	WINDOW		*highscore_pad;
	WINDOW		*highscore_header_win;
	WINDOW		*help_pad;
	WINDOW		*help_header_win;
	WINDOW		*input_win;
	char		message[MESSAGE_MAX_SIZE];
	char		dummy_timestr[TIMESTR_MAX_SIZE];
	const chtype	*elements_chs;
	int		board_cursor_y;
	int		board_cursor_x;
	int		highscore_pos_y;
	int		highscore_pos_x;
	int		help_pos_y;
	int		help_pos_x;
	int		active_screen;
	bool		use_colors;
	bool		too_small;
};

char		*progname;
bool		curses_init = FALSE;
static int	signal_pipe_fd[2] = { -1, -1 };
static const chtype elements_chs[COLORS_MAX] = { ' ', '@', '#', '$', 'X', 'O' };
static const char *help_strs[] = {
	"rantaiwarna is a tile-matching puzzle game which is also known under "
	"the names ",
	"\"Chain Shot!\" or \"SameGame\". It is played on a rectangular board "
	"which is ",
	"initially filled with elements of several different colors. Two or "
	"more ",
	"adjacent elements of the same color may be eliminated, the score "
	"resulting from ",
	"the elimination of elements depends on the number of elements "
	"eliminated at ",
	"once. The goal of the game is to eliminate as many elements as "
	"possible until ",
	"there are no more adjacent elements of the same color left or the "
	"board is ",
	"completely cleared. Vertical gaps resulting from the elimination of "
	"elements ",
	"are filled by sliding down elements from above the gap, column gaps "
	"are filled ",
	"by sliding columns on the right side of the column gap to the left.",
	"",
	"Game Screen Commands",
	"",
	"    Arrow up, right, down, left",
	"        move the cursor around",
	"",
	"    h, j, k, l",
	"        same as arrow keys",
	"",
	"    Space",
	"        eliminate elements under the cursor",
	"",
	"    Enter",
	"        same as Space",
	"",
	"    Left mouse button",
	"        same as Space",
	"",
	"    Ctrl+l",
	"        refresh the screen",
	"",
	"    H",
	"        switch to the help screen",
	"",
	"    i",
	"        switch to the highscore screen",
	"",
	"    n",
	"        start a new game",
	"",
	"    q",
	"        quit the game",
	"",
	"Highscore Screen Commands",
	"",
	"    Arrow up, right, down, left",
	"        scroll on line up or down or one character to the left or"
	"right",
	"",
	"    h, j, k, l",
	"        same as arrow keys",
	"",
	"    Space",
	"        scroll down one line",
	"",
	"    Enter",
	"        same as Space",
	"",
	"    Page up, Page down",
	"        scroll up or down one screenful",
	"",
	"    b, f",
	"        same as Page up or Page down",
	"",
	"    Ctrl+b, Ctrl+f",
	"        same as Page up or Page down",
	"",
	"    Home, End",
	"        go to the first or last line",
	"",
	"    g, G,",
	"        same as Home or End",
	"",
	"    Ctrl+l",
	"        refresh the screen",
	"",
	"    a",
	"        switch to the game screen",
	"",
	"    H",
	"        switch to the help screen",
	"",
	"    q",
	"        quit the game",
	"",
	"Help Screen Commands",
	"",
	"    Arrow up, right, down, left",
	"        scroll on line up or down or one character to the left or "
	"right",
	"",
	"    h, j, k, l",
	"        same as arrow keys",
	"",
	"    Space",
	"        scroll down one line",
	"",
	"    Enter",
	"        same as Space",
	"",
	"    Page up, Page down",
	"        scroll up or down one screenful",
	"",
	"    b, f",
	"        same as Page up or Page down",
	"",
	"    Ctrl+b, Ctrl+f",
	"        same as Page up or Page down",
	"",
	"    Home, End",
	"        go to the first or last line",
	"",
	"    g, G,",
	"        same as Home or End",
	"",
	"    Ctrl+l",
	"        refresh the screen",
	"",
	"    a",
	"        switch to the game screen",
	"",
	"    i",
	"        switch to the highscore screen",
	"",
	"    q",
	"        quit the game",
	"",
	NULL
};

static void
on_signal(int signo)
{
	int		old_errno = errno;
	ssize_t		n;
	sigset_t	sigset;

	/* try to read unread signals from the pipe and add the new one to it */
	n = read(signal_pipe_fd[PIPE_R_FD], &sigset, sizeof (sigset));
	if (n == -1 || (size_t)n < sizeof (sigset)) {
		sigemptyset(&sigset);
	}
	sigaddset(&sigset, signo);
	write(signal_pipe_fd[PIPE_W_FD], &sigset, sizeof (sigset));

	errno = old_errno;
}

static int
rantaiwarna_doupdate(struct rantaiwarna_ctx *ctx)
{
	/*
	 * ensure the physical cursor is a sensible place in case it could not
	 * be made invisible
	 */
	if ((ctx->active_screen == GAME_SCREEN) && (ctx->board_win != NULL)) {
		wmove(ctx->board_win, ctx->board_cursor_y,
		    ctx->board_cursor_x * ELEMENT_WIDTH);
		wnoutrefresh(ctx->board_win);
	}
	return (doupdate());
}

static void
board_win_render_cursor(struct rantaiwarna_ctx *ctx, bool visible)
{
	/* highlighting color pairs are base color + COLORS_MAX */
	short	color = ctx->board->elements[ctx->board_cursor_y *
	    ctx->board->width + ctx->board_cursor_x] +
	    (visible ? COLORS_MAX : 0);
	int	i;
	chtype	ch;

	for (i = 0; i < ELEMENT_WIDTH; i++) {
		ch = mvwinch(ctx->board_win, ctx->board_cursor_y,
		    ctx->board_cursor_x * ELEMENT_WIDTH + i);
		if (visible) {
			/* use reverse and underline for highlighting */
			ch = (ch & ~A_COLOR) | A_REVERSE | A_UNDERLINE;
		} else {
			ch &= ~(A_COLOR | A_REVERSE | A_UNDERLINE);
		}
		if (ctx->use_colors) {
			ch |= COLOR_PAIR(color);
		}
		mvwaddch(ctx->board_win, ctx->board_cursor_y,
		    ctx->board_cursor_x * ELEMENT_WIDTH + i, ch);
	}
}

static void
board_win_update_cursor(struct rantaiwarna_ctx *ctx, bool visible)
{
	board_win_render_cursor(ctx, visible);
	touchwin(ctx->board_container_win);
	wnoutrefresh(ctx->board_win);
}

static int
board_win_move_cursor(struct rantaiwarna_ctx *ctx, int y, int x)
{
	if ((x < 0) || (x >= ctx->board->width) || (y < 0) ||
	    (y >= ctx->board->height)) {
		return (ERR);
	}

	board_win_update_cursor(ctx, FALSE);
	ctx->board_cursor_x = x;
	ctx->board_cursor_y = y;
	board_win_update_cursor(ctx, TRUE);

	return (OK);
}

static void
score_win_render(struct rantaiwarna_ctx *ctx)
{
	mvwprintw(ctx->score_win, 0, 0, "%*" PRId32, SCORE_WIN_WIDTH,
	    ctx->board->score);
}

static void
score_win_update(struct rantaiwarna_ctx *ctx)
{
	score_win_render(ctx);
	touchwin(ctx->game_header_win);
	wnoutrefresh(ctx->score_win);
}

static void
set_message(struct rantaiwarna_ctx *ctx, const char *message)
{
	snprintf(ctx->message, MESSAGE_MAX_SIZE, "%s", message);
}

static void
message_win_render(struct rantaiwarna_ctx *ctx)
{
	int	game_header_win_height;
	int	game_header_win_width;
	int	message_win_begy;
	int	message_win_begx;

	getmaxyx(ctx->game_header_win, game_header_win_height,
	    game_header_win_width);
	getbegyx(ctx->message_win, message_win_begy, message_win_begx);
	wclear(ctx->message_win);
	mvwaddstr(ctx->message_win, 0, (game_header_win_width -
	    (int)strlen(ctx->message)) / 2 - message_win_begx, ctx->message);
}

static void
message_win_update(struct rantaiwarna_ctx *ctx)
{
	message_win_render(ctx);
	touchwin(ctx->game_header_win);
	wnoutrefresh(ctx->message_win);
}

static void
board_win_render(struct rantaiwarna_ctx *ctx)
{
	chtype	ch;
	int	x;
	int	y;
	short	color;
	int	i;

	for (x = 0; x < ctx->board->width; x++) {
		for (y = 0; y < ctx->board->height; y++) {
			color = ctx->board->elements[y * ctx->board->width + x];
			ch = ctx->elements_chs[color];
			if (ctx->use_colors) {
				ch |= COLOR_PAIR(color);
			}
			for (i = 0; i < ELEMENT_WIDTH; i++) {
				mvwaddch(ctx->board_win, y,
				    x * ELEMENT_WIDTH + i, ch);
			}
		}
	}
}

static void
board_win_update(struct rantaiwarna_ctx *ctx)
{
	board_win_render(ctx);
	if ((ctx->board->status & GAME_OVER) == 0) {
		board_win_update_cursor(ctx, TRUE);
	}
	touchwin(ctx->board_container_win);
	wnoutrefresh(ctx->board_win);
}

static void
highscore_pad_update(struct rantaiwarna_ctx *ctx)
{
	struct highscore_entry *entry;
	int	i;
	int32_t	score;
	char	timestr[TIMESTR_MAX_SIZE];
	char	*timestrp;
	int	height;
	int	width;
	int	highscore_pad_height;
	int	highscore_pad_width;

	for (entry = ctx->highscore, i = 0; i < 10; i++) {
		if (entry != NULL) {
			score = entry->score;
			timestrp = timestr;
			if (strftime(timestr, sizeof (timestr), "%x",
			    &entry->time) == 0) {
				timestr[0] = '\0';
			}
			entry = entry->next;
		} else {
			score = 0;
			timestrp = ctx->dummy_timestr;
		}
		mvwprintw(ctx->highscore_pad, i, 0, HIGHSCORE_FMT, i + 1,
		    timestrp, score);
	}

	getmaxyx(stdscr, height, width);
	getmaxyx(ctx->highscore_pad, highscore_pad_height, highscore_pad_width);
	if (ctx->highscore_pos_y + height > highscore_pad_height) {
		ctx->highscore_pos_y = MAX(highscore_pad_height - height, 0);
	}
	if (ctx->highscore_pos_x + width > highscore_pad_width) {
		ctx->highscore_pos_x = MAX(highscore_pad_width - width, 0);
	}
}

static int
highscore_pad_refresh(struct rantaiwarna_ctx *ctx)
{
	int	height;
	int	width;
	int	highscore_pad_height;
	int	highscore_pad_width;

	getmaxyx(stdscr, height, width);
	getmaxyx(ctx->highscore_pad, highscore_pad_height, highscore_pad_width);

	return (pnoutrefresh(ctx->highscore_pad, ctx->highscore_pos_y,
	    ctx->highscore_pos_x, 1, 0, MIN(height - 1, highscore_pad_height -
	    1), MIN(width - 1, highscore_pad_width - 1)));
}

static int
help_pad_refresh(struct rantaiwarna_ctx *ctx)
{
	int	height;
	int	width;
	int	help_pad_height;
	int	help_pad_width;

	getmaxyx(stdscr, height, width);
	getmaxyx(ctx->help_pad, help_pad_height, help_pad_width);

	return (pnoutrefresh(ctx->help_pad, ctx->help_pos_y, ctx->help_pos_x,
	    1, 0, MIN(height - 1, help_pad_height - ctx->help_pos_y - 1),
	    MIN(width - 1, help_pad_width - ctx->help_pos_x - 1)));
}

static void
active_screen_switch(struct rantaiwarna_ctx *ctx)
{
	clear();
	wnoutrefresh(stdscr);
	switch (ctx->active_screen) {
	case GAME_SCREEN:
		touchwin(ctx->game_header_win);
		wnoutrefresh(ctx->game_header_win);
		touchwin(ctx->board_container_win);
		wnoutrefresh(ctx->board_container_win);
		touchwin(ctx->board_win);
		wnoutrefresh(ctx->board_win);
		ctx->input_win = ctx->board_win;
		break;
	case HIGHSCORE_SCREEN:
		touchwin(ctx->highscore_header_win);
		wnoutrefresh(ctx->highscore_header_win);
		highscore_pad_refresh(ctx);
		ctx->input_win = stdscr;
		break;
	case HELP_SCREEN:
		touchwin(ctx->help_header_win);
		wnoutrefresh(ctx->help_header_win);
		help_pad_refresh(ctx);
		ctx->input_win = stdscr;
		break;
	}
	flushinp();
}

static int
scrolled_win_handle_input(WINDOW *pad, int *pos_yp, int *pos_xp, int c)
{
	int	y = *pos_yp;
	int	x = *pos_xp;
	int	height;
	int	width;
	int	pad_height;
	int	pad_width;

	getmaxyx(stdscr, height, width);
	getmaxyx(pad, pad_height, pad_width);

	switch (c) {
	case 'k':
	case KEY_UP:
		/* scroll line up */
		y--;
		break;
	case 'l':
	case KEY_RIGHT:
		/* scroll right */
		x++;
		break;
	case 'j':
	case ' ':
	case '\n':
	case '\r':
	case KEY_ENTER:
	case KEY_DOWN:
		/* scroll line down */
		y++;
		break;
	case 'h':
	case KEY_LEFT:
		/* scroll left */
		x--;
		break;
	case 'b':
	case '\002':	/* ^B */
	case KEY_PPAGE:
		/* scroll up one screenful */
		y -= height;
		break;
	case 'f':
	case '\006':	/* ^F */
	case KEY_NPAGE:
		/* scroll down one screenful */
		y += height;
		break;
	case 'g':
	case KEY_HOME:
		/* go to the first line */
		y = 0;
		break;
	case 'G':
	case KEY_END:
		/* go to the line */
		y = MAX(pad_height - height, y);
		break;
	}

	if (y < 0) {
		y = 0;
	} else if (y + height > pad_height) {
		y = MAX(pad_height - height, 0);
	}
	if (x < 0) {
		x = 0;
	} else if (x + width > pad_width) {
		x = MAX(pad_width - width, 0);
	}

	if ((x != *pos_xp) || (y != *pos_yp)) {
		prefresh(pad, y, x, 1, 0, MIN(height - 1, pad_height - 1),
		    MIN(width - 1, pad_width - 1));
		*pos_yp = y;
		*pos_xp = x;
	}

	return (OK);
}

static int
highscore_handle_input(struct rantaiwarna_ctx *ctx, int c)
{
	switch (c) {
	case 'a':
		/* switch to game screen */
		ctx->active_screen = GAME_SCREEN;
		active_screen_switch(ctx);
		rantaiwarna_doupdate(ctx);
		return (OK);
	case 'H':
		/* switch to help screen */
		ctx->active_screen = HELP_SCREEN;
		active_screen_switch(ctx);
		rantaiwarna_doupdate(ctx);
		return (OK);
	default:
		/* delegate input handling to scrolled window handler */
		return (scrolled_win_handle_input(ctx->highscore_pad,
		    &ctx->highscore_pos_y, &ctx->highscore_pos_x, c));
	}
}

static int
help_handle_input(struct rantaiwarna_ctx *ctx, int c)
{
	switch (c) {
	case 'a':
		/* switch to game screen */
		ctx->active_screen = GAME_SCREEN;
		active_screen_switch(ctx);
		rantaiwarna_doupdate(ctx);
		return (OK);
	case 'i':
		/* switch to highscore screen */
		ctx->active_screen = HIGHSCORE_SCREEN;
		active_screen_switch(ctx);
		rantaiwarna_doupdate(ctx);
		return (OK);
	default:
		/* delegate input handling to scrolled window handler */
		return (scrolled_win_handle_input(ctx->help_pad,
		    &ctx->help_pos_y, &ctx->help_pos_x, c));
	}
}

static int
game_handle_input(struct rantaiwarna_ctx *ctx, int c)
{
	int	y = ctx->board_cursor_y;
	int	x = ctx->board_cursor_x;
	int	active_screen = ctx->active_screen;
	int	removed = 0;
#ifdef NCURSES_MOUSE_VERSION
	MEVENT	event;
	int	mx;
	int	my;
#endif /* NCURSES_MOUSE_VERSION */

	switch (c) {
	case 'k':
	case KEY_UP:
		/* move cursor up */
		y--;
		break;
	case 'l':
	case KEY_RIGHT:
		/* move cursor right */
		x++;
		break;
	case 'j':
	case KEY_DOWN:
		/* move cursor down */
		y++;
		break;
	case 'h':
	case KEY_LEFT:
		/* move cursor left */
		x--;
		break;
#ifdef NCURSES_MOUSE_VERSION
	case KEY_MOUSE:
		if (getmouse(&event) == OK) {
			mx = event.x;
			my = event.y;

			/* only handle mouse input inside the board window */
			if (wmouse_trafo(ctx->board_win, &my, &mx, FALSE)) {
				mx = (int)(floor(mx / ELEMENT_WIDTH));

				if (event.bstate & BUTTON1_RELEASED) {
					/* move cursor and remove element */
					y = my;
					x = mx;
					removed =
					    board_remove_elements(ctx->board,
					    y, x);
				} else if (event.bstate &
				    REPORT_MOUSE_POSITION) {
					/* move cursor to mouse position */
					y = my;
					x = mx;
				}
			}
		}
		break;
#endif /* NCURSES_MOUSE_VERSION */
	case KEY_ENTER:
	case '\n':
	case '\r':
	case ' ':
		/* remove current element */
		removed = board_remove_elements(ctx->board, y, x);
		break;
	case 'n':
		/* start a new game */
		ctx->board_cursor_x = ctx->board_cursor_y = 0;
		set_message(ctx, "rantaiwarna");
		if (board_generate(ctx->board) == OK) {
			score_win_update(ctx);
			message_win_update(ctx);
			board_win_update(ctx);
			rantaiwarna_doupdate(ctx);
			flash();
			return (OK);
		} else {
			return (ERR);
		}
	case 'H':
		/* switch to help screen */
		active_screen = HELP_SCREEN;
		break;
	case 'i':
		/* switch to highscore screen */
		active_screen = HIGHSCORE_SCREEN;
		break;
	}

	if (active_screen != ctx->active_screen) {
		ctx->active_screen = active_screen;
		active_screen_switch(ctx);
		rantaiwarna_doupdate(ctx);
	} else if (removed > 0) {
		score_win_update(ctx);
		board_win_update(ctx);
		if (ctx->board->status & GAME_OVER) {
			highscore_update(&ctx->highscore,
			    (int16_t)ctx->board->width,
			    (int16_t)ctx->board->height,
			    (int16_t)ctx->board->colors, ctx->board->score,
			    time(NULL));
			highscore_save(ctx->highscore, ctx->board->height,
			    ctx->board->width, ctx->board->colors);
			board_win_update_cursor(ctx, FALSE);
			ctx->board_cursor_y = ctx->board_cursor_x = 0;
			set_message(ctx, "GAME OVER");
			message_win_update(ctx);
			highscore_pad_update(ctx);
			flash();
		} else if ((y != ctx->board_cursor_y) ||
		    (x != ctx->board_cursor_x)) {
			board_win_move_cursor(ctx, y, x);
		}
		rantaiwarna_doupdate(ctx);
	} else if (((ctx->board->status & GAME_OVER) == 0) &&
	    ((y != ctx->board_cursor_y) || (x != ctx->board_cursor_x))) {
		if (board_win_move_cursor(ctx, y, x) == OK) {
			rantaiwarna_doupdate(ctx);
		}
	}

	return (OK);
}

static void
all_windows_delete(struct rantaiwarna_ctx *ctx)
{
	if (ctx == NULL) {
		return;
	}

	if (ctx->message_win != NULL) {
		delwin(ctx->message_win);
		ctx->message_win = NULL;
	}

	if (ctx->score_win != NULL) {
		delwin(ctx->score_win);
		ctx->score_win = NULL;
	}

	if (ctx->game_header_win != NULL) {
		delwin(ctx->game_header_win);
		ctx->game_header_win = NULL;
	}

	if (ctx->board_win != NULL) {
		delwin(ctx->board_win);
		ctx->board_win = NULL;
	}

	if (ctx->board_container_win != NULL) {
		delwin(ctx->board_container_win);
		ctx->board_container_win = NULL;
	}

	if (ctx->highscore_header_win != NULL) {
		delwin(ctx->highscore_header_win);
		ctx->highscore_header_win = NULL;
	}

	if (ctx->highscore_pad != NULL) {
		delwin(ctx->highscore_pad);
		ctx->highscore_pad = NULL;
	}

	if (ctx->help_header_win != NULL) {
		delwin(ctx->help_header_win);
		ctx->help_header_win = NULL;
	}

	if (ctx->help_pad != NULL) {
		delwin(ctx->help_pad);
		ctx->help_pad = NULL;
	}

}

static int
all_windows_create(struct rantaiwarna_ctx *ctx)
{
	int		width;
	int		height;
	char		score_label[] = "Score: ";
	int		score_label_len;
	int		header_min_width;
	char		highscore_title[] = "Highscore";
	int		highscore_entry_len;
	char		help_title[] = "Help";
	int		help_pad_height;
	int		help_pad_width;
	int		i;
	int		line_len;
	int		help_width;
	int		help_height;
	time_t		dummy_time = (time_t)0;

	getmaxyx(stdscr, height, width);

	/* enforce minimum width, both board and headers must be visble */
	score_label_len = (int)strlen(score_label);
	header_min_width = 20 + 1 + score_label_len + SCORE_WIN_WIDTH;
	header_min_width = MAX(header_min_width, (int)strlen(highscore_title));
	header_min_width = MAX(header_min_width, (int)strlen(help_title));
	if ((width < ctx->board->width * ELEMENT_WIDTH + 2) ||
	    (height < 1 + ctx->board->height + 2) ||
	    (width < header_min_width)) {
		ctx->too_small = TRUE;
		ctx->input_win = stdscr;
		mvaddstr(0, 0, "terminal size is too small");
		refresh();
		return (ERR);
	} else {
		ctx->too_small = FALSE;
	}

	/* set up game screen */
	ctx->game_header_win = newwin(1, width, 0, 0);
	if (ctx->game_header_win == NULL) {
		rantaiwarna_errx(1, "could not create window");
	}
	wbkgd(ctx->game_header_win, A_UNDERLINE | A_BOLD);
	mvwaddstr(ctx->game_header_win, 0, width - (score_label_len +
	    SCORE_WIN_WIDTH), score_label);

	ctx->score_win = derwin(ctx->game_header_win, 1, SCORE_WIN_WIDTH, 0,
	    width - SCORE_WIN_WIDTH);
	if (ctx->score_win == NULL) {
		rantaiwarna_errx(1, "could not create window");
	}
	score_win_render(ctx);

	ctx->message_win = derwin(ctx->game_header_win, 1, width - (1 +
	    score_label_len + SCORE_WIN_WIDTH), 0, 0);
	if (ctx->message_win == NULL) {
		rantaiwarna_errx(1, "could not create window");
	}
	message_win_render(ctx);

	ctx->board_container_win = newwin(ctx->board->height + 2,
	    ctx->board->width * ELEMENT_WIDTH + 2, (height - 1 -
	    (ctx->board->height + 2)) / 2, (width - (ctx->board->width *
	    ELEMENT_WIDTH + 2)) / 2);
	if (ctx->board_container_win == NULL) {
		rantaiwarna_errx(1, "could not create window");
	}
	box(ctx->board_container_win, 0, 0);

	ctx->board_win = derwin(ctx->board_container_win, ctx->board->height,
	    ctx->board->width * ELEMENT_WIDTH, 1, 1);
	if (ctx->board_win == NULL) {
		rantaiwarna_errx(1, "could not create window");
	}
	ctx->input_win = ctx->board_win;
	wbkgd(ctx->board_win, A_BOLD);
	wtimeout(ctx->board_win, 100);
	keypad(ctx->board_win, TRUE);
	board_win_render(ctx);

	board_win_render_cursor(ctx, TRUE);

	/* set up highscore screen */
	ctx->highscore_header_win = newwin(1, width, 0, 0);
	if (ctx->highscore_header_win == NULL) {
		rantaiwarna_errx(1, "could not create window");
	}
	wbkgd(ctx->highscore_header_win, A_UNDERLINE | A_BOLD);
	mvwaddstr(ctx->highscore_header_win, 0, 0, highscore_title);

	if (strftime(ctx->dummy_timestr, sizeof (ctx->dummy_timestr), "%x",
	    gmtime(&dummy_time)) == 0) {
		ctx->dummy_timestr[0] = '\0';
	}
	highscore_entry_len = snprintf(NULL, 0, HIGHSCORE_FMT, 1,
	    ctx->dummy_timestr, 0);
	if (highscore_entry_len < 0) {
		rantaiwarna_err(1, NULL);
	}
	ctx->highscore_pad = newpad(10, highscore_entry_len);
	if (ctx->highscore_pad == NULL) {
		rantaiwarna_errx(1, "could not create pad");
	}
	highscore_pad_update(ctx);

	/* set up help screen */
	ctx->help_header_win = newwin(1, width, 0, 0);
	if (ctx->help_header_win == NULL) {
		rantaiwarna_errx(1, "could not create window");
	}
	wbkgd(ctx->help_header_win, A_UNDERLINE | A_BOLD);
	mvwaddstr(ctx->help_header_win, 0, 0, help_title);

	for (i = 0, help_width = 1; help_strs[i] != NULL; i++) {
		line_len = (int)strlen(help_strs[i]);
		if (line_len > help_width) {
			help_width = line_len;
		}
	}
	help_height = i;
	ctx->help_pad = newpad(help_height, help_width);
	if (ctx->help_pad == NULL) {
		rantaiwarna_errx(1, "could not create pad");
	}

	for (i = 0; help_strs[i] != NULL; i++) {
		mvwaddstr(ctx->help_pad, i, 0, help_strs[i]);
	}
	getmaxyx(ctx->help_pad, help_pad_height, help_pad_width);
	if (ctx->help_pos_y + height - 1 > help_height) {
		ctx->help_pos_y = MAX(help_height - height - 1, 0);
	}
	if (ctx->help_pos_x + width > help_width) {
		ctx->help_pos_x = MAX(help_width - width, 0);
	}

	active_screen_switch(ctx);

	return (OK);
}

static void
handle_resize(struct rantaiwarna_ctx *ctx)
{
	all_windows_delete(ctx);
	clear();
	endwin();
	refresh();
	if (all_windows_create(ctx) == OK) {
		rantaiwarna_doupdate(ctx);
	}
}

static int
main_loop(struct rantaiwarna_ctx *ctx)
{
	sigset_t	sigset;
	sigset_t	old_sigset;
	ssize_t		n;
	int		saved_errno;
	int		c;

	while (TRUE) {
		/*
		 * deal with pending signals previously received in the signal
		 * handler, try to read a sigset from the pipe, avoid partial
		 * reads by blocking all signals during the read operation
		 */
		sigfillset(&sigset);
		sigprocmask(SIG_BLOCK, &sigset, &old_sigset);
		n = read(signal_pipe_fd[PIPE_R_FD], &sigset, sizeof (sigset));
		saved_errno = errno;
		sigprocmask(SIG_SETMASK, &old_sigset, NULL);
		if (n == -1) {
			if (saved_errno != EAGAIN) {
				/* unknown error */
				return (ERR);
			}
		} else if ((size_t)n != sizeof (sigset)) {
			/* short read, should not happen */
			return (ERR);
		} else {
			if ((sigismember(&sigset, SIGINT) == 1) ||
			    (sigismember(&sigset, SIGTERM) == 1) ||
			    (sigismember(&sigset, SIGQUIT) == 1) ||
			    (sigismember(&sigset, SIGHUP) == 1)) {
				return (ERR);
			}
#ifdef SIGWINCH
			if (sigismember(&sigset, SIGWINCH) == 1) {
				/* handle terminal resize */
				handle_resize(ctx);
			}
#endif /* SIGWINCH */
		}

		/* handle keyboard and mouse input */
		c = wgetch(ctx->input_win);
		/* only allow to quit if the terminal is too small */
		if (ctx->too_small) {
			if (c == 'q') {
				return (OK);
			} else {
				continue;
			}
		} else {
		/* handle global keys */
			switch (c) {
			case ERR:
				/* no input */
				continue;
			case 'q':
				/* quit */
				return (OK);
			case '\f':
				/* refresh */
				handle_resize(ctx);
				continue;
			default:
				/*
				 * delegate input handling to screen-specific
				 * handler
				 */
				switch (ctx->active_screen) {
				case GAME_SCREEN:
					if (game_handle_input(ctx, c) == ERR) {
						return (ERR);
					}
					break;
				case HIGHSCORE_SCREEN:
					if (highscore_handle_input(ctx, c) ==
					    ERR) {
						return (ERR);
					}
					break;
				case HELP_SCREEN:
					if (help_handle_input(ctx, c) == ERR) {
						return (ERR);
					}
					break;
				}
			}
		}
	}
}

int
main(int argc, char *argv[])
{
	int	status = EXIT_FAILURE;
	struct rantaiwarna_ctx *ctx = NULL;
	int	c;
	bool	errflag = FALSE;
	char	*endptr;
	long	colors = DEFAULT_COLORS;
	long	height = DEFAULT_HEIGHT;
	long	width = DEFAULT_WIDTH;
	int	flags;
	struct sigaction sigact;

	setlocale(LC_ALL, "");

	if ((progname = strrchr(argv[0], '/')) == NULL) {
		progname = argv[0];
	} else {
		progname++;
	}

	while (!errflag && (c = getopt(argc, argv, "c:h:w:")) != -1) {
		switch (c) {
		case 'c':
			errno = 0;
			colors = strtol(optarg, &endptr, 10);
			if ((errno != 0) || (*endptr != '\0') || (colors < 2) ||
			    (colors > COLORS_MAX - 1)) {
				errflag = TRUE;
			}
			break;
		case 'h':
			errno = 0;
			height = strtol(optarg, &endptr, 10);
			if ((errno != 0) || (*endptr != '\0') ||
			    (height < HEIGHT_MIN) || (height > HEIGHT_MAX)) {
				errflag = TRUE;
			}
			break;
		case 'w':
			errno = 0;
			width = strtol(optarg, &endptr, 10);
			if ((errno != 0) || (*endptr != '\0') ||
			    (width < WIDTH_MIN) || (width > WIDTH_MAX)) {
				errflag = TRUE;
			}
			break;
		default:
			errflag = TRUE;
		}
	}
	if (errflag) {
		fprintf(stderr, "usage: %s [-c colors] [-w width] "
		    "[-h height]\n", progname);
		status = EXIT_USAGE;
		goto out;
	}

	ctx = rantaiwarna_malloc(sizeof (struct rantaiwarna_ctx));
	ctx->board = board_create((int)height, (int)width, (short)colors);
	if (ctx->board->elements == NULL) {
		goto out;
	}
	ctx->active_screen = GAME_SCREEN;
	ctx->highscore = NULL;
	ctx->use_colors = FALSE;
	ctx->too_small = FALSE;
	ctx->game_header_win = NULL;
	ctx->message_win = NULL;
	ctx->score_win = NULL;
	ctx->board_container_win = NULL;
	ctx->board_win = NULL;
	ctx->highscore_pad = NULL;
	ctx->highscore_header_win = NULL;
	ctx->help_pad = NULL;
	ctx->help_header_win = NULL;
	ctx->input_win = stdscr;
	ctx->board_cursor_x = ctx->board_cursor_y = 0;
	ctx->highscore_pos_x = ctx->highscore_pos_y = 0;
	ctx->help_pos_x = ctx->help_pos_y = 0;
	ctx->message[0] = '\0';
	ctx->dummy_timestr[0] = '\0';
	ctx->elements_chs = elements_chs;

	highscore_load(&ctx->highscore, ctx->board->height, ctx->board->width,
	    ctx->board->colors);

	/* create pipe for delivering signals to a listener in the main loop */
	if (pipe(signal_pipe_fd) == -1) {
		goto out;
	}
	flags = fcntl(signal_pipe_fd[PIPE_R_FD], F_GETFL, 0);
	if ((flags == -1) || (fcntl(signal_pipe_fd[PIPE_R_FD], F_SETFL,
	    flags | O_NONBLOCK) == -1)) {
		goto out;
	}
	flags = fcntl(signal_pipe_fd[PIPE_W_FD], F_GETFL, 0);
	if ((flags == -1) || (fcntl(signal_pipe_fd[PIPE_W_FD], F_SETFL,
	    flags | O_NONBLOCK) == -1)) {
		goto out;
	}

	/* set up signal handler */
	sigact.sa_handler = on_signal;
	sigact.sa_flags = SA_RESTART;
	sigemptyset(&sigact.sa_mask);
	if ((sigaction(SIGINT, &sigact, NULL) < 0) ||
	    (sigaction(SIGTERM, &sigact, NULL) < 0) ||
	    (sigaction(SIGQUIT, &sigact, NULL) < 0) ||
	    (sigaction(SIGHUP, &sigact, NULL) < 0)) {
		goto out;
	}
#ifdef SIGWINCH
	if (sigaction(SIGWINCH, &sigact, NULL) < 0) {
		goto out;
	}
#endif /* SIGWINCH */

	/* initialize curses */
	initscr();
	curses_init = TRUE;
	cbreak();
	noecho();
	intrflush(stdscr, FALSE);
	nonl();
	curs_set(0);
	timeout(100);
	keypad(stdscr, TRUE);
#ifdef NCURSES_MOUSE_VERSION
	mousemask(REPORT_MOUSE_POSITION | ALL_MOUSE_EVENTS, NULL);
	mouseinterval(0);
#endif /* NCURSES_MOUSE_VERSION */
	if (has_colors()) {
		start_color();
		if ((COLORS >= 8) && (COLOR_PAIRS >= 12)) {
			ctx->use_colors = TRUE;
			/* colors 0 -- COLORS_MAX correspond to elements */
			init_pair(1, COLOR_RED, COLOR_RED);
			init_pair(2, COLOR_GREEN, COLOR_GREEN);
			init_pair(3, COLOR_YELLOW, COLOR_YELLOW);
			init_pair(4, COLOR_BLUE, COLOR_BLUE);
			init_pair(5, COLOR_MAGENTA, COLOR_MAGENTA);
			/*
			 * colors COLORS_MAX -- (COLORS_MAX + COLORS_MAX)
			 * are used for the cursor to highlight elements
			 */
			init_pair(6, COLOR_WHITE, COLOR_BLACK);
			init_pair(7, COLOR_WHITE, COLOR_RED);
			init_pair(8, COLOR_WHITE, COLOR_GREEN);
			init_pair(9, COLOR_WHITE, COLOR_YELLOW);
			init_pair(10, COLOR_WHITE, COLOR_BLUE);
			init_pair(11, COLOR_WHITE, COLOR_MAGENTA);
		}
	}

	set_message(ctx, "rantaiwarna");
	if (board_generate(ctx->board) == ERR) {
		goto out;
	}

	if (all_windows_create(ctx) == ERR) {
		goto out;
	}
	rantaiwarna_doupdate(ctx);

	if (main_loop(ctx) == OK) {
		status = EXIT_SUCCESS;
	}

out:
	restore_term();
	all_windows_delete(ctx);

	if (signal_pipe_fd[PIPE_R_FD] != -1) {
		close(signal_pipe_fd[PIPE_R_FD]);
	}
	if (signal_pipe_fd[PIPE_W_FD] != -1) {
		close(signal_pipe_fd[PIPE_W_FD]);
	}

	if (ctx != NULL) {
		board_free(ctx->board);
		highscore_free(ctx->highscore);
	}
	free(ctx);

	exit(status);
}
