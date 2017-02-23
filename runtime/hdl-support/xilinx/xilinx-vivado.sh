# Setup to use Vivado.  Output the tools directory on stdout.
# On errors put errors on stderr and exit 1

function XilinxBad {
  echo Problem initializing XILINX environment: $* 1>&2
  exit 1
}
[ -z "$OCPI_XILINX_VIVADO_TOOLS_DIR" ] && {
  [ -z "$OCPI_XILINX_VIVADO_DIR" ] && {
    [ -z "$OCPI_XILINX_DIR" ] && OCPI_XILINX_DIR=/opt/Xilinx
    [ ! -d "$OCPI_XILINX_DIR" ] && XilinxBad OCPI_XILINX_DIR value $OCPI_XILINX_DIR not found
    OCPI_XILINX_VIVADO_DIR=$OCPI_XILINX_DIR/Vivado
    [ ! -d "$OCPI_XILINX_VIVADO_DIR" ] &&
      XilinxBad OCPI_XILINX_VIVADO_DIR value $OCPI_XILINX_VIVADO_DIR not found
  }
  [ -z "$OCPI_XILINX_VIVADO_VERSION" ] && {
    for i in $(shopt -s nullglob && echo $OCPI_XILINX_VIVADO_DIR/*  | tr ' ' '\n' | sort -r); do
      [ -d $i -a -f $i/settings64.sh ] && OCPI_XILINX_VIVADO_VERSION=$(basename $i) && break
    done
    [ -z "$OCPI_XILINX_VIVADO_VERSION" ] &&
      XilinxBad No Xilinx Vivado tool versions found in $OCPI_XILINX_VIVADO_DIR
  }
  OCPI_XILINX_VIVADO_TOOLS_DIR=$OCPI_XILINX_VIVADO_DIR/$OCPI_XILINX_VIVADO_VERSION
}      
[ ! -d $OCPI_XILINX_VIVADO_TOOLS_DIR ] &&
  XilinxBad No XILINX Vivado directory at $OCPI_XILINX_VIVADO_TOOLS_DIR
echo $OCPI_XILINX_VIVADO_TOOLS_DIR
