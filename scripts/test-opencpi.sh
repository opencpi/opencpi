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
minimal_tests="os datatype load-drivers container driver"
network_tests="assets"
dev_tests="swig python av ocpidev core inactive"
alltests="$minimal_tests $network_tests $dev_tests"
tests="$minimal_tests"
# runtime/standalone tests we can run
case "$1" in
  --showtests)
    echo $alltests && exit 0;;
  --help|-h) 
    echo This script runs various built-in tests.
    echo Available tests are: $alltests
    echo 'Usage is: ocpitest [--showtests | --help ] [<platform> [ <test> ... ]]'
    exit 1;;
  -*)
    echo Unknown option: $1
    exit 1;;  
esac
platform=$1
shift
the_tests="$*"
# Note the -e is so, especially in embedded environments, we do not deal with getPlatform.sh etc.
[ -L cdk ] && source `pwd`/cdk/opencpi-setup.sh -e 
[ -z "$OCPI_CDK_DIR" ] && echo No OpenCPI CDK available && exit 1
if [ -n "$the_tests" ]; then
  tests="$the_tests"
else
  if [ -d $OCPI_CDK_DIR/../project-registry/ocpi.assets/exports ]; then
    echo ========= Running project-based tests since \"projects/core/exports\" is available
    tests="$tests $network_tests"
  fi
  # Note "which -s" not available on busybox
  if which make > /dev/null && [ -d $OCPI_CDK_DIR/../project-registry ] ; then
    echo ========= Running dev system tests since \"make\" and project-registry is available.
    tests="$tests $dev_tests"   
    runtime=
    source $OCPI_CDK_DIR/scripts/ocpitarget.sh $platform
  else
    echo ========= Running only runtime tests since \"make\" or project-registry is not available.
    runtime=1
    [ -n "$platform" -a "$platform" != $OCPI_TOOL_PLATFORM ] && {
      echo "Cannot run tests on a different targeted platform ($platform) than we are running on."
      exit 1
    }
  fi
fi
[ -z "$OCPI_TARGET_PLATFORM" ] && {
  # Set just enough target variables to run runtime tests
  export OCPI_TARGET_PLATFORM=$OCPI_TOOL_PLATFORM
  export OCPI_TARGET_OS=$OCPI_TOOL_OS
  export OCPI_TARGET_DIR=$OCPI_TOOL_DIR
}
bin=$OCPI_CDK_DIR/$OCPI_TARGET_DIR/bin
set -e
[ -z "$TESTS" ] && TESTS="$tests"
echo ======================= Running these tests: $TESTS
# Arg 1 is the directory under tests/ to go
function framework_test {
  local dir=$OCPI_CDK_DIR/../tests/$1
  [ -d $dir ] || {
    dir=tests/$1 
    [ -d tests/$1 ] || {
      echo The framework tests in tests/$1 is not present >&2
      exit 1
    }
  }
  cd $dir
}

for t in $TESTS; do
  set -e # required inside a for;do;done to enable this case/esac to fail
  case $t in
    os)
      echo ======================= Running OS Unit Tests in $bin
      $VG $bin/cxxtests/ocpitests;;
    datatype)
      echo ======================= Running Datatype/protocol Tests
      $VG $bin/cxxtests/ocpidds -t 10000 > /dev/null;;
    container)
      echo ======================= Running Container Tests
      $bin/ctests/run_tests.sh;;
    ##########################################################################################
    # After this we are depending on the core project being built for the targeted platform
    swig)
      echo ======================= Running python swig test
      OCPI_LIBRARY_PATH=$OCPI_CDK_DIR/../projects/core/exports/artifacts \
		       PYTHONPATH=$OCPI_CDK_DIR/$OCPI_TARGET_DIR/lib \
		       python - $OCPI_CDK_DIR/../projects/assets/applications/bias.xml <<-EOF
	import sys
	old=sys.getdlopenflags()
	if sys.platform != 'darwin':
	   import ctypes
	   sys.setdlopenflags(old|ctypes.RTLD_GLOBAL)
	import OcpiApi as OA
	sys.setdlopenflags(old)
	app=OA.Application(sys.argv[1])
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
      (framework_test pytests && git clean -dfx . && ./run_pytests.sh);;
    av)
      echo ======================= Running av_tests
      (framework_test av-test && ./run_avtests.sh);;
    ocpidev)
      echo ======================= Running ocpidev tests
      (framework_test /ocpidev && ./run-dropin-tests.sh)
      # These tests might do HDL building
      hplats=($HdlPlatform $HdlPlatforms)
      echo ======================= Running ocpidev_test tests
      (unset HdlPlatforms; unset HdlPlatforms; \
       framework_test ocpidev_test && rm -r -f test_project && \
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
echo ======================= All tests passed.
