# Post processing after custom env setup
export OCPI_OUT_DIR=target-$OCPI_TARGET_HOST
# break out the target spec if not done
if test "$OCPI_TARGET_OS" = ""; then
  tgt=(`echo $OCPI_TARGET_HOST | sed 's/-/ /g'`)
  export OCPI_TARGET_OS=${tgt[0]}
  export OCPI_TARGET_OS_VERSION=${tgt[1]}
  export OCPI_TARGET_ARCH=${tgt[2]}
fi
if test "$OCPI_OMNI_DIR" != ""; then
  export OCPI_OMNI_BIN_DIR=$OCPI_OMNI_DIR/$OCPI_TARGET_HOST/bin
  export OCPI_OMNI_IDL_DIR=$OCPI_OMNI_DIR/$OCPI_TARGET_HOST/share/idl/omniORB
  export OCPI_OMNI_LIBRARY_DIR=$OCPI_OMNI_DIR/$OCPI_TARGET_HOST/lib
  export OCPI_OMNI_INCLUDE_DIR=$OCPI_OMNI_DIR/$OCPI_TARGET_HOST/include
  export OCPI_CORBA_INCLUDE_DIRS="$OCPI_OMNI_INCLUDE_DIR $OCPI_OMNI_INCLUDE_DIR/omniORB4"
fi
if test "$OCPI_OPENCL_EXPORTS" = ""; then
  export OPENCL_EXPORTS="$OCPI_OPENCL_INCLUDE_DIR $OCPI_OPENCL_INCLUDE_DIR/CL"
fi
# compatibility
export OCPI_OS=$OCPI_TARGET_OS
export OCPI_ARCH=$OCPI_TARGET_ARCH
# For now this script needs to know where it is, and on some cira 2002 bash versions,
# it can't
. ocpi/ocpisetup.sh ocpi/ocpisetup.sh
echo ""; echo " *** OpenCPI Environment settings"; echo ""
env | grep OCPI_ | sort
