#!/bin/sh
# Populate from drop (in addition to creating platform.mk by hand)
DROP=$OCPI_HDL_IMPORTS_DIR
# Source files that are specific to this platform
cp $DROP/libsrc/hdl/ocpi/mkFTopV6.v .
cp $DROP/libsrc/hdl/ocpi/fpgaTop_v6.v .
cp $DROP/ucf/ml605.ucf .
cp $DROP/ucf/ml605.xcf .

