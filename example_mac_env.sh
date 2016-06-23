trap "trap - ERR; break" ERR; for i in 1; do
. ./env/start.sh

. ./platforms/macos10_11/macos10_11-env.sh
export OCPI_DYNAMIC=1
export OCPI_EXCLUDE_TARGETS=xilinx
export OCPI_HAVE_OPENCL=1
export OCPI_OPENCL_OBJS=/System/Library/Frameworks/OpenCL.framework/Versions/A/OpenCL
export OCPI_OPENCV_HOME=/opt/opencpi/prerequisites/opencv/macos-10.8-x86_64

. ./env/finish.sh
done; trap - ERR
