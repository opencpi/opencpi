. ./env/start.sh

export OCPI_HAVE_CORBA=1
export OCPI_EXCLUDE_TARGETS=xilinx
export OCPI_VERILATOR_DIR=/opt/opencpi/prerequisites/verilator/$OCPI_BUILD_HOST
export OCPI_ICARUS_DIR=/opt/opencpi/prerequisites/icarus/$OCPI_BUILD_HOST
export OCPI_CXXFLAGS+=" -Wno-sign-conversion"
# replace default with our custom version
#export OCPI_OMNI_DIR=/Users/jek/sw/omniORB-4.1.5-ocpi/OCPI_INSTALL
#export OPENCL_INCLUDE_DIR=/usr/local/share/NVIDIA_GPU_Computing_SDK/OpenCL/common/inc
#export OCPI_HAVE_OPENCL=1
#export OCPI_OPENCL_OBJS=/System/Library/Frameworks/OpenCL.framework/Versions/A/OpenCL
export OCPI_OPENCV_HOME=/opt/opencpi/prerequisites/opencv/macos-10.8-x86_64
# suppress execution while allowing building
#export OCPI_HAVE_OPENSPLICE=1
#export OCPI_OPENSPLICE_HOME=/opt/opencpi/prerequisites/opensplice/linux-x86_64

. ./env/finish.sh
