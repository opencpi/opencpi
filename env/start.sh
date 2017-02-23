# Sourced script to start the process of a customized environment.

if test "$OCPI_CDK_DIR" != ""; then
  echo "Warning!!!!!!: "you are setting up the OpenCPI build environment when it is already set.
  echo "Warning!!!!!!: "this is not guaranteed to work.  You should probably use a new shell.
  echo You can also \"source scripts/clean-env.sh\" to start over.
fi
# Initialize access to the core tree's export directory
source ./scripts/core-init.sh
# Run the bootstrap to establish the CDK and the OCPI_TOOL_* variables
# This usually generates any errors before the custom script gets control
export OCPI_BOOTSTRAP=$(pwd)/exports/scripts/ocpibootstrap.sh; source $OCPI_BOOTSTRAP
