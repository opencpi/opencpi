#!/bin/sh
set -e
if test "$1" = ""; then
  cat <<EOF
This script prepares OpenCPI to use a Zed board by creating and filling a directory
to be written to an SD card for booting the zed board.
It assumes you have completed the zed platform OpenCPI build, including the kernel driver.

This script can be repeatedly run; it first clears out the directory it is filling.

It assumes you are running it where it lives in the OpenCPI tree, in platforms/zed.
This script takes a single argument to run: the Xilinx Zynq release name to use.
Don't forget to copy defaultsetup.sh to mysetup.sh and customize mysetup.sh before doing this.
Also, artifacts are copied based on OCPI_LIBRARY_PATH, so make sure it includes the desired
artifacts for running on the zed.
The result will be placed in a subdirectory of the release directory, called:
release-{release}/OpenCPI-SD.  You can further customize it if needed.
EOF
  exit 1
fi
if test $1 = -; then
  echo Using the current Xilinx kernel release:
  if test ! -d release; then
    echo Error: the '"release"' symlink is missing.  It should point to the current release.
    exit 1
  fi
  ls -l release
  REL=release
else
  REL=release-$1
  if test ! -d release-$1; then
    echo The release directory, release-$1, doesn\'t exist.
    exit 1
  fi
fi

export OCPI_TARGET_PLATFORM=zed
if test -z $RPM_BUILD_ROOT; then
  source ../../exports/scripts/ocpisetup.sh ../../exports/scripts/ocpisetup.sh
  EXAMPLES_ROOTDIR=$OCPI_CDK_DIR
  OCPIRUN_PATH=$OCPI_CDK_DIR/bin/$OCPI_TOOL_HOST/
  if test "$OCPI_LIBRARY_PATH" = ""; then
    # Put all rcc components, and precanned bitstreams for the platform.
    export OCPI_LIBRARY_PATH=$OCPI_CDK_DIR/lib/components:$OCPI_CDK_DIR/lib/platforms/zed
  fi
else
  echo RPM Build detected - faking directory structure
  OCPI_CDK_DIR=${RPM_BUILD_ROOT}/opt/opencpi/cdk
  # Cannot just use CDK/lib and CDK/bin because the driver stuff isn't pushed there
  EXAMPLES_ROOTDIR=${RPM_BUILD_ROOT}/opt/opencpi/base_project_source
  OCPIRUN_PATH=${RPM_BUILD_ROOT}/opt/opencpi/cdk/bin/$OCPI_TOOL_HOST/
  export OCPI_LIBRARY_PATH=${OCPI_CDK_DIR}/components/:${OCPI_HDL_PLATFORM_PATH}/${OCPI_TARGET_PLATFORM}/
fi
BIN_DIR=${OCPI_CDK_DIR}/bin/linux-zynq-arm
KERNEL_LIB_DIR=$OCPI_CDK_DIR/lib/linux-zynq-arm
RUNTIME_LIB_DIR=${OCPI_CDK_DIR}/lib/linux-zynq-arm

cd $REL
sd=OpenCPI-SD
rel=SD-release
rm -r -f $sd
mkdir $sd
echo Populating the SD image directory from the release files.
#cp uImage uramdisk.image.gz zed/boot.bin zed/devicetree.dtb ../$sd
cp $rel/zed/boot.bin $sd
echo Populating the SD image directory from patched kernel/device-tree/rootfs
# Use the device tree from the kernel build since it might be patched...
cp zynq-zed.dtb $sd/devicetree.dtb
# Use the kernel from the kernel build since it might be patched...
cp uImage $sd
# Use the new patched root fs.
cp uramdisk.image.gz $sd
echo Adding OpenCPI setup scripts to the SD image directory
mkdir -p $sd/opencpi/lib $sd/opencpi/bin $sd/opencpi/artifacts $sd/opencpi/xml
cp ../zednetsetup.sh ../zedsetup.sh $sd/opencpi
echo You should have already customized the mysetup.sh script for your environment
if test -r ../mynetsetup.sh; then
  cp ../mynetsetup.sh $sd/opencpi
fi
if test -r ../mysetup.sh; then
  cp ../mysetup.sh $sd/opencpi
fi

# After this is files for standalone operation
shopt -s nullglob
drivers=(${KERNEL_LIB_DIR}/opencpi*.ko)
shopt -u nullglob
if test "${drivers[*]}" = ""; then
  echo No OpenCPI linux kernel drivers for zed have been built.
  echo It is expected to be in: "${KERNEL_LIB_DIR}/opencpi*.ko"
  exit 1
fi
mkdir $sd/opencpi/lib/linux-zynq-arm
cp -L ${KERNEL_LIB_DIR}/opencpi*.ko $sd/opencpi/lib/linux-zynq-arm
cp -L ${KERNEL_LIB_DIR}/mdev-opencpi.rules $sd/opencpi/lib/linux-zynq-arm
for b in run hdl zynq serve; do
  cp -L $BIN_DIR/ocpi$b $sd/opencpi/bin
  test -z $RPM_BUILD_ROOT && $OCPI_CROSS_BUILD_BIN_DIR/$OCPI_CROSS_HOST-strip $sd/opencpi/bin/ocpi$b
done
# we use rdate for now... : cp ../ntpclient $sd/opencpi/bin
# cp ../libstdc++.so.6 $sd/opencpi/lib
# copy driver libraries to the subdirectory so that OCPI_CDK_DIR will
# find them.
cp -L ${RUNTIME_LIB_DIR}/*_s.so $sd/opencpi/lib/linux-zynq-arm
cp -L $OCPI_CDK_DIR/scripts/ocpibootstrap.sh $sd/opencpi/bin
cp -L $OCPI_CDK_DIR/scripts/ocpidriver $sd/opencpi/bin
cp -L $OCPI_CDK_DIR/scripts/ocpi_linux_driver $sd/opencpi/bin
cp -L ${EXAMPLES_ROOTDIR}/examples/xml/{*.xml,test.input} $sd/opencpi/xml

# Add the default system.xml to the SD card.
cp ../system.xml $sd/opencpi
n=0
echo Adding artifacts found in OCPI_LIBRARY_PATH for linux-zynq-arm and zed targets.
export OCPI_SYSTEM_CONFIG=
for i in $(${OCPIRUN_PATH}ocpirun -A linux-zynq-arm,zed | sort); do
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
