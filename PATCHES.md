# Changes from upstream

This is the Asciiverse packaging of
[rantaiwarna](https://code.guido-berhoerster.org/projects/rantaiwarna/) by
Guido Berhoerster, a SameGame (Chain Shot!) for curses terminals, under
the MIT licence (the licence text is in README: "Except otherwise noted,
all files are Copyright (C) 2014 Guido Berhoerster"; the project page
says the same).

## History from Mercurial

Upstream is a Mercurial repository,
https://hg.guido-berhoerster.org/projects/rantaiwarna/, with eleven
changesets (2014-03-15 to 2016-09-13). Mercurial is not installed on the
machine this was packaged on (Ubuntu has it as the `mercurial` package;
`hg-fast-export` and `git-remote-hg` are not packaged there), so the
history was rebuilt from hgweb: each changeset's `raw-rev` patch
(retrieved 2026-10-18) applied in order and committed with its own
author, date and message, plus a line naming the Mercurial changeset
(node id and revision number). The branch is `default`, as in
Mercurial; `.hgtags` is kept as it was, and Mercurial's tags are git
tags: `version-1` (rev 0, `a9a7ad180c3b`) and `version-2` (rev 4,
`aec74ae4d6e5`).

Checked against what upstream publishes:

- the tree at the last commit is identical to hgweb's archive of tip
  (`archive/tip.tar.gz`, sha256
  `024e2f60fd903efa301627e89a7b09b1c1637004d3f1908a6c5443f83c54403e`),
  apart from its `.hg_archival.txt`;
- `rantaiwarna-2.tar.gz` (sha256
  `af995031cc850849bf3f10f54590875037633d2a5ada1066a2f904618396e1f2`),
  the newest release from the downloads page, is identical to rev 6,
  "Release version 2" (2014-11-17), apart from `.hgtags`;
- `rantaiwarna-1.tar.gz` (sha256
  `3b5d4bf06c9da64d0979b49ac52bbcb5bc81042affb224886bce2dde74556bf2`)
  is identical to rev 0.

The `asciiverse` branch starts from the last changeset, rev 10
(2016-09-13), four commits after the release: a NEWS file, a better
README, and Makefile changes that move `_XOPEN_SOURCE=600` from the
sources into the build flags and pick the curses library by system. No distribution packages rantaiwarna
(repology lists none), so there are no distribution patches.

## Code

- **High scores in the XDG data folder** (`highscore.c`). The table was
  `~/.rantaiwarna_hiscore`; it is `$XDG_DATA_HOME/rantaiwarna/hiscore`
  (`~/.local/share/rantaiwarna/hiscore` when `XDG_DATA_HOME` is unset,
  empty or relative), with missing folders created and the temporary
  file for the atomic rename in the same folder. The old file is read,
  for the table and for the other board sizes' entries kept when saving,
  until the new one exists. The temporary file's descriptor was also
  closed twice (`fclose()` of its stream, then `close()`); once now.
- **Fallback date written in full.** When `strftime()` failed, the date
  written was cut to "1970-01-0" by a size one short of the buffer.
- **A terminal too small at startup waits.** After a resize the game
  shows "terminal size is too small" and waits for the terminal to grow
  (or q); started in one, it exited at once with status 1. It now waits
  the same way from the start.

## Packaging (new files)

- `asciiverse/build.sh`: the four sources in one compiler run with
  upstream's `-std=c99 -D_XOPEN_SOURCE=600`, against wide ncurses. The
  man page is DocBook (`rantaiwarna.6.xml`, needing xsltproc and the
  DocBook stylesheets) and is not built; the game's own help screen (H)
  has the same keys.
- `asciiverse/package.sh`: one archive per target with the binary, README
  (which holds the licence), NEWS and this file.
- `.github/workflows/release.yml`: static Linux in Alpine (x86_64,
  aarch64) and macOS (x86_64, arm64, with `_DARWIN_C_SOURCE` so
  `SIGWINCH` is not hidden by `_XOPEN_SOURCE`), plus SHA256SUMS, on a
  tag. No Windows build: the game is built on POSIX signals, a self-pipe
  and `getpwuid()`.
- `asciiverse/record-preview.py`: records `asciiverse/rantaiwarna.cast`; a
  driver reads the board from the screen and removes groups with a
  one-group lookahead, to the end of a game, then the high score table
  and a new game.
- `.gitignore`: the build folders and outputs.

## Tested

Linux: native (glibc, ncursesw 6.6) and static (Alpine 3.22, musl)
builds; games played to GAME OVER with the keys, the high score table
written to `~/.local/share/rantaiwarna/hiscore` with an old
`~/.rantaiwarna_hiscore` read in, and a new game; started at 40x12 it
waits with the message and plays once the terminal grows to 100x30,
shrinks back to the message, q quits with status 0; at 44x15 the whole
game fits. ASan and UBSan: three 40-second runs of random keys and
mouse clicks with random resizes, and runs at `-w 5 -h 5 -c 2` and
`-w 300 -h 200 -c 5`, no reports. macOS: not built here.
