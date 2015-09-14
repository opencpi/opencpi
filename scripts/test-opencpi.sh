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
if test "$OCPI_TARGET_DIR" = ""; then
  if test "${OCPI_TARGET_MODE+UNSET}" = ""; then
    if test "$OCPI_DEBUG" = 1; then do=d; else do=o; fi
    if test "$OCPI_BUILD_SHARED_LIBRARIES" = 1; then sd=d; else sd=s; fi
    export OCPI_TARGET_MODE=$sd$do
  fi
  OCPI_TARGET_DIR=${OCPI_TARGET_HOST}${OCPI_TARGET_MODE:+/${OCPI_TARGET_MODE}}
fi
echo ======================= Loading the OpenCPI Linux Kernel driver. &&
(test "$OCPI_TOOL_OS" = macos || ocpidriver load) &&
echo ======================= Running Unit Tests &&
tests/target-$OCPI_TARGET_DIR/ocpitests &&
echo ======================= Running Container Tests &&
(cd runtime/ctests/target-$OCPI_TARGET_DIR && ${OCPI_TARGET_MODE:+../}../bin/src/run_tests.sh) &&
echo ======================= Running Datatype/protocol Tests &&
tools/cdk/ocpidds/target-$OCPI_TARGET_DIR/ocpidds -t 10000 > /dev/null &&
echo All tests passed.
