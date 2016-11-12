#!/bin/sh
set -e
if test "$OCPI_CDK_DIR" != ""; then
  echo Since OCPI_CDK_DIR is set, we will use the existing environment.
  if test "$1" != ""; then
   if "$1" != "$OCPI_TARGET_PLATFORM"; then
      echo Error: supplied platform $1 is different from the environment: $OCPI_TARGET_PLATFORM
      exit 1
   fi
  fi
else
  export OCPI_TARGET_PLATFORM=$1
  # Initialize access to the core tree's export directory
  source scripts/core-init.sh
  # Initialize access to CDK
  source exports/scripts/ocpisetup.sh exports/scripts/ocpisetup.sh
fi
echo ================================================================================
echo We are running in `pwd` where the git clone of opencpi has been placed.
echo ================================================================================
echo Now we will '"make"' the core OpenCPI libraries and utilities for $OCPI_TARGET_PLATFORM
make
echo ================================================================================
echo Now we will '"make"' the built-in RCC '(software)' components for $OCPI_TARGET_PLATFORM
make rcc
echo ================================================================================
echo Now we will '"make"' the examples for $OCPI_TARGET_PLATFORM
make examples
echo ================================================================================
echo Finally, we will built the OpenCPI kernel device driver for $OCPI_TARGET_PLATFORM
make driver
echo ================================================================================
echo OpenCPI has been built for $OCPI_TARGET_PLATFORM, with software components, examples and kernel driver
trap - ERR
