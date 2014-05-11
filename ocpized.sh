. ./env/start.sh

export OCPI_XILINX_DIR=/home/jek/mac/Xilinx
export OCPI_XILINX_VERSION=14.6
export OCPI_XILINX_LICENSE_FILE=/home/jek/mac/Xilinx/Xilinx-License-VM.lic
. ./env/xilinx.sh

export OCPI_ALTERA_DIR=/home/jek/mac/Altera
export OCPI_ALTERA_VERSION=12.1
export OCPI_ALTERA_LICENSE_FILE=$OCPI_ALTERA_DIR/1-99VGI3_License.dat
. ./env/altera.sh

. ./env/zynq.sh

# #### Location of the Modelsim tools ####################################### #

export OCPI_MODELSIM_DIR=/home/jek/mac/Mentor/modelsim_dlx
export OCPI_MODELSIM_LICENSE_FILE=$OCPI_MODELSIM_DIR/../3113964_001C42EFC0F4.txt
#export OCPI_MODELSIM_LICENSE_FILE=$OCPI_MODELSIM_DIR/../james.non-server.lic.txt
#export OCPI_MODELSIM_LICENSE_FILE=$OCPI_MODELSIM_DIR/../3113964_multipleServers.txt
#export OCPI_MODELSIM_LICENSE_FILE=$OCPI_MODELSIM_DIR/../Site_3113964.txt

export OCPI_BUILD_SHARED_LIBRARIES=0
#export OCPI_OPENCV_HOME=/opt/opencpi/prerequisites/opencv/linux-c6-x86_64
export OCPI_OPENCV_HOME=

. ./env/finish.sh
