export OCPI_BASE_DIR=`pwd`
export OCPI_CDK_DIR=$OCPI_BASE_DIR/ocpi
if test "$OCPI_TOOL_HOST" = ""; then
  ovp=(`tools/cdk/scripts/showRuntimeHost`)
  if test "$ovp" = "" -o "${#ovp[*]}" != 3; then
    echo Error determining run time host.  1>&2
    exit 1
  fi
  export OCPI_TOOL_OS=${ovp[0]}
  export OCPI_TOOL_OS_VERSION=${ovp[1]}
  export OCPI_TOOL_ARCH=${ovp[2]}
  export OCPI_TOOL_HOST=$OCPI_TOOL_OS-$OCPI_TOOL_OS_VERSION-$OCPI_TOOL_ARCH
fi
#default the target host to the tool host
export OCPI_TARGET_HOST=$OCPI_TOOL_HOST

export OCPI_GTEST_DIR=/opt/opencpi/prerequisites/gtest
export OCPI_DEBUG=1
export OCPI_ASSERT=1
export OCPI_SHARED_LIBRARIES_FLAGS=
export OCPI_BUILD_SHARED_LIBRARIES=1
export OCPI_HAVE_CORBA=1

# OpenCPI uses OmniORB exclusivly
export OCPI_CORBA_ORB=OMNI
export OCPI_OMNI_DIR=/opt/opencpi/prerequisites/omniorb

# Set this to "1" to include the OFED IBVERBS transfer driver (for linking)
export OCPI_HAVE_IBVERBS=

# ##########OpenCL - default is that we have headers to compile against
export OCPI_OPENCL_INCLUDE_DIR=$OCPI_BASE_DIR/core/container/ocl/include
export OCPI_OPENCL_OBJS=
export OCPI_HAVE_OPENCL=


export OCPI_LIBRARY_PATH=$OCPI_BASE_DIR/components/lib/rcc
export OCPI_SMB_SIZE=100000000

# #########  OpenCV 
#export OCPI_OPENCV_HOME=/opt/opencpi/prerequisites/opencv/darwin-x86_64
# suppress execution while allowing building

# ######### OpenSplice

#export OCPI_HAVE_OPENSPLICE=1
#export OCPI_OPENSPLICE_HOME=/opt/opencpi/prerequisites/opensplice/linux-x86_64
# temporarily remove this

