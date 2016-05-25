# #### Location of the Xilinx tools ####################################### #

# This is all disabled since it is all taking place in the xilinx.mk script or whereever
# else it is needed - no global setup.
# This might be enhanced for early error checking - ideally checking licenses?


if test x = y; then
if test "$OCPI_XILINX_DIR" = ""; then
  export OCPI_XILINX_DIR=/opt/Xilinx
fi
if test "$OCPI_XILINX_VERSION" = ""; then
 export OCPI_XILINX_VERSION=14.7
fi
if test "$OCPI_XILINX_LICENSE_FILE" = ""; then
 export OCPI_XILINX_LICENSE_FILE=$OCPI_XILINX_DIR/Xilinx-License.lic
fi
if test "$OCPI_XILINX_TOOLS_DIR" = ""; then
export OCPI_XILINX_TOOLS_DIR=$OCPI_XILINX_DIR/$OCPI_XILINX_VERSION/ISE_DS
fi
if ! test -d $OCPI_XILINX_TOOLS_DIR; then
  export OCPI_XILINX_TOOLS_DIR=$OCPI_XILINX_DIR/$OCPI_XILINX_VERSION/LabTools
fi
export OCPI_XILINX_EDK_DIR=$OCPI_XILINX_DIR/$OCPI_XILINX_VERSION/ISE_DS/EDK
export OCPI_VIVADO_TOOLS_DIR=$OCPI_XILINX_DIR/Vivado/$OCPI_VIVADO_VERSION
fi
