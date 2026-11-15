#!/bin/sh
p="$1"
case "$p" in
  ../../build/vc6_dromed/*) p="${p#../../build/vc6_dromed/}" ;;
esac
exec ninja -C ../../build/vc6_dromed "$p"
