#!/bin/sh
set -e
if test "$1" = ""; then
  cat <<EOF
This script formats an SD card for a Zynq platform.
It should be run with "sudo".
The single argument is the name of the device of the SD card, e.g. /dev/sdb or /dev/mmcblk0
The SD card will be umounted if it is mounted.
You can use "sudo fdisk -l | grep 'Disk /'" to see all disk devices.
You can use "df -h" or "mount" to see what is mounted.
EOF
  exit 1
fi
if test "$USER" != root; then
  echo This script must be run as root or under sudo.
  exit 1
fi
ME=$(basename $0)
PART=""
if [ $(echo ${1} | grep mmc) ]; then
  if [ $(echo ${1} | grep 'p[0-9]\+$') ]; then
    echo -n "You are attempting to create a partition within a partition. Run $ME again targeting "
    echo $1 | sed -e 's/p[0-9]\+$//'
    exit 1
  fi
  PART="p"
fi
echo =============Unmounting all file systems for SD device: $1
#eject $3 || true
for i in $(seq 0 20);do
   umount $1${PART}$i 2>/dev/null || true 
done
echo =============Formatting the SD device: $1
SIZE=$(parted -s $1 unit B print| grep -m1 Disk | sed 's/^.* \([0-9]*\)B$/\1/')
if [ -z "$SIZE" ]; then
  echo $ME: Error determining size for $1. Aborting.
  exit 1
fi
MB=$(expr $SIZE / \( 1024 \* 1024 \))
END=$(expr $MB - 1)
echo Size of the $1 device is: $SIZE bytes or $MB MB.
echo Leaving the first and last MB unused.
echo Start of first partition is 1MB and end is 201 MB
echo Start of second partition is 202 MB to $END MB
echo =============Creating and printing partition table for $1:
parted -s $1 mklabel msdos mkpart primary fat32 1 201 mkpart primary ext2 201 $END print
if [ $(parted -s $1 print | grep -c -m1 lba) -gt 0 ]; then
  echo =============Fixing LBA flags on partition 1:
  parted -s $1 toggle 1 lba print
fi
echo =============Creating the 200 MB FAT32 file system in partition 1:
mkfs.vfat -F 32 -v -s 2 -n boot ${1}${PART}1
echo =============Creating the $(expr $END - 201) MB Linux ext2 file system in partition 2 \(takes a while - as much as 5 minutes\):
mke2fs -j -L linux ${1}${PART}2
echo =============The SD device $1 has been formatted with a new partition table and 2 partitions.
