#!/bin/sh
set -e
if test "$1" = ""; then
  cat <<EOF
This script formats an SD card for the Zed platform.
It should be run with "sudo".
The single argument is the name of the device of the SD card, e.g. /dev/sdb
***The SD card should be unmounted beforee this is run***
You can use "sudo fdisk -l" to see all devices.
You can use "mount" to see what is mounted.
EOF
  exit 1
fi
if test "$USER" != root; then
  echo This script must be run as root or under sudo.
  exit 1
fi
echo =============Unmounting all file systems for SD device: $1
eject $1
echo =============Formatting the SD device: $1
SIZE=$(parted -s $1 unit B print| grep Disk | sed 's/^.* \([0-9]*\)B$/\1/')
MB=$(expr $SIZE / \( 1024 \* 1024 \))
END=$(expr $MB - 1)
echo Size of the $1 device is: $SIZE bytes or $MB MB.
echo Leaving the first and last MB unused.
echo Start of first partition is 1MB and end is 201 MB
echo Start of second partition is 202 MB to $END MB
echo =============Creating and printing partition table for $1:
parted -s $1 mklabel msdos mkpart primary fat32 1 201 mkpart primary ext2 201 $END print
echo =============Creating the 200 MB FAT32 file system in partition 1:
mkfs.vfat -F 32 -n boot ${1}1
echo =============Creating the $(expr $END - 201) MB Linux ext2 file system in partition 2 \(takes a while - as much as 5 minutes\):
mke2fs -j -L linux ${1}2
echo =============The SD device $1 has been formatted with a new partition table and 2 partitions.
