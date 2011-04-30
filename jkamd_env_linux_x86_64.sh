
# #####
#
#  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2011
#
#    Mercury Federal Systems, Incorporated
#    1901 South Bell Street
#    Suite 402
#    Arlington, Virginia 22202
#    United States of America
#    Telephone 703-413-0781
#    FAX 703-413-0784
#
#  This file is part of OpenCPI (www.opencpi.org).
#     ____                   __________   ____
#    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
#   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
#  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
#  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
#      /_/                                             /____/
#
#  OpenCPI is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OpenCPI is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################### #


# Build from 32-bit x86 Linux for Linux

# #### Absolute path to the base directory of the OpenCPI installation #### #

if [ -z "$OCPI_BASE_DIR" ]
then
  export OCPI_BASE_DIR=`pwd`
fi

if [ -z "$OCPI_CDK_DIR" ]
then
  export OCPI_CDK_DIR=$OCPI_BASE_DIR/ocpi
fi

# #### Build target architecture and OS ################################### #

export OCPI_OS=linux
export OCPI_ARCH=x86_64
export OCPI_BUILD_HOST=linux-x86_64
export OCPI_RUNTIME_HOST=$OCPI_BUILD_HOST

# #### Location of the Xilinx tools ####################################### #

export OCPI_XILINX_TOOLS_DIR=/opt/Xilinx/13.1/ISE_DS

# #### Location of Google Test (gtest) #################################### #

export OCPI_GTEST_DIR=/opt/opencpi/linux-x86_64/prerequisites/gtest

# #### Location of Verilator #################################### #

export OCPI_VERILATOR_DIR=/opt/opencpi/prerequisites/verilator/linux-x86_64

# #### Location of Icarus #################################### #

export OCPI_ICARUS_DIR=/opt/opencpi/prerequisites/icarus/linux-x86_64
# #### Build output location ############################################## #

export OCPI_OUT_DIR=$OCPI_OS-$OCPI_ARCH-bin

export LD_LIBRARY_PATH=$OCPI_BASE_DIR/lib/$OCPI_BUILD_HOST-bin:$OCPI_GTEST_DIR/lib:$LD_LIBRARY_PATH

# #### Compiler linker flags ############################################## #

export OCPI_CFLAGS=-m64
export OCPI_CXXFLAGS=-m64
export OCPI_LDFLAGS=-m64

# #### Debug and assert settings ########################################## #

# Change both to 0 for "release" build

export OCPI_DEBUG=1
export OCPI_ASSERT=1

# #### Shared library build settings ###################################### #

export OCPI_SHARED_LIBRARIES_FLAGS="-m64 -m elf_x86_64"

# Set to 0 to build static libraries
export OCPI_BUILD_SHARED_LIBRARIES=1

# #### CORBA OCPI_CORBA_ORB/IDL tools ################################################ #

export HAVE_CORBA=1

# OpenCPI uses OmniORB exclusivly
export OCPI_CORBA_ORB=OMNI
export OCPI_OMNI_DIR=/usr/local
export OCPI_OMNI_BIN_DIR=$OCPI_OMNI_DIR/bin
export OCPI_OMNI_IDL_DIR=$OCPI_OMNI_DIR/share/idl/omniORB
export OCPI_OMNI_LIBRARY_DIR=$OCPI_OMNI_DIR/lib
export OCPI_OMNI_INCLUDE_DIR=$OCPI_OMNI_DIR/include/omniORB4

# #### Path to Mercury tools and libraries ################################ #

export OCPI_PPP_LIBRARY_DIR=
export OCPI_PPP_INCLUDE_DIR=

# #### Other settings ##################################################### #

# Set this to "1" to include the OFED IBVERBS transfer driver
export OCPI_HAVE_IBVERBS=0

# ######################################################################### #

echo ""; echo " *** OpenCPI Environment settings"; echo ""
env | grep OCPI_
