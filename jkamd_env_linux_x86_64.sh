trap "trap - ERR; break" ERR; for i in 1; do
. ./env/start.sh

export OCPI_HAVE_CORBA=1

# #### Location of the Xilinx tools ####################################### #

export OCPI_XILINX_DIR=/opt/Xilinx
export OCPI_XILINX_VERSION=14.6
export OCPI_VIVADO_TOOLS_DIR=/home/jek1/mac/Xilinx/Vivado/2012.1
. ./env/xilinx.sh

# #### Location of the Altera tools ####################################### #

export OCPI_ALTERA_DIR=/home/jim/Altera
export OCPI_ALTERA_VERSION=13.1
export OCPI_ALTERA_KITS_DIR=$OCPI_ALTERA_DIR/11.1/kits
. ./env/altera.sh
# License not needed - this is runtime only
#export OCPI_ALTERA_LICENSE_FILE=/home/jim/mac/altera/1-9095JT_License.dat

#export OCPI_CFLAGS=-m64
#export OCPI_CXXFLAGS=-m64
#export OCPI_LDFLAGS=-m64

# #### Shared library build settings ###################################### #

export OCPI_SHARED_LIBRARIES_FLAGS="-m64 -m elf_x86_64"

# Set to 0 to build static libraries
export OCPI_BUILD_SHARED_LIBRARIES=1
. ./env/redhat5.sh


# export OCPI_HAVE_OPENSPLICE=1
# export OCPI_OPENSPLICE_HOME=/opt/opencpi/prerequisites/opensplice/linux-x86_64

. ./env/finish.sh
done; trap - ERR
