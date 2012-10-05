#!/bin/bash --noprofile
. setup_install.sh
if test ! -d loc ; then
  mkdir -p loc
fi
cd loc
curl -O http://headerfile-free-cyclomatic-complexity-analyzer.googlecode.com/files/hfcca14.py









