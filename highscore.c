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

#define	HIGHSCORE_DIRNAME	"rantaiwarna"
#define	HIGHSCORE_FILENAME	"hiscore"
#define	HIGHSCORE_TMP_TMPL	"hiscoreXXXXXX"
#define	LEGACY_FILENAME		".rantaiwarna_hiscore"

/*
 * Builds dir + "/" + name in newly allocated memory.
 */
static char *
path_join(const char *dir, const char *name)
{
	int	n;
	char	*path;

	n = snprintf(NULL, 0, "%s/%s", dir, name);
	if (n < 0) {
		rantaiwarna_err(1, NULL);
	}
	path = rantaiwarna_malloc((size_t)n + 1);
	if (snprintf(path, (size_t)n + 1, "%s/%s", dir, name) != n) {
		rantaiwarna_err(1, NULL);
	}

	return (path);
}

/*
 * The folder holding the highscore file, $XDG_DATA_HOME/rantaiwarna or
 * ~/.local/share/rantaiwarna, created with any missing parents, and the
 * file the game used before, ~/.rantaiwarna_hiscore, which is read while
 * the new file does not exist yet.  Returns ERR when there is no home
 * folder.
 */
static int
highscore_dirs(char **dirp, char **legacyp)
{
	char		*home_dir;
	char		*xdg_dir;
	struct passwd	*pw;
	char		*dir;
	char		*p;

	home_dir = getenv("HOME");
	if ((home_dir == NULL) || (home_dir[0] == '\0')) {
		pw = getpwuid(getuid());
		if (pw == NULL) {
			return (ERR);
		}
		home_dir = pw->pw_dir;
	}

	/* an empty or relative XDG_DATA_HOME counts as unset (XDG spec) */
	xdg_dir = getenv("XDG_DATA_HOME");
	if ((xdg_dir != NULL) && (xdg_dir[0] == '/')) {
		dir = path_join(xdg_dir, HIGHSCORE_DIRNAME);
	} else {
		p = path_join(home_dir, ".local/share");
		dir = path_join(p, HIGHSCORE_DIRNAME);
		free(p);
	}
	for (p = dir + 1; *p != '\0'; p++) {
		if (*p == '/') {
			*p = '\0';
			mkdir(dir, 0755);
			*p = '/';
		}
	}
	mkdir(dir, 0755);

	*dirp = dir;
	*legacyp = path_join(home_dir, LEGACY_FILENAME);

	return (OK);
}

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
	char		*highscore_dir = NULL;
	char		*legacy_filename = NULL;
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

	if (highscore_dirs(&highscore_dir, &legacy_filename) == ERR) {
		goto out;
	}
	highscore_filename = path_join(highscore_dir, HIGHSCORE_FILENAME);

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
		fp = fopen(legacy_filename, "r");
	}
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
	free(highscore_dir);
	free(legacy_filename);

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
	char		*highscore_dir = NULL;
	char		*legacy_filename = NULL;
	int		n;
	char		*highscore_filename = NULL;
	char		*tmp_filename = NULL;
	mode_t		old_mode;
	FILE		*fp_in = NULL;
	int		fd_out = -1;
	int		fd_sync = -1;
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

	if (highscore_dirs(&highscore_dir, &legacy_filename) == ERR) {
		goto out;
	}
	highscore_filename = path_join(highscore_dir, HIGHSCORE_FILENAME);
	tmp_filename = path_join(highscore_dir, HIGHSCORE_TMP_TMPL);

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
	fd_sync = fd_out;
	fd_out = -1;	/* closed by fclose(fp_out) from here on */
	/*
	 * preserve entries which do not match the current combination of width,
	 * height, and colors
	 */
	fp_in = fopen(highscore_filename, "r");
	if (fp_in == NULL) {
		fp_in = fopen(legacy_filename, "r");
	}
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
	if (fsync(fd_sync) == -1) {
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
	free(highscore_dir);
	free(legacy_filename);

	return (error);
}
