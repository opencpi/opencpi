#!/bin/sh
set -e
if test "$1" = ""; then
  cat <<EOF
This script fills an SD card from the directory created using makeSD.sh
It takes 2 arguments:
1.  The SD directory prepared using the makeSD.sh script.
    This is usually release-<release-name>/OpenCPI-SD
    If you just say "-", it defaults to release/OpenCPI-SD
2.  The SD linux device (like dev/sdX).
This script will copy the directory identified by the first argument to the first partition
on the device identified by the second argument.  It will mount, then unmount that partition.
If you want to mount/umount yourself, e.g. on /media/boot, you can just do:
  rm -r -f /media/boot/*
  cp -r release/OpenCPI-SD/*  /media/boot
  umount /media/boot
EOF
  exit 1
fi
set -e
if test "$1" = -; then
  dir=release/OpenCPI-SD
else
  dir="$1"
fi
if test ! -d $dir; then
  echo Error: The SD directory, $dir, does not exist. Run makeSD.sh before this\?
  exit 1
fi
if test ! -w $2; then
  echo The device $2 is not writable.  Perhaps you need to run this command using sudo.
  exit 1
fi
if test -e tmp-mnt; then
  echo There is a left over tmp-mnt directory here.  Remove/rename it and run this again.
  exit 1
fi
mkdir tmp-mnt
mount ${2}1 tmp-mnt
echo Copying files from $dir to partition 1 of device $2
cp -R --preserve=mode,timestamps $dir/* tmp-mnt
umount tmp-mnt
rmdir tmp-mnt
echo The partition that was filled is now umounted.
