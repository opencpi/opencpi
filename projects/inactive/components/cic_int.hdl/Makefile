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

# Put Makefile customizations for worker cic_int.hdl here:

# Either a {worker}.build file is used or the following
# "ParamValues_*" are used to describe the build configurations
#ParamValues_N=4
#ParamValues_M=1
#ParamValues_R=2048
#ParamValues_DIN_WIDTH=16
# >= ceil(N*log2(RM))+DIN_WIDTH
#ParamValues_ACC_WIDTH=60
#ParamValues_DOUT_WIDTH=16

# Vivado:
# The .ltx file under vivado_ila/ is the debug probe information that
# can be loaded in Vivado's Logic Analyzer
#
# When debugging with Vivado, we recommend that you use the following option
# so that the probe names are readable in the logic analyzer:
# VivadoExtraOptions_synth=" -flatten_hierarchy none "
#
# Vivado option 1:
# Use this EDIF netlist along with its stub file for debugging with Vivado
# Set VIVADO_ILA_p to true and uncomment the 'Cores=' line below to enable 
# the debug functionality in the worker VHDL.
SourceFiles=./vivado_ila/vivado_ila_stub.vhd
#Cores=./vivado_ila/vivado_ila.edf
# Note: This Vivado stub file has '.vhdl' suffix by default. It was renamed
# to '.vhd' because otherwise ISE could not compile it.

# Vivado option 2:
# Use this DCP file (no stub needed) for debugging with Vivado
# Set VIVADO_ILA_p to true and uncomment the 'Cores=' line below to enable 
# the debug functionality in the worker VHDL
#Cores=./vivado_ila/vivado_ila.dcp

# Legacy option:
# Use these legacy files with for debugging with Chipscope
# Set CHIPSCOPE_p set to true and uncomment the lines below to enable the 
# debug functionality in the worker VHDL
#SourceFiles=./chipscope/chipscope_icon.vhd ./chipscope/chipscope_ila_trig_n_data.vhd
#Cores=./chipscope/chipscope_icon.ngc ./chipscope/chipscope_ila_trig_n_data.ngc


# During the build process, swap HdlTarget with 'actual' target part of the target platform
# We do this to build for a part with the correct number of resources, such as hardware
# multipliers, rather than the default smallest part in the family
# zynq => xc7z020 (ZedBoard, Matchstiq-Z1, Picoflexor-T6A)
# virtex6 => xc6vlx240t (ml605 development board)
# stratix4 => ep4sgx230k (Stratix4 development board)
HdlExactPart=\
	$(or $(and $(filter zynq_ise,$(HdlTarget)),xc7z020_ise_alias-1-clg484), \
	$(and $(filter zynq,$(HdlTarget)),xc7z020-1-clg484), \
	$(and $(filter virtex6,$(HdlTarget)),xc6vlx240t-1-ff1156), \
	$(and $(filter stratix4,$(HdlTarget)),ep4sgx230k-c2-f40))

include $(OCPI_CDK_DIR)/include/worker.mk
