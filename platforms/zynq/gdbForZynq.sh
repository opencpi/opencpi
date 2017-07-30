#!/bin/bash
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

# 1. First on the target system run (literally):  gdbserver host:1234 <command> <args>
# 2. Run this on the dev system within the zynq development environment:
#     $0 <mainprogram> ip-addr-of-zed <sw-platform> <other-gdb-args>
#    The platform can be "-" if OCPI_TARGET_PLATFORM is already set in the environment
#    Otherwise use the platform explicitly
# 3. Use the "set directories" command to point to source directories
#
# If you add the args "-iex c" at the end, it basically just runs the program under gdb
# i.e. for backtraces on crashes or assertions
# E.g., to run ocpirun, on address 1.3.5.7, in the core source tree:
#  1. On the target system do:
#      % gdbserver host:1234 ocpirun -C
#  2. On the development system do:
#      $ gdbForZynq.sh runtime/application/target-linux-x13_4-arm/ocpirun 1.3.5.7 xilinx13_4 -iex c

main=$1
shift
addr=$1
shift
plat=$1
[ $plat = - ] && plat=$OCPI_TARGET_PLATFORM
shift
# assume the current target is a zynq target
source $OCPI_CDK_DIR/scripts/ocpitarget.sh $plat
exec $OCPI_CROSS_BUILD_BIN_DIR/$OCPI_CROSS_HOST-gdb \
  $main \
  -ex "set sysroot $OCPI_CDK_DIR/platforms/$plat/release/uramdisk/root" \
  -ex "set solib-search-path $OCPI_CDK_DIR/lib/$OCPI_TARGET_DIR:$OCPI_CDK_DIR/lib/components/rcc/$OCPI_TARGET_DIR" \
  -ex "target remote ${addr}:1234" \
  $*
