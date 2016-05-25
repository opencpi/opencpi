# Setup to use ISE.  Output the tools directory on stdout.
# On errors put errors on stderr and exit 1

function XilinxBad {
  echo Problem initializing XILINX environment: $* 1>&2
  exit 1
}
[ -z "$OCPI_XILINX_TOOLS_DIR" ] && {
  [ -z "$OCPI_XILINX_DIR" ] && OCPI_XILINX_DIR=/opt/Xilinx
    [ ! -d "$OCPI_XILINX_DIR" ] && XilinxBad OCPI_XILINX_DIR value $OCPI_XILINX_DIR not found
  [ -z "$OCPI_XILINX_VERSION" ] && {
    for i in $(shopt -s nullglob && echo $OCPI_XILINX_DIR/*  | tr ' ' '\n' | sort -r); do
      [ -d $i -a -d $i/ISE_DS ] && OCPI_XILINX_VERSION=$(basename $i) && break
    done
    [ -z "$OCPI_XILINX_VERSION" ] &&
      XilinxBad No Xilinx tool versions found in $OCPI_XILINX_DIR
  }
  OCPI_XILINX_TOOLS_DIR=$OCPI_XILINX_DIR/$OCPI_XILINX_VERSION/ISE_DS
}      
[ ! -d $OCPI_XILINX_TOOLS_DIR ] &&
  XilinxBad No XILINX ISE directory at $OCPI_XILINX_TOOLS_DIR
echo $OCPI_XILINX_TOOLS_DIR
