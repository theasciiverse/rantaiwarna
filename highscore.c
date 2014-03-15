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
#include <pwd.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <curses.h>	/* for ERR, OK */

#include "highscore.h"
#include "compat.h"
#include "util.h"

#define	HIGHSCORE_FILENAME	".rantaiwarna_hiscore"
#define	HIGHSCORE_TMP_TMPL	".rantaiwarna_hiscoreXXXXXX"

static struct highscore_entry *
highscore_entry_new(int16_t width, int16_t height, int16_t colors,
    int32_t score, time_t time)
{
	struct highscore_entry *entry;

	entry = rantaiwarna_malloc(sizeof (struct highscore_entry));
	entry->next = NULL;
	entry->width = width;
	entry->height = height;
	entry->colors = colors;
	entry->score = score;
	localtime_r(&time, &entry->time);

	return (entry);
}

void
highscore_entry_free(struct highscore_entry *entry)
{
	free(entry);
}

void
highscore_free(struct highscore_entry *highscore)
{
	struct highscore_entry *next_entry = highscore;
	struct highscore_entry *entry = NULL;

	while (next_entry != NULL) {
		entry = next_entry;
		next_entry = entry->next;
		highscore_entry_free(entry);
	}
}

void
highscore_update(struct highscore_entry **highscorep, int16_t width,
    int16_t height, int16_t colors, int32_t score, time_t time)
{
	struct highscore_entry *new_entry;
	struct highscore_entry *entry = *highscorep;
	int	n = 0;

	if ((entry == NULL) || (entry->score < score)) {
		/*
		 * if there are no entries or the current score is greater than
		 * the first entry, prepend a new entry
		 */
		new_entry = highscore_entry_new(width, height, colors, score,
		    time);
		new_entry->next = entry;
		*highscorep = new_entry;
		n++;
	} else {
		/*
		 * if the current score is higher than the score of one of the
		 * top ten entries, insert a new entry
		 */
		for (n = 0; (entry->next != NULL) && (entry->next->score >
		    score) && (n < 10); entry = entry->next, n++);
		new_entry = highscore_entry_new(width, height, colors, score,
		    time);
		new_entry->next = entry->next;
		entry->next = new_entry;
	}
	if (entry != NULL) {
		/* trim any entries outside the top ten */
		while ((entry->next != NULL) && (n < 10)) {
			entry = entry->next;
			n++;
		}
		highscore_free(entry->next);
		entry->next = NULL;
	}
}

int
highscore_load(struct highscore_entry **highscorep, int16_t ref_height,
    int16_t ref_width, int16_t ref_colors)
{
	int		error = ERR;
	struct highscore_entry *highscore = NULL;
	char		*home_dir;
	struct passwd	*pw;
	int		n;
	char		*highscore_filename = NULL;
	FILE		*fp = NULL;
	char		*scan_fmt = NULL;
	int16_t		width;
	int16_t		height;
	int16_t		colors;
	int32_t		score;
	char		timestr[4 + 1 + 2 + 1 + 2 + 1];	/* holds %Y-%m-%d */
	struct tm	tm;
	time_t		time;

	home_dir = getenv("HOME");
	if (home_dir == NULL) {
		pw = getpwuid(getuid());
		if (pw != NULL) {
			home_dir = pw->pw_dir;
		} else {
			goto out;
		}
	}

	n = snprintf(NULL, 0, "%s/%s", home_dir, HIGHSCORE_FILENAME);
	if (n < 0) {
		rantaiwarna_err(1, NULL);
	}
	highscore_filename = rantaiwarna_malloc((size_t)n + 1);
	if (snprintf(highscore_filename, (size_t)n + 1,
	    "%s/%s", home_dir, HIGHSCORE_FILENAME) != n) {
		rantaiwarna_err(1, NULL);
	}

	n = snprintf(NULL, 0, "%%" SCNd16 "%%" SCNd16 "%%" SCNd16 "%%" SCNd32
	    "%%%lus", sizeof (timestr) - 1);
	if (n < 0) {
		rantaiwarna_err(1, NULL);
	}
	scan_fmt = rantaiwarna_malloc((size_t)n + 1);
	if (snprintf(scan_fmt, (size_t)n + 1, "%%" SCNd16 "%%" SCNd16 "%%"
	    SCNd16 "%%" SCNd32 "%%%lus", sizeof (timestr) - 1) != n) {
		rantaiwarna_err(1, NULL);
	}

	fp = fopen(highscore_filename, "r");
	if (fp == NULL) {
		goto out;
	}
	while ((n = fscanf(fp, scan_fmt, &width, &height, &colors, &score,
	    timestr)) != EOF) {
		if ((n == 5) && ((width == ref_width) &&
		    (height == ref_height) && (colors == ref_colors))) {
			memset(&tm, 0, sizeof (struct tm));
			time = (time_t)0;
			if (strptime(timestr, "%Y-%m-%d", &tm) != NULL) {
				time = mktime(&tm);
				if (time == (time_t)-1) {
					time = (time_t)0;
				}
			}
			highscore_update(&highscore, width, height, colors,
			    score, time);
		}
	}
	if (ferror(fp) != 0) {
		goto out;
	}

	error = OK;

out:
	if (fp != NULL) {
		fclose(fp);
	}

	free(scan_fmt);
	free(highscore_filename);

	if (error == ERR) {
		highscore_free(highscore);
	} else {
		highscore_free(*highscorep);
		*highscorep = highscore;
	}

	return (error);
}

int
highscore_save(struct highscore_entry *highscore, int16_t ref_height,
    int16_t ref_width, int16_t ref_colors)
{
	int		error = ERR;
	char		*scan_fmt = NULL;
	char		*home_dir;
	struct passwd	*pw;
	int		n;
	char		*highscore_filename = NULL;
	char		*tmp_filename = NULL;
	mode_t		old_mode;
	FILE		*fp_in = NULL;
	int		fd_out = -1;
	FILE		*fp_out = NULL;
	int16_t		width;
	int16_t		height;
	int16_t		colors;
	int32_t		score;
	char		timestr[4 + 1 + 2 + 1 + 2 + 1]; /* holds %Y-%m-%d */
	struct highscore_entry *entry;

	/*
	 * file format:
	 * entries consisting of five fields, entries and fields separated by
	 * whitespace
	 *
	 * fields:
	 * width (int16_t)
	 * height (int16_t)
	 * colors (int16_t)
	 * score (int32_t)
	 * time string (YYYY-MM-DD)
	 */
	if (highscore == NULL) {
		goto out;
	}

	n = snprintf(NULL, 0, "%%" SCNd16 "%%" SCNd16 "%%" SCNd16 "%%" SCNd32
	    "%%%lus", sizeof (timestr) - 1);
	if (n < 0) {
		rantaiwarna_err(1, NULL);
	}
	scan_fmt = rantaiwarna_malloc((size_t)n + 1);
	if (snprintf(scan_fmt, (size_t)n + 1, "%%" SCNd16 "%%" SCNd16 "%%"
	    SCNd16 "%%" SCNd32 "%%%lus", sizeof (timestr) - 1) != n) {
		rantaiwarna_err(1, NULL);
	}

	home_dir = getenv("HOME");
	if (home_dir == NULL) {
		pw = getpwuid(getuid());
		if (pw != NULL) {
			home_dir = pw->pw_dir;
		} else {
			goto out;
		}
	}

	n = snprintf(NULL, 0, "%s/%s", home_dir, HIGHSCORE_FILENAME);
	if (n < 0) {
		rantaiwarna_err(1, NULL);
	}
	highscore_filename = rantaiwarna_malloc((size_t)n + 1);
	if (snprintf(highscore_filename, (size_t)n + 1, "%s/%s", home_dir,
	    HIGHSCORE_FILENAME) != n) {
		rantaiwarna_err(1, NULL);
	}

	n = snprintf(NULL, 0, "%s/%s", home_dir, HIGHSCORE_TMP_TMPL);
	if (n < 0) {
		rantaiwarna_err(1, NULL);
	}
	tmp_filename = rantaiwarna_malloc((size_t)n + 1);
	if (snprintf(tmp_filename, (size_t)n + 1, "%s/%s", home_dir,
	    HIGHSCORE_TMP_TMPL) != n) {
		rantaiwarna_err(1, NULL);
	}

	old_mode = umask(077);
	fd_out = mkstemp(tmp_filename);
	umask(old_mode);
	if (fd_out == -1) {
		goto out;
	}

	fp_out = fdopen(fd_out, "w");
	if (fp_out == NULL) {
		goto out;
	}
	/*
	 * preserve entries which do not match the current combination of width,
	 * height, and colors
	 */
	fp_in = fopen(highscore_filename, "r");
	if (fp_in != NULL) {
		while ((n = fscanf(fp_in, scan_fmt, &width, &height, &colors,
		    &score, timestr)) != EOF) {
			if ((n == 5) && ((width != ref_width) ||
			    (height != ref_height) || (colors != ref_colors))) {
				if (fprintf(fp_out, "%" PRId16 " %" PRId16 " %"
				    PRId16 " %" PRId32 " %s\n", width, height,
				    colors, score, timestr) < 0) {
					goto out;
				}
			}
		}
	}
	/*
	 * append entries for the current combination of width, height, and
	 * colors
	 */
	for (entry = highscore; entry != NULL; entry = entry->next) {
		if (strftime(timestr, sizeof (timestr), "%Y-%m-%d",
		    &entry->time) == 0) {
			snprintf(timestr, sizeof (timestr) - 1, "1970-01-01");
		}
		n = snprintf(NULL, 0, "%" PRId16 " %" PRId16 " %" PRId16 " %"
		    PRId32 " %s\n", entry->width, entry->height, entry->colors,
		    entry->score, timestr);
		if (n < 0) {
			goto out;
		}
		if (fprintf(fp_out, "%" PRId16 " %" PRId16 " %" PRId16 " %"
		    PRId32 " %s\n", entry->width, entry->height,
		    entry->colors, entry->score, timestr) != n) {
			goto out;
		}
	}
	if (fflush(fp_out) == EOF) {
		goto out;
	}
	if (fsync(fd_out) == -1) {
		goto out;
	}

	error = OK;

out:
	if (fp_in != NULL) {
		fclose(fp_in);
	}

	if (fp_out != NULL) {
		fclose(fp_out);
	}

	if (fd_out != -1) {
		close(fd_out);
	}

	if (error == ERR) {
		if (tmp_filename != NULL) {
			unlink(tmp_filename);
		}
	} else {
		if (rename(tmp_filename, highscore_filename) == -1) {
			unlink(tmp_filename);
		}
	}

	free(highscore_filename);
	free(tmp_filename);
	free(scan_fmt);

	return (error);
}
