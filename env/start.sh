# Sourced script to start the process of a customized environment.

# Initialize access to the core tree's export directory
source ./scripts/core-init.sh
# Run the bootstrap to establish the CDK and the OCPI_TOOL_* variables
# This usually generates any errors before the custom script gets control
export OCPI_BOOTSTRAP=$(pwd)/exports/scripts/ocpibootstrap.sh; source $OCPI_BOOTSTRAP
