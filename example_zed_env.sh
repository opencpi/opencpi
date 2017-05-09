trap "trap - ERR; break" ERR; for i in 1; do
source env/start.sh

#fully qualified location to the Xilinx installation
#if /opt/Xilinx/14.7/ISE_DS/ is the location then
export OCPI_XILINX_DIR= #/opt/Xilinx
export OCPI_XILINX_VERSION= #14.7
export OCPI_XILINX_LICENSE_FILE= #$OCPI_XILINX_DIR/OCPI_XILINX_VERSION/License.lic

OCPI_TARGET_PLATFORM=xilinx13_4

# #### Location of the Modelsim tools ####################################### #

#fully qualified location to the ModelSim installation
#if /opt/Modelsim/modelsim_dlx/bin
export OCPI_MODELSIM_DIR= #/opt/Modelsim/modelsim_dlx
export OCPI_MODELSIM_LICENSE_FILE= #$OCPI_MODELSIM_DIR/../license.txt

source env/finish.sh
done; trap - ERR
