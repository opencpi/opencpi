trap "trap - ERR; break" ERR; for i in 1; do
export OCPI_EXPORTS_VERBOSE=1
. ./env/start.sh

export OCPI_EXCLUDE_TARGETS=xilinx
export OCPI_HAVE_OPENCL=1
export OCPI_OPENCL_OBJS=/System/Library/Frameworks/OpenCL.framework/Versions/A/OpenCL
export OCPI_OPENCV_HOME=/opt/opencpi/prerequisites/opencv/macos-10.8-x86_64

. ./env/finish.sh
done; trap - ERR
