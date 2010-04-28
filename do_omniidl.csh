#!/bin/csh
set echo
set verbose
set l=$argv[$#]
/usr/local/bin/omniidl -bcxx -Wbh=.h -Wbd=.cxx -Wbs=.cxx -Wba -I$OMNI_IDL_DIR $*
cp $l:r.h $l:r_s.h
