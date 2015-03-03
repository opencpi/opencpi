trap "trap - ERR; break" ERR; for i in 1; do
source env/start.sh

export OCPI_XILINX_DIR=/opt/Xilinx
export OCPI_XILINX_VERSION=14.7
export OCPI_XILINX_LICENSE_FILE=/home/user/Xilinx-License.lic

source platforms/zed/zed-env.sh

# #### Location of the Modelsim tools if you have them ####################################### #

export OCPI_MODELSIM_DIR=/opt/Mentor/modelsim_dlx
export OCPI_MODELSIM_LICENSE_FILE=$OCPI_MODELSIM_DIR/Site_1234567.txt

source env/finish.sh
done; trap - ERR
