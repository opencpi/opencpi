#!/bin/bash
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
  -iex "set sysroot $OCPI_CDK_DIR/platforms/$plat/release/uramdisk/root" \
  -iex "set solib-search-path  $OCPI_CDK_DIR/platforms/$plat:$OCPI_CDK_DIR/lib/$OCPI_TARGET_DIR:$OCPI_CDK_DIR/lib/components/rcc/$OCPI_TARGET_DIR" \
  -iex "target remote ${addr}:1234" \
  $*
