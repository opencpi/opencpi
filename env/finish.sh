if test "$OCPI_OMNI_DIR" != ""; then
  export OCPI_OMNI_BIN_DIR=$OCPI_OMNI_DIR/$OCPI_TARGET_HOST/bin
  export OCPI_OMNI_IDL_DIR=$OCPI_OMNI_DIR/$OCPI_TARGET_HOST/share/idl/omniORB
  export OCPI_OMNI_LIBRARY_DIR=$OCPI_OMNI_DIR/$OCPI_TARGET_HOST/lib
  export OCPI_OMNI_INCLUDE_DIR=$OCPI_OMNI_DIR/$OCPI_TARGET_HOST/include
  export OCPI_CORBA_INCLUDE_DIRS="$OCPI_OMNI_INCLUDE_DIR $OCPI_OMNI_INCLUDE_DIR/omniORB4"
fi
# If we are building the framework, and targeting ourselves, and have not set the tool mode,
# we make the tool mode track the target mode, i.e. we execute what we are building.
# FIXME: this transformation in the shell needs to be consistent with the one in
# makefiles.
if test "$OCPI_TARGET_HOST" = "$OCPI_TOOL_HOST" -a "${OCPI_TOOL_MODE+UNSET}" = ""; then
 case $OCPI_BUILD_SHARED_LIBRARIES$OCPI_DEBUG in
   00) OCPI_TOOL_MODE=so;;
   01) OCPI_TOOL_MODE=sd;;
   10) OCPI_TOOL_MODE=do;;
   11) OCPI_TOOL_MODE=dd;;
   *) echo "Invalid values for OCPI_BUILD_SHARED_LIBRARIES or OCPI_DEBUG"; return 1;;
 esac
fi
if test "$OCPI_PREREQUISITES_INSTALL_DIR" = ""; then
  export OCPI_PREREQUISITES_INSTALL_DIR=/opt/opencpi/prerequisites
fi
# For now this script needs to know where it is, and on some circa 2002 bash versions,
# it can't.  This sets up the CDK VARS
if [ ! -d exports ]; then
  # This is a bootstrapping issue.  The exports tree doesn't exist, but we need it
  # to at least exist enough to know where the CDK will be.
  echo No exports tree exists in this OpenCPI base directory.  Creating an initial one.
  ./scripts/makeExportLinks.sh $OCPI_TOOL_HOST ocpi_ xxx
fi
source exports/scripts/ocpisetup.sh exports/scripts/ocpisetup.sh
# compatibility
export OCPI_OS=$OCPI_TARGET_OS
export OCPI_ARCH=$OCPI_TARGET_ARCH
if test "$OPENCL_EXPORTS" = ""; then
  export OPENCL_EXPORTS="$OCPI_OPENCL_INCLUDE_DIR $OCPI_OPENCL_INCLUDE_DIR/CL"
fi
echo ""; echo " *** OpenCPI Environment settings"; echo ""
env | grep OCPI_ | sort
