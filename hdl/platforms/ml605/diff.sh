#!/bin/sh
# Populate from drop (in addition to creating platform.mk by hand)
DROP=$OCPI_HDL_IMPORTS_DIR
# Source files that are specific to this platform
for f in $DROP/rtl/mkFTop_ml605.v $DROP/libsrc/hdl/ocpi/fpgaTop_ml605.v \
         $DROP/ucf/ml605.ucf $DROP/ucf/ml605.xcf
do
echo =======diff $f
diff $f .
done


