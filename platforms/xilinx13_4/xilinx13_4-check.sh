#!/bin/sh --noprofile
if test "$1" = linux -a "$2" = armv7l -a -d /sys/class/xdevcfg; then
  # this "zynq" without further versioning is temporary until we add new os versions...
  dir=/mnt/card/opencpi/lib/*
  [ -d $dir ] && {
    osv=$(echo $(basename $dir) | sed 's/.*-\(.*\)-.*$/\1/')
    echo $1 $osv arm
    exit 0
  }
fi
exit 1


