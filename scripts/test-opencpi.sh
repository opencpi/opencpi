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
alltests="os datatype load-drivers container swig core assets inactive python av ocpidev driver"
[ "$1" = --help ] && {
    echo Available tests are: $alltests
    echo 'Uses TESTS="a b c" ./scripts/test-opencpi.sh [<platform>]'
    exit 1
}
source $OCPI_CDK_DIR/scripts/ocpitarget.sh $1
bin=$OCPI_CDK_DIR/$OCPI_TARGET_DIR/bin
set -e
[ -z "$TESTS" ] && TESTS="$alltests"
for t in $TESTS; do
  case $t in
    os)
      echo ======================= Running OS Unit Tests in $bin
      $VG $bin/cxxtests/ocpitests;;
    datatype)
      echo ======================= Running Datatype/protocol Tests
      $VG $bin/ocpidds -t 10000 > /dev/null;;
    container)
      echo ======================= Running Container Tests
      ./runtime/ctests/src/run_tests.sh;;
    ##########################################################################################
    # After this we are depending on the core project being built for the targeted platform
    swig)
      echo ======================= Running python swig test
      OCPI_LIBRARY_PATH=projects/core/exports/artifacts \
		       PYTHONPATH=$OCPI_CDK_DIR/$OCPI_TOOL_DIR/lib \
		       python<<-EOF
	import sys
	old=sys.getdlopenflags();
	if sys.platform != 'darwin':
	   import dl
	   sys.setdlopenflags(old|dl.RTLD_GLOBAL)
	import OcpiApi as OA
	sys.setdlopenflags(old)
	app=OA.Application("projects/assets/applications/bias.xml")
	EOF
      ;;
    core)
      echo ======================= Running unit tests in project/core
      make -C $OCPI_CDK_DIR/../projects/core runtest;;
    ##########################################################################################
    # After this we are depending on the other projects being built for the targeted platform
    assets)
      echo ======================= Running Application tests in project/assets
      make -C $OCPI_CDK_DIR/../projects/assets/applications run;;
    inactive)
      echo ======================= Running Application tests in project/inactive
      make -C $OCPI_CDK_DIR/../projects/inactive/applications run;;
    python)
      echo ======================= Running Python utility tests in tests/pytests
      (cd $OCPI_CDK_DIR/../tests/pytests && git clean -dfx . && ./run_pytests.sh);;
    av)
      echo ======================= Running av_tests
      (cd $OCPI_CDK_DIR/../tests/av-test && ./run_avtests.sh);;
    ocpidev)
      echo ======================= Running ocpidev tests
      (cd $OCPI_CDK_DIR/../tests/ocpidev && ./run-dropin-tests.sh)
      # These tests might do HDL building
      hplats=($HdlPlatform $HdlPlatforms)
      echo ======================= Running ocpidev_test tests
      (unset HdlPlatforms; unset HdlPlatforms; \
       cd $OCPI_CDK_DIR/../tests/ocpidev_test && rm -r -f test_project && \
         HDL_PLATFORM=$hplats ./test-ocpidev.sh);;
    load-drivers)
      echo ======================= Loading all the OpenCPI plugins/drivers.
      $bin/cxxtests/load-drivers x;;
    driver)
      [ "$OCPI_TOOL_OS" != macos ] &&
        echo ======================= Loading the OpenCPI Linux Kernel driver. &&
        $OCPI_CDK_DIR/scripts/ocpidriver load;;
    *)
      echo Error: the test \"$t\" is not a valid/known test.
      exit 1;;
  esac
done
echo All tests passed.
exit 0
