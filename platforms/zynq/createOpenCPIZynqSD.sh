#!/bin/sh
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

set -e
if test "$1" = ""; then
  cat <<EOF
This script creates and fills a directory to be written to an SD card for OpenCPI on Zynq.
When the contents is written to an SD card, that card can be used as the boot disk on a
Zynq-based system.

It assumes:
1. You have created an OpenCPI Zynq release from Xilinx releases and kernel sources.
2. You have cross-built OpenCPI for the corresponding zynq software platform (version)
3. You have built the HDL assets for the particular zynq HDL platform.
4. You are running in the platforms/zynq directory or a specific software platform's directory.
5. You are running in the same directory where the release was created.

This script can be repeatedly run; it first clears out the directory it is filling.

This script takes three arguments to run:
1. the OpenCPI Zynq release name to use
2. the HDL platform name to use associated with the software platform you are enabling.
3. the Zynq software platform to use (which defaults to the name of the CWD)

Don't forget to copy zynq/user_zynq_setup.sh to mysetup.sh and customize mysetup.sh before
doing this.  Also, copy zynq/user_zynq_net_setup.sh to mynetsetup.sh and customize
mynetsetup.sh before doing this.

Also, artifacts for the SD card are found based on the OCPI_LIBRARY_PATH environment variable,
so make sure it includes the desired artifacts for running on the system you are targeting.

The result will be placed in a subdirectory of the release directory, called OpenCPI-SD, e.g.
opencpi-zynq-linux-release-<rel-name>/OpenCPI-SD.  You can further customize it if needed.
EOF
  exit 1
fi
REL=opencpi-zynq-linux-release-$1
if test ! -d $REL; then
  echo The release directory, $REL, doesn\'t exist.
  exit 1
fi

export OCPI_TARGET_PLATFORM=${3:-$(basename $(pwd))}
# This one might be overridden if we want an SD from a particular build mode
# Someday provide the option to select the build mode
export OCPI_TARGET_DIR=$OCPI_TARGET_PLATFORM
export HDL_PLATFORM=${2:-zed}
if [ -z "$OCPI_PROJECT_REGISTRY_DIR" ]; then
  OCPI_PROJECT_REGISTRY_DIR=$OCPI_CDK_DIR/../project-registry
fi
source $OCPI_CDK_DIR/scripts/util.sh
echo Software platform is $OCPI_TARGET_PLATFORM, and hardware platform is $HDL_PLATFORM.
if test -z $RPM_BUILD_ROOT; then
  # We assume a built tree for the tool platform - check for exports etc.?
  # ensure OCPI_CDK_DIR and OCPI_TOOL_DIR
  OCPI_BOOTSTRAP=$OCPI_CDK_DIR/scripts/ocpibootstrap.sh; source $OCPI_BOOTSTRAP
  source $OCPI_CDK_DIR/scripts/ocpitarget.sh $OCPI_TARGET_PLATFORM
  EXAMPLES_ROOTDIR=$(getProjectRegistryDir)/ocpi.assets
  if test "$OCPI_LIBRARY_PATH" = ""; then
    # Put all rcc components, and available bitstreams for the platform.
    export OCPI_LIBRARY_PATH=$(getProjectRegistryDir)/ocpi.core/exports/lib/components:$OCPI_CDK_DIR/lib/platforms/$HDL_PLATFORM
  fi
else
  echo RPM Build detected - faking directory structure
  OCPI_CDK_DIR=${RPM_BUILD_ROOT}/opt/opencpi/cdk
  # Cannot just use CDK/lib and CDK/bin because the driver stuff isn't pushed there
  # EXAMPLES_ROOTDIR set externally
  # This is using a "path" variable assuming it has no colons in it!
  export OCPI_LIBRARY_PATH=$(getProjectRegistryDir)/ocpi.core/exports/lib/components:${OCPI_HDL_PLATFORM_PATH}/${OCPI_TARGET_PLATFORM}/
fi
BIN_DIR=${OCPI_CDK_DIR}/${OCPI_TARGET_DIR}/bin
KERNEL_LIB_DIR=$OCPI_CDK_DIR/${OCPI_TARGET_DIR}/lib
RUNTIME_LIB_DIR=${OCPI_CDK_DIR}/${OCPI_TARGET_DIR}/lib

cd $REL
sd=OpenCPI-SD-$HDL_PLATFORM
rm -r -f $sd
mkdir $sd
echo Populating the SD image directory from some of the binary release files.
case $HDL_PLATFORM in
    (zed)
	cp zed/boot.bin $sd
	;;
esac
echo Populating the SD image directory from patched kernel/device-tree/rootfs
# Use the device tree from the kernel build since it might be patched...
cp dts/zynq-$HDL_PLATFORM.dtb $sd/devicetree.dtb
# Use the kernel from the kernel build since it might be patched...
cp uImage $sd
# Use the new patched root fs.
cp uramdisk.image.gz $sd
echo Adding OpenCPI setup scripts to the SD image directory
mkdir -p $sd/opencpi/lib $sd/opencpi/bin $sd/opencpi/artifacts $sd/opencpi/xml
cp $OCPI_CDK_DIR/platforms/zynq/zynq_net_setup.sh $OCPI_CDK_DIR/platforms/zynq/zynq_setup.sh $sd/opencpi
echo You should have already customized the mysetup.sh script for your environment
if test -r $OCPI_CDK_DIR/platforms/zynq/mynetsetup.sh; then
  cp $OCPI_CDK_DIR/platforms/zynq/mynetsetup.sh $sd/opencpi
fi
if test -r $OCPI_CDK_DIR/platforms/zynq/mysetup.sh; then
  cp $OCPI_CDK_DIR/platforms/zynq/mysetup.sh $sd/opencpi
fi

# After this is files for standalone operation
shopt -s nullglob
drivers=(${KERNEL_LIB_DIR}/opencpi*.ko)
shopt -u nullglob
if test "${drivers[*]}" = ""; then
  echo No OpenCPI linux kernel drivers for $OCPI_TARGET_PLATFORM have been built.
  echo It is expected to be in: "${KERNEL_LIB_DIR}/opencpi*.ko"
  exit 1
fi
# Note we take files from OCPI_TARGET_DIR and put them in OCPI_TARGET_PLATFORM
# So we can support building SD cards from particular modes, but any particular SD card will
# have only one mode for now.
mkdir $sd/opencpi/lib/${OCPI_TARGET_PLATFORM}
cp -L ${KERNEL_LIB_DIR}/opencpi*.ko $sd/opencpi/lib/${OCPI_TARGET_PLATFORM}
cp -L ${KERNEL_LIB_DIR}/mdev-opencpi.rules $sd/opencpi/lib/${OCPI_TARGET_PLATFORM}
for b in run hdl zynq serve xml; do
  cp -L $BIN_DIR/ocpi$b $sd/opencpi/bin
  # Ensure the deployed files are stripped - if we debug we'll be looking at dev-sys executables
  test -z $RPM_BUILD_ROOT && $OCPI_CROSS_BUILD_BIN_DIR/$OCPI_CROSS_HOST-strip $sd/opencpi/bin/ocpi$b
done
# we use rdate for now... : cp ../ntpclient $sd/opencpi/bin
# copy driver libraries to the subdirectory so that OCPI_CDK_DIR will
# find them.
cp -L ${RUNTIME_LIB_DIR}/*_s.so $sd/opencpi/lib/${OCPI_TARGET_PLATFORM}
cp -L $OCPI_CDK_DIR/scripts/ocpibootstrap.sh $sd/opencpi/bin
cp -L $OCPI_CDK_DIR/scripts/ocpidriver $sd/opencpi/bin
cp -L $OCPI_CDK_DIR/scripts/ocpi_linux_driver $sd/opencpi/bin
cp -L ${EXAMPLES_ROOTDIR}/applications/{*.xml,test.input} $sd/opencpi/xml

# Add the default system.xml to the SD card.
sx=../system.xml
[ -f $sx ] || sx=$OCPI_CDK_DIR/platforms/zynq/zynq_system.xml
cp $sx $sd/opencpi/system.xml
n=0
echo Adding artifacts found in OCPI_LIBRARY_PATH for ${OCPI_TARGET_PLATFORM} and ${HDL_PLATFORM} HDL targets.
export OCPI_SYSTEM_CONFIG=
for i in $(${OCPI_CDK_DIR}/${OCPI_TOOL_DIR}/bin/ocpirun -A ${OCPI_TARGET_PLATFORM},${HDL_PLATFORM} | xargs -rn1 readlink -e | sort -u ); do
  cp $i $sd/opencpi/artifacts/$(printf %03d-%s $n $(basename $i))
  n=$(expr $n + 1)
done
echo Added $n artifacts to SD image.
cd ..
echo New OpenCPI Release SD:
du -k -s -h $REL/$sd/opencpi/artifacts
du -k -s -h $REL/$sd/opencpi/lib
du -k -s -h $REL/$sd/opencpi/bin
du -k -s -h $REL/$sd/opencpi
du -k -s -h $REL/$sd
