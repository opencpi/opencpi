# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

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
