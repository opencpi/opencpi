#!/bin/csh -f
set l=$argv[$#argv]
$OCPI_OMNI_BIN_DIR/omniidl -bcxx -Wbh=.h -Wbd=_c.cxx -Wbs=_c.cxx -Wba -I$OCPI_OMNI_IDL_DIR $*
cp $l:r.h $l:r_s.h
