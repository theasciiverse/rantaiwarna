#!/bin/sh
# Builds rantaiwarna into out/: the four sources in one compiler run (one
# process, so no parallel jobs to limit), with the flags upstream's
# Makefile uses (-std=c99 -D_XOPEN_SOURCE=600). The compiler, flags and
# curses library come from the environment, so one script serves every
# target (see .github/workflows/release.yml):
#
#   asciiverse/build.sh                                        # Linux
#   LDFLAGS="-static -s" asciiverse/build.sh                   # Alpine, static
#   LIBS=-lncurses CFLAGS="-O2 -arch arm64 -D_DARWIN_C_SOURCE" \
#     LDFLAGS="-arch arm64" asciiverse/build.sh                # macOS
#
# The man page is DocBook (rantaiwarna.6.xml) and needs xsltproc and the
# DocBook stylesheets to build, so it is not built here.
set -eu
cd "$(dirname "$0")/.."
CC="${CC:-cc}"
CFLAGS="${CFLAGS:--O2}"
LDFLAGS="${LDFLAGS:-}"
LIBS="${LIBS:--lncursesw}"
mkdir -p out
rm -f out/rantaiwarna
# shellcheck disable=SC2086
nice -n 10 $CC -std=c99 -D_XOPEN_SOURCE=600 $CFLAGS -o out/rantaiwarna \
  rantaiwarna.c board.c highscore.c util.c $LDFLAGS $LIBS -lm
echo "built rantaiwarna"
