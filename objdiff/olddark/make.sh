#!/bin/sh
# objdiff build wrapper: objdiff passes the unit's base_path (relative to this dir).
# Strip the build-dir prefix and hand the object to ninja.
p="$1"
case "$p" in
  ../../build/vc6/*) p="${p#../../build/vc6/}" ;;
esac
exec ninja -C ../../build/vc6 "$p"
