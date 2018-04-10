#!/bin/bash --noprofile

# Run the av-tests, assuming a clean tree
set -e
shopt -s expand_aliases
alias odev="$OCPI_CDK_DIR/$OCPI_TOOL_DIR/bin/ocpidev -v"
echo Cleaning the project
make cleaneverything
echo Building the components
odev build rcc
echo Building the application
odev build application aci_property_test_app
echo Running the test application
(cd applications/aci_property_test_app &&
  OCPI_LIBRARY_PATH=../../:$OCPI_LIBRARY_PATH ./target-$OCPI_TARGET_DIR/test_app)
cd components
odev build worker prop_mem_align_info.rcc
odev build test prop_mem_align_info.test
