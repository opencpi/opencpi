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

##########################################################################################
# Run all the go-no-go tests we have

[ -z "$OCPI_CDK_DIR" -a -L cdk ] && source `pwd`/cdk/opencpi-setup.sh -
source $OCPI_CDK_DIR/scripts/ocpitarget.sh $1
bin=$OCPI_CDK_DIR/$OCPI_TARGET_DIR/bin
set -e
echo ======================= Running OS Unit Tests in $bin
$bin/gtests/ocpitests
echo ======================= Running Datatype/protocol Tests
$bin/ocpidds -t 10000 > /dev/null
echo ======================= Running Container Tests
$OCPI_CDK_DIR/scripts/run_tests.sh
##########################################################################################
# After this we are depending on the core project being built for the targeted platform
echo ======================= Running python swig test
OCPI_LIBRARY_PATH=projects/core/exports/lib/components \
PYTHONPATH=$OCPI_CDK_DIR/$OCPI_TOOL_DIR/lib \
python<<EOF
import sys;import dl
old=sys.getdlopenflags();
sys.setdlopenflags(old|dl.RTLD_GLOBAL)
import OcpiApi as OA
sys.setdlopenflags(old)
app=OA.Application("projects/assets/applications/bias.xml")
EOF
echo ======================= Running unit tests in project/core
make -C $OCPI_CDK_DIR/../projects/core runtest
##########################################################################################
# After this we are depending on the other projects being built for the targeted platform
echo ======================= Running Application tests in project/assets
make -C $OCPI_CDK_DIR/../projects/assets/applications run
echo ======================= Running Application tests in project/inactive
make -C $OCPI_CDK_DIR/../projects/inactive/applications run
echo ======================= Running Python/ocpidev Tests in tests/pytests
(cd $OCPI_CDK_DIR/../tests/pytests && git clean -dfx . && ./run_pytests.sh)
echo ======================= Running av_test application
(cd $OCPI_CDK_DIR/../tests/av-test && ./run_avtests.sh)
echo ======================= Running ocpidev tests
# These tests might do HDL building
hplats=($HdlPlatform $HdlPlatforms)
(unset HdlPlatforms; unset HdlPlatforms; \
 cd $OCPI_CDK_DIR/../tests/ocpidev_test && rm -r -f test_project && \
     HDL_PLATFORM=$hplats ./test-ocpidev.sh)
echo All tests passed.
[ "$OCPI_TOOL_OS" != macos ] && {
  echo ======================= Loading the OpenCPI Linux Kernel driver. &&
    $OCPI_CDK_DIR/scripts/ocpidriver load
}
exit 0
