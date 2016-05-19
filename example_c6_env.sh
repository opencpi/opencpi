trap "trap - ERR; break" ERR; for i in 1; do
. ./env/start.sh

#fully qualified location to the Xilinx installation
#if /opt/Xilinx/14.7/ISE_DS/ is the location then
export OCPI_XILINX_DIR= #/opt/Xilinx
export OCPI_XILINX_VERSION= #14.7
export OCPI_XILINX_LICENSE_FILE= #$OCPI_XILINX_DIR/OCPI_XILINX_VERSION/License.lic
. ./env/xilinx.sh

#fully qualified location to the Altera installation
#if /opt/Altera/15.1
export OCPI_ALTERA_DIR= #/opt/Altera
export OCPI_ALTERA_VERSION= #15.1
export OCPI_ALTERA_LICENSE_FILE= #$OCPI_ALTERA_DIR/License.dat
. ./env/altera.sh

export OCPI_BUILD_SHARED_LIBRARIES=0
. ./platforms/centos6/centos6-env.sh

# #### Location of the Modelsim tools ####################################### #

#fully qualified location to the ModelSim installation
#if /opt/Modelsim/modelsim_dlx/bin
export OCPI_MODELSIM_DIR= #/opt/Modelsim/modelsim_dlx
export OCPI_MODELSIM_LICENSE_FILE= #$OCPI_MODELSIM_DIR/../license.txt

# Enable this if you want to, and have, installed OPENCV
# export OCPI_OPENCV_HOME=/opt/opencpi/prerequisites/opencv/linux-c6-x86_64

. ./env/finish.sh
done; trap - ERR
