#!/bin/bash
# Regenerate h2scan inputs: preprocessed mod_http2 sources (minus h2_session.c, which harness.c includes),
# harness and stubs, for httpd 2.4.70 and 2.4.71. With H2SCAN_PATCH_WORKERS=1, h2_workers.c is replaced by patched/h2_workers.c
# (identical in both versions; APR_RING producer queue mirrored by a pointer, see the h2scan comment there).
# usage: prep.sh <dir with httpd-2.4.70 and httpd-2.4.71 source trees> <APR include dir>
set -e
D=$1; P=$2; W=$(cd "$(dirname "$0")" && pwd)
DEFS="-DLINUX -D_REENTRANT -D_GNU_SOURCE -DH2_OPENSSL -DH2_NG2_CHANGE_PRIO -DH2_NG2_INVALID_HEADER_CB -DH2_NG2_LOCAL_WIN_SIZE -DH2_NG2_NO_CLOSED_STREAMS -DH2_NG2_RFC9113_STRICTNESS"
for v in 70 71; do H=$D/httpd-2.4.$v; M=$H/modules/http2; O=$W/i$v; rm -rf $O; mkdir -p $O
  INC="-I$M -I$H/include -I$H/os/unix -I$H/server -I$H/modules/ssl -I$P"
  for f in $(sed -n 's/^mod_http2.la: //p' $M/modules.mk | tr ' ' '\n' | sed -n 's/\.slo$/.c/p' | grep -v '^h2_session.c$'); do
    src=$M/$f; [ "$f" = h2_workers.c ] && [ "${H2SCAN_PATCH_WORKERS:-0}" = 1 ] && src=$W/patched/h2_workers.c
    gcc -E -P $DEFS $INC $src -o $O/${f%.c}.i
  done
  for f in harness stubs; do gcc -E -P $DEFS $INC $W/$f.c -o $O/$f.i; done
  echo "2.4.$v: $(ls $O | wc -l) files, $(cat $O/*.i | wc -l) lines"
done
