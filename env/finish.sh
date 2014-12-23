# The TARGET vars might be set for cross-compilation, but if not, default to the
# TOOL host vars
if test "$OCPI_TARGET_OS" = ""; then
  export OCPI_TARGET_OS=$OCPI_TOOL_OS
  export OCPI_TARGET_OS_VERSION=$OCPI_TOOL_OS_VERSION
  export OCPI_TARGET_ARCH=$OCPI_TOOL_ARCH
  export OCPI_TARGET_HOST=$OCPI_TOOL_HOST
  export OCPI_TARGET_PLATFORM=$OCPI_TOOL_PLATFORM
fi
if test "$OCPI_OMNI_DIR" != ""; then
  export OCPI_OMNI_BIN_DIR=$OCPI_OMNI_DIR/$OCPI_TARGET_HOST/bin
  export OCPI_OMNI_IDL_DIR=$OCPI_OMNI_DIR/$OCPI_TARGET_HOST/share/idl/omniORB
  export OCPI_OMNI_LIBRARY_DIR=$OCPI_OMNI_DIR/$OCPI_TARGET_HOST/lib
  export OCPI_OMNI_INCLUDE_DIR=$OCPI_OMNI_DIR/$OCPI_TARGET_HOST/include
  export OCPI_CORBA_INCLUDE_DIRS="$OCPI_OMNI_INCLUDE_DIR $OCPI_OMNI_INCLUDE_DIR/omniORB4"
fi
# Post processing after custom env setup
export OCPI_OUT_DIR=target-$OCPI_TARGET_HOST
# compatibility
export OCPI_OS=$OCPI_TARGET_OS
export OCPI_ARCH=$OCPI_TARGET_ARCH
# For now this script needs to know where it is, and on some circa 2002 bash versions,
# it can't.  This sets up the CDK VARS
source ocpi/ocpisetup.sh ocpi/ocpisetup.sh
if test "$OPENCL_EXPORTS" = ""; then
  export OPENCL_EXPORTS="$OCPI_OPENCL_INCLUDE_DIR $OCPI_OPENCL_INCLUDE_DIR/CL"
fi
echo ""; echo " *** OpenCPI Environment settings"; echo ""
env | grep OCPI_ | sort
