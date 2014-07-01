#!/bin/sh
# The sourced script that starts most other install scripts
if test "$OCPI_PREREQUISITES_BUILD_DIR" = ""; then
  export OCPI_PREREQUISITES_BUILD_DIR=/opt/opencpi/prerequisites
fi
mkdir -p $OCPI_PREREQUISITES_BUILD_DIR
if test "$OCPI_PREREQUISITES_INSTALL_DIR" = ""; then
  export OCPI_PREREQUISITES_INSTALL_DIR=/opt/opencpi/prerequisites
fi
mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR
if test "$OCPI_TOOL_HOST" = ""; then
  vars=($(platforms/getPlatform.sh))
  if test $? != 0; then
    echo Failed to determine runtime platform.
    return 1
  fi
  export OCPI_TOOL_OS=${vars[0]}
  export OCPI_TOOL_OS_VERSION=${vars[1]}
  export OCPI_TOOL_ARCH=${vars[2]}
  export OCPI_TOOL_HOST=${vars[3]}
fi
if test "$OCPI_TARGET_OS" = ""; then
  export OCPI_TARGET_OS=$OCPI_TOOL_OS
  export OCPI_TARGET_OS_VERSION=$OCPI_TOOL_OS_VERSION
  export OCPI_TARGET_ARCH=$OCPI_TOOL_ARCH
  export OCPI_TARGET_HOST=$OCPI_TOOL_HOST
fi
set -e
cd $OCPI_PREREQUISITES_BUILD_DIR
