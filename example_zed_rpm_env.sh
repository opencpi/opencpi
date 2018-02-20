# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

####################################################################################
## If using this script to initialize the base project, uncomment the following
## environment variable and set the location of the base project.
## If using this script in a project, uncomment the following environment variable
## and set the location of the base project and any project dependencies.
#
#export OCPI_PROJECT_PATH= 

####################################################################################
## Xilinx
## If using Xilinx tools and hardware, uncomment the following lines
## and set the location of the license file.
#
#export OCPI_XILINX_LICENSE_FILE=
## Insert additional variables here if not using the default tool paths,
## see documentation for details.
#. ${OCPI_CDK_DIR}/env/xilinx.sh


####################################################################################
## Cross-Building
## If using both RCC and HDL workers, uncomment the following lines and set to "zed"
## as seen below. Whenever non-cross compiling "unset" the following variable.
#
#export OCPI_TARGET_PLATFORM=zed

