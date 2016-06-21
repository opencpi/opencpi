# Setup to use the EDK.  This algorithm should be consistent with xilinx.mk in the cdk
# Puts the EDK on standout
function XilinxBad {
  echo Problem initializing XILINX environment: $* 1>&2
  exit 1
}
[ -z "$OCPI_XILINX_EDK_DIR" ] && {
  OCPI_XILINX_TOOLS_DIR=$($OCPI_CDK_DIR/scripts/xilinx-ise.sh) || exit 1
  OCPI_XILINX_EDK_DIR=$OCPI_XILINX_TOOLS_DIR/EDK
}
[ ! -d $OCPI_XILINX_EDK_DIR ] && XilinxBad no EDK directory under $OCPI_XILINX_TOOLS_DIR
# Note that this script is sourced, so this variable is set in the caller,
# but if the caller does not source it, the stdout is the variable value.
echo $OCPI_XILINX_EDK_DIR
