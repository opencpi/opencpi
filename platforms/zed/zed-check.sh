#!/bin/sh --noprofile
if test $1 = linux -a $2 = armv7l -a -d /sys/class/xdevcfg; then
  echo $1 zynq arm
  exit 0
fi
exit 1


