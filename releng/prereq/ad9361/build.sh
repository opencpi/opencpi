#!/bin/sh
set -e
# As of 13 Oct 2017, 2016_R2 points to e9f8fe509cc0e3685cdca47998979f287be4c360 (AV-3478)
OCPI_AD9361_CURRENT_2016_R2_GIT_COMMIT_ID=e9f8fe509cc0e3685cdca47998979f287be4c360
OCPI_AD9361_VERSION=$OCPI_AD9361_CURRENT_2016_R2_GIT_COMMIT_ID
# was master

# set these for prereq_utils:
SPECFILE=ocpi_ad9361.spec
OCPI_BUNDLED_VERSION=${OCPI_AD9361_VERSION}
. ../prereq_utils.sh

if [ "$1" != "rpm" ]; then
    echo This script only builds RPMs. Try \"$0 rpm\".
    exit 99
fi

SUBDIR=ad9361-gitrepo
TARBALL=ad9361-bundled.tar
rm -rf ${TARBALL}

prereq_init_git ${SUBDIR} https://github.com/analogdevicesinc/no-OS.git ${OCPI_BUNDLED_VERSION}

cp ../../../scripts/ad9361.patch ../../../scripts/install-ad9361.sh ${SUBDIR}
rm -f ad9361
ln -s ${SUBDIR} ad9361
echo Creating tarball:
tar --exclude-vcs -hcf ${TARBALL} ad9361
ls -halF ${TARBALL}
rm -f ad9361

OCPI_AD9361_COMMIT_SHORT=$(cd ${SUBDIR} && git rev-parse --short HEAD)

# Parameters:
# 1: platform, e.g. arm-xilinx-linux-gnueabi
# 2: target host, e.g. linux-x13_3-arm
# 3: software platform, e.g. x13_3
# 4: RPM platform nice name, e.g. zynq
# 5: CFLAGS, e.g. "-O2 -g -pipe -Wall"
#    For the OCPI_CFLAGS, start with the ones in /usr/lib/rpm/redhat/macros for %__global_cflags and then take out
#    the ones that fail on the target platform.
#    Then add the ones that tune for the platform.
#    It goes through an "echo" to evaluate if things like ${CROSS_DIR} were passed in.
# 6-9: Any extra rpmbuild options, e.g. "--define=OCPI_AD9361_COMMIT_SHORT ${OCPI_AD9361_COMMIT_SHORT}"

if [ -e ${SPECFILE} ]; then
    mkdir -p ~/rpmbuild/SOURCES || :
    cp ${TARBALL} ~/rpmbuild/SOURCES
    skip_host || rpmbuild -ba ${SPECFILE} \
      --define="OCPI_AD9361_COMMIT_SHORT ${OCPI_AD9361_COMMIT_SHORT}" \
      --define="OCPI_TARGET_HOST ${OCPI_TARGET_HOST}" \
      --define="OCPI_BUNDLED_VERSION ${OCPI_BUNDLED_VERSION}"
    ### Cross compile for zynq (Copy this block for other platform targets)
    cross_build arm-xilinx-linux-gnueabi linux-x13_3-arm x13_3 zynq "-O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions --param=ssp-buffer-size=4 -grecord-gcc-switches -mfpu=neon-fp16 -mfloat-abi=softfp -march=armv7-a -mtune=cortex-a9" "--define=OCPI_AD9361_COMMIT_SHORT ${OCPI_AD9361_COMMIT_SHORT}"
    cp -v ~/rpmbuild/RPMS/*/ocpi-prereq-ad9361* . || :
    rm *.tar
else
    echo "Missing RPM spec file in `pwd`"
    exit 1
fi
