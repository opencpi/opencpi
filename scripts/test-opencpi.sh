#!/bin/bash --noprofile
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

# This are in order.
# Run three categories of tests in this order:
# 1. Framework tests that do not deal with projects at all.
# 2. Tests that might test project tools, but not using the builtin projects
# 3. Tests using the built-in projects
alltests="os datatype load-drivers container swig python driver av ocpidev core assets inactive"
case "$1" in
  --showtests)
    echo $alltests && exit 0;;
  --help|-h) 
    echo Available tests are: $alltests
    echo 'Uses TESTS="a b c" ./scripts/test-opencpi.sh [<platform>]'
    exit 1;;
esac
if ! which make; then
  echo ========= Running only runtime tests '(no development tool tests)'
  runtime=1
  alltests="os datatype load-drivers container driver assets" # core assets inactive
fi
# Note the -e is so, especially in embedded environments, we do not deal with getPlatform.sh etc.
[ -L cdk ] && source `pwd`/cdk/opencpi-setup.sh -e 
source $OCPI_CDK_DIR/scripts/ocpitarget.sh $1
bin=$OCPI_CDK_DIR/$OCPI_TARGET_DIR/bin
[ -n "$1" -a "$1" != $OCPI_TOOL_PLATFORM ] &&
    
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
		       PYTHONPATH=$OCPI_CDK_DIR/$OCPI_TARGET_DIR/lib \
		       python<<-EOF
	import sys
	old=sys.getdlopenflags();
	if sys.platform != 'darwin':
	   import ctypes
	   sys.setdlopenflags(old|ctypes.RTLD_GLOBAL)
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
      if [ -z $runtime ] ; then
        make -C $OCPI_CDK_DIR/../projects/assets/applications run
      else
        (cd $OCPI_CDK_DIR/../projects/assets/applications; ./run.sh)
      fi;;
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
      if [ "$OCPI_TOOL_OS" != linux ]; then
        echo ======================= Skipping loading the OpenCPI kernel driver:  not running linux.
      elif [ -e /.dockerenv ] ; then
        echo ======================= Skipping loading the OpenCPI kernel driver:  running in a docker container.
      else
        echo ======================= Loading the OpenCPI Linux Kernel driver. &&
            $OCPI_CDK_DIR/scripts/ocpidriver status
            $OCPI_CDK_DIR/scripts/ocpidriver unload
            $OCPI_CDK_DIR/scripts/ocpidriver load
      fi;;
    *)
      echo Error: the test \"$t\" is not a valid/known test.
      exit 1;;
  esac
done
echo All tests passed.
exit 0
