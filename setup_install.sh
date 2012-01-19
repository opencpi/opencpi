#!/bin/sh
# The sourced script that starts most other install scripts
OCPI_PREREQUISITES_BUILD_DIR=/opt/opencpi/prerequisites
OCPI_PREREQUISITES_INSTALL_DIR=/opt/opencpi/prerequisites
OCPI_BUILD_OS=`uname -s | tr A-Z a-z`
OCPI_BUILD_PROCESSOR=`uname -m | tr A-Z a-z`
OCPI_BUILD_TARGET=$OCPI_BUILD_OS-$OCPI_BUILD_PROCESSOR
OCPI_BUILD_HOST=$OCPI_BUILD_TARGET
cd $OCPI_PREREQUISITES_BUILD_DIR
