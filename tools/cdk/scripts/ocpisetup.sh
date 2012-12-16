# This script should be sourced to set the OpenCPI CDK environment variables
if [ "${BASH_ARGV[0]}" == "" ]; then
  echo Error: ocpisetup.sh can only be run using the \".\" or \"source\" command.
  exit 1
elif [[ "${BASH_ARGV[0]}" != */* ]]; then
  echo Error: ocpisetup.sh can only be run from a different directory.
  return 1
else
  export OCPI_CDK_DIR=`dirname ${BASH_ARGV[0]}`
  export OCPI_CDK_DIR=`cd $OCPI_CDK_DIR; pwd`
  export OCPI_RUNTIME_SYSTEM=`$OCPI_CDK_DIR/scripts/showRuntimeHost`
  export PATH=$OCPI_CDK_DIR/bin/$OCPI_RUNTIME_SYSTEM:$OCPI_CDK_DIR/scripts:$PATH
#  if [[ $OCPI_RUNTIME_SYSTEM == darwin-* ]]; then
#    export DYLD_LIBRARY_PATH=$OCPI_CDK_DIR/lib/$OCPI_RUNTIME_SYSTEM:$DYLD_LIBRARY_PATH
#  else
#    export LD_LIBRARY_PATH=$OCPI_CDK_DIR/lib/$OCPI_RUNTIME_SYSTEM:$LD_LIBRARY_PATH
# fi
  export OCPI_LIBRARY_PATH=$OCPI_CDK_DIR/lib/components
  echo OCPI_CDK_DIR is $OCPI_CDK_DIR and OCPI_RUNTIME_SYSTEM is $OCPI_RUNTIME_SYSTEM
fi
