#!/bin/sh
if test "$#" != 7; then
  cat <<EOF
This script does everything to process a Xilinx release and populate a directory
to create a bootable OpenCPI SD card for a Xilinx Linux Zynq platform
You must already know the the release name, the git repo tag, and the download URL.
If not, go to http://www.wiki.xilinx.com/Zynq+Releases, and also inspect a Xilinx git repo clone
to determine the correct repo tag.
This script is used to go through the entire process, but you usually need to do it
in separate steps in order to properly determine the xilinx repo tag.
Hence it is more of a test tool.

Arg 1: The our Xilinx release name. (e.g. 13_4)
Arg 2: The Xilinx repo tag (e.g. xilinx-v2013.4)
Arg 3: The download URL for the Xilinx Release
       e.g. http://www.wiki.xilinx.com/file/view/2013.4-release.tar.xz/483333010/2013.4-release.tar.xz
Arg 4: The directory to use when downloading the Xilinx git repo.

EOF
  exit 1
fi
d=$(dirname $0)/
set -e
[ -z "$4" ] && {
  echo Fourth argument must be specified.
}
rel=$1
tag=$2
url=$3
work=$4
sw=$5
hw=$6
drive=$7
# ./getXilinxLinuxSources.sh $work
$d/showXilinxLinuxTags.sh $work
OCPI_CDK_DIR=../../bootstrap $d/createLinuxKernelHeaders.sh $rel $tag $work
$d/getXilinxLinuxBinaryRelease.sh $rel $url $work
$d/createLinuxRootFS.sh $rel $work
$d/createOpenCPIZynqSD.sh $rel $hw $sw
sudo $d/formatOpenCPIZynqSD.sh $drive
sudo $d/writeOpenCPIZynqSD.sh $rel $hw $drive


