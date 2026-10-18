#!/bin/sh
# Packs the build into dist/rantaiwarna-<target>.tar.gz for av: the
# binary at the top, with README, NEWS and PATCHES.md (README holds the
# MIT licence).
#
#   asciiverse/package.sh x86_64-unknown-linux-musl
set -eu
cd "$(dirname "$0")/.."
target="$1"
stage=stage/rantaiwarna
rm -rf "$stage" && mkdir -p "$stage" dist
cp out/rantaiwarna README NEWS PATCHES.md "$stage/"
tar -czf "dist/rantaiwarna-$target.tar.gz" -C "$stage" .
ls dist
