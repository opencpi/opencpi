trap "trap - ERR; break" ERR; for i in 1; do
source env/start.sh

export OCPI_XILINX_DIR=/home/jek/mac/Xilinx
export OCPI_XILINX_VERSION=14.6
export OCPI_XILINX_LICENSE_FILE=/home/jek/mac/Xilinx/Xilinx-License.lic

source platforms/zed/zed-env.sh

# #### Location of the Modelsim tools ####################################### #

export OCPI_MODELSIM_DIR=/home/jek/mac/Mentor/modelsim_dlx
export OCPI_MODELSIM_LICENSE_FILE=$OCPI_MODELSIM_DIR/../license-20151005.txt

source env/finish.sh
done; trap - ERR
