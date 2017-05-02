#!/bin/sh --noprofile
if test $1 = darwin && which -s sw_vers; then
  echo macos `sw_vers -productVersion | sed 's/^\([0-9][0-9]*\.[0-9][0-9]*\).*/\1/' | tr . _` $2
  exit 0
fi
exit 1
