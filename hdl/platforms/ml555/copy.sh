#!/bin/sh
# Populate from drop (in addition to creating platform.mk by hand)
DROP=$OCPI_HDL_IMPORTS_DIR
# Source files that are specific to this platform
cp $DROP/rtl/mkFTop_ml555.v .
cp $DROP/libsrc/hdl/ocpi/fpgaTop_ml555.v .
cp $DROP/ucf/ml555.ucf .
cp $DROP/ucf/ml555.xcf .

