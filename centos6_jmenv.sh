trap "trap - ERR; break" ERR; for i in 1; do
. env/start.sh

# Build statically, which is sometimes preferable
export OCPI_BUILD_SHARED_LIBRARIES=0
. env/centos6.sh

# If install_opencv.sh has run successfully:
# export OCPI_OPENCV_HOME=/opt/opencpi/prerequisites/opencv/linux-c6-x86_64

export OCPI_LOG_LEVEL=8

CILKROOT=/opt/opencpi/prerequisites/cilk/cilkplus
export PATH=$CILKROOT/bin:$PATH
export LD_LIBRARY_PATH=$CILKROOT/lib64:$LD_LIBRARY_PATH

export OCPI_HAVE_OPENCL=1
export OCPI_OPENCL_OBJS=/home/jmiller/AMDAPPSDK-2.9-1/lib/x86_64/libOpenCL.so

export OCPI_SMB_SIZE=100000

export OCPI_HAVE_SCIF=1
export OCPI_SCIF_NODE=0
export OCPI_SCIF_PORT=2


. env/finish.sh
done; trap - ERR
