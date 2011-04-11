#!/bin/sh
# Populate from drop (in addition to creating platform.mk by hand)
DROP=$OCPI_HDL_IMPORTS_DIR
# Source files that are specific to this platform
cp $DROP/rtl/mkFTop_schist.v .
cp $DROP/libsrc/hdl/ocpi/fpgaTop_schist.v .
cp $DROP/ucf/schist.ucf .
cp $DROP/ucf/schist.xcf .

