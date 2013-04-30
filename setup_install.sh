#!/bin/sh
# The sourced script that starts most other install scripts
if test "$OCPI_PREREQUISITES_BUILD_DIR" == ""; then
  export OCPI_PREREQUISITES_BUILD_DIR=/opt/opencpi/prerequisites
fi
mkdir -p $OCPI_PREREQUISITES_BUILD_DIR
if test "$OCPI_PREREQUISITES_INSTALL_DIR" == ""; then
  export OCPI_PREREQUISITES_INSTALL_DIR=/opt/opencpi/prerequisites
fi
mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR
if test "$OCPI_TARGET_HOST" == ""; then
  ovp=(`showRuntimeHost`)
  if test "$ovp" = "" -o "${#ovp[*]}" != 3; then
    echo Error determining run time host.  1>&2
    exit 1
  fi
  export OCPI_TARGET_OS=${ovp[0]}
  export OCPI_TARGET_OS_VERSION=${ovp[1]}
  export OCPI_TARGET_ARCH=${ovp[2]}
  export OCPI_TARGET_HOST=$OCPI_TARGET_OS-$OCPI_TARGET_OS_VERSION-$OCPI_TARGET_ARCH
fi
set -e
cd $OCPI_PREREQUISITES_BUILD_DIR
