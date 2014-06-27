#!/bin/sh
# Run all the go-no-go tests we have
set -e
echo ======================= Running Unit Tests
tests/target-$OCPI_TARGET_HOST/ocpitests &&
echo ======================= Running Container Tests &&
(cd core/container/ctests/target-$OCPI_TARGET_HOST && ../bin/src/run_tests.sh) &&
echo ======================= Running Datatype/protocol Tests &&
tools/cdk/ocpidds/target-$OCPI_TARGET_HOST/ocpidds -t 10000 > /dev/null &&
echo All tests passed.
