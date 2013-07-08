. ./generic_env.sh

# #### Location of the Xilinx tools ####################################### #

export OCPI_XILINX_TOOLS_DIR=/home/jek/mac/Xilinx/14.6/ISE_DS
export OCPI_VIVADO_TOOLS_DIR=/home/jek/mac/Xilinx/Vivado/2013.2
export OCPI_XILINX_LICENSE_FILE=/home/jek/mac/Xilinx/Xilinx-License-VM.lic

# #### Location of the Altera tools ####################################### #

export OCPI_ALTERA_TOOLS_DIR=/home/jek/mac/altera/12.1
export OCPI_ALTERA_LICENSE_FILE=/home/jek/mac/altera/1-9095JT_License.dat

# #### Location of the Modelsim tools ####################################### #

export OCPI_MODELSIM_DIR=/home/jek/mac/Mentor/modelsim_dlx
export OCPI_MODELSIM_LICENSE_FILE=$OCPI_MODELSIM_DIR/../james.non-server.lic.txt
#export OCPI_MODELSIM_LICENSE_FILE=$OCPI_MODELSIM_DIR/../3113964_multipleServers.txt
#export OCPI_MODELSIM_LICENSE_FILE=$OCPI_MODELSIM_DIR/../Site_3113964.txt

# #### Compiler linker flags ############################################## #

#export OCPI_CFLAGS+=" -m64"
export OCPI_CXXFLAGS+=" -Wno-dangling-else"
#export OCPI_LDFLAGS=-m64

# #### Shared library build settings ###################################### #

export OCPI_SHARED_LIBRARIES_FLAGS="-m64 -m elf_x86_64"

# Set to 0 to build static libraries
export OCPI_BUILD_SHARED_LIBRARIES=0

export OCPI_OPENCV_HOME=/opt/opencpi/prerequisites/opencv/linux-c6-x86_64

. ./generic_env_post.sh
