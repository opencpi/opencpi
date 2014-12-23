#!/bin/sh
# Run all the go-no-go tests we have
set -e
if test "$OCPI_BASE_DIR" != ""; then
  echo Since OCPI_BASE_DIR is set, we will use the existing environment.
else
  # We're being run in an uninitialized environment
  if test ! -d env; then
    echo It appears that this script is not being run at the top level of OpenCPI.
    exit 1
  fi
  source env/default-env.sh
  test $? = 0 || exit 1; 
fi
echo ======================= Loading the OpenCPI Linux Kernel driver. &&
(test "$OCPI_TOOL_OS" = macos || ocpidriver load) &&
echo ======================= Running Unit Tests &&
tests/target-$OCPI_TARGET_HOST/ocpitests &&
echo ======================= Running Container Tests &&
(cd core/ctests/target-$OCPI_TARGET_HOST && ../bin/src/run_tests.sh) &&
echo ======================= Running Datatype/protocol Tests &&
tools/cdk/ocpidds/target-$OCPI_TARGET_HOST/ocpidds -t 10000 > /dev/null &&
echo All tests passed.
