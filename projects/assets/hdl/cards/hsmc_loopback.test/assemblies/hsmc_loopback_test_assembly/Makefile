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

$(if $(realpath $(OCPI_CDK_DIR)),,\
  $(error The OCPI_CDK_DIR environment variable is not set correctly.))
# This is the HDL assembly Makefile for the "hsmc_alst4_b_debug_header_breakout" assembly
# The file "hsmc_alst4_b_debug_header_breakout.xml" defines the assembly.
# The default container for all assemblies is one that connects all external ports to
# the devices interconnect to communicate with software workers or other FPGAs. 
# If you want to connect external ports of the assembly to local devices on the platform,
# you must define container XML files, and mention them in a "Containers" variable here, e.g.:
# Containers=take_input_from_local_ADC
DefaultContainers=
Containers= \
	loopback_card_plugged_into_hsmc_port_a_and_port_b_container
include $(OCPI_CDK_DIR)/include/hdl/hdl-assembly.mk
