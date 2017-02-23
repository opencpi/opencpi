#!/bin/sh
set -e
if test "$1" = ""; then
  cat <<EOF
This script fills an SD card from the directory created using makeSD.sh or createOpenCPIZynqSD.sh
It takes 3 arguments:
1.  The release name of the OpenCPI Zynq release prepared in the CWD.
    This is usually YY_Q (e.g. 13_4)
    If it is "-", then the single release directory in this CWD will be used.
2.  The HDL platform name to target for this SD card.
3.  The SD linux device (like dev/sdX).
This script will copy contents from the release specified by the first argument to the first
partition on the device identified by the second argument.  It will mount, then unmount that
partition, so that it can be immediately removed.
If you want to mount/umount yourself, e.g. on /media/boot, you can just do:
  rm -r -f /media/boot/*
  cp -r opencpi-zynq-linux-release-<rel-name>/OpenCPI-SD-<hdl-platform>/*  /media/boot
  umount /media/boot
EOF
  exit 1
fi
set -e
if test "$1" = -; then
  shopt -s nullglob
  dir=(opencpi-zynq-linux-release-*)
  shopt -u nullglob
  [ ${#dir[*]} != 1 ] && {
     echo You must specify the release since there are more than one. 1>&2
     exit 1
  }
else
  dir="opencpi-zynq-linux-release-$1"
fi
if test ! -d $dir; then
    echo Error: The OpenCPI Zynq linux release directory, $dir, does not exist.
    echo Run createOpenCPIZynqRelease.sh and createOpenCPIZynqSD.sh before this\?
  exit 1
fi
sd_dir=$dir/OpenCPI-SD-$2
if test ! -d $sd_dir; then
    echo Error: The OpenCPI Zynq linux release SD directory, $sd_dir, does not exist.
    echo Run createOpenCPIZynqSD.sh before this\?
  exit 1
fi
PART=${3}1
if df -h | grep -q $PART; then
    echo The partition $PART is already mounted.  This script requires it to be unmounted,
    echo so that you are clear that it will be erased and rewritten.
    echo Use "umount $PART" to unmount the partition if you are sure.
    exit 1
fi
if test "$USER" != root; then
  echo This script must be run as root or under sudo.
  exit 1
fi
if test ! -w $3; then
  echo The device $3 does not exist or is not writable.  Perhaps you need to run this command using sudo.
  exit 1
fi
rm -r -f tmp-mnt
mkdir tmp-mnt
mount $PART tmp-mnt
echo Removing files and directories in the boot partition of device $3.
rm -r -f tmp-mnt/*
echo Copying files from $dir to partition 1 of device $3
cp -R --preserve=mode,timestamps $sd_dir/* tmp-mnt
df $PART
echo "Unmounting the partition $PART (flushing the system's buffers to the device)."
umount tmp-mnt
echo The partition on $PART that was filled is now umounted.
rmdir tmp-mnt

