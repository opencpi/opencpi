# #####
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
############################################################################

# This primitive library contains VHDL primitive modules for zynq platforms.
#include $(OCPI_CDK_DIR)/include/util.mk
include $(OCPI_CDK_DIR)/include/hdl/xilinx.mk

OnlyTargets=zynq zynq_ise xsim isim modelsim
HdlLibraries=fixed_float ocpi platform axi

IseZynqTargets=zynq_ise
VivadoZynqTargets=zynq
# Vivado's core
# We assume a sub-version of ps7 version 5
# Older versions of vivado may use v4. These are not
# directly supported 
XilinxZynqCore_base=processing_system7_v5_
XilinxIpDir=$(call OcpiXilinxVivadoDir,infox)/data/ip/xilinx
# Look for the most recent version of ps7 in the vivado installation
XilinxZynqCore=$(notdir $(lastword $(wildcard $(XilinxIpDir)/$(XilinxZynqCore_base)*)))
XilinxZynq=$(call OcpiXilinxVivadoDir, infox)/data/ip/xilinx/$(XilinxZynqCore)/hdl/verilog
XilinxZynqPSFiles=atc aw_atc b_atc trace_buffer w_atc
# The Vivado processing_system7 v5_5 verilog file must be generated. 
# This is done via a make rule below.

# Extract the PS7 version to pass to TCL. Replace '_' with '.' for the tool
XilinxZynqPS7Version=$(subst _,.,$(subst processing_system7_v,,$(XilinxZynqCore)))

# ISE's core
XilinxZynq_iseCore=processing_system7_v4_03_a
XilinxZynq_ise=$(call OcpiXilinxIseEdkDir,infox)/hw/XilinxProcessorIPLib/pcores/$(XilinxZynq_iseCore)/hdl/verilog
# Since the wrapper file is verilog. we'll provide the verilog PS7
#XilinxPS7=$(OcpiXilinxToolsDir)/ISE/verilog/src/unisims/PS7.v
XilinxZynq_isePSFiles=atc aw_atc b_atc processing_system7 trace_buffer w_atc

#   sdp2axi.vhd sdp2axi_wd.vhd sdp2axi_rd.vhd

VivadoZynqSources=\
  vivado_zynq/zynq_pkg.vhd \
  $(XilinxZynqPSFiles:%=$(XilinxZynq)/$(XilinxZynqCore)_%.v) \
  vivado_zynq/$(XilinxZynqCore)_processing_system7.v \
  vivado_zynq/zynq_ps.vhd

IseZynqSources=$(XilinxZynq_isePSFiles:%=$(XilinxZynq_ise)/%.v) zynq_ps.vhd

# If we are building for Vivado, grab the files from the Vivado installation,
# along with vivado_zynq/zynq_p*
# Otherwise, include zynq_pkg.vhd, AND if we are building for ISE, grab the
# files from the ISE installation along with zynq_ps.pkg
# When building for ISE or Vivado, we have tool-specific sources. Otherwise, all tools
# can use generic sources.
HdlToolSpecificSources=$(if $(filter $(VivadoZynqTargets) $(IseZynqTargets),$(HdlTargets)),yes)

SourceFiles=\
  $(if $(filter $(VivadoZynqTargets),$(HdlTargets)),\
    $(VivadoZynqSources), \
    zynq_pkg.vhd \
    $(if $(filter $(IseZynqTargets),$(HdlTargets)),\
      $(IseZynqSources)))

ifeq ($(wildcard $(XilinxZynq)),)
  ifeq  ($(wildcard $(XilinxZynq_ise)),)
    $(warning Xilinx Zynq IP files are unavailable.  Assemblies for Zynq platforms will not build.)
  endif
endif

include $(OCPI_CDK_DIR)/include/hdl/hdl-lib.mk

# If we are building for zynq_ise, make sure ise's pcore is there
# Otherwise, we look for Vivado's pcore.
ifneq ($(filter $(IseZynqTargets),$(HdlTarget)),)
  ifeq ($(wildcard $(XilinxZynq_ise)),)
       $(error The Xilinx IP is missing at $(XilinxZynq_ise) when building for HdlTarget=zynq_ise)
  endif
else
  ifneq ($(filter $(VivadoZynqTargets),$(HdlTarget)),)
    ifeq ($(wildcard $(XilinxZynq)),)
      $(error The Xilinx IP is missing at $(XilinxZynq) when building for HdlTarget=zynq)
    endif
  endif
endif

# In order to generate the processing_system7 top verilog file with Vivado,
# we need to run a series of TCL commands located in vivado/tcl
vivado_zynq/$(XilinxZynqCore)_processing_system7.v:
	mkdir vivado_zynq/tmp ; \
	cd vivado_zynq/tmp ; \
	$(call OcpiXilinxVivadoInit); \
	vivado -mode batch -source ../ps7.tcl -tclargs $(XilinxZynqPS7Version) > ../ps7.log ;\
	cd ../ ; \
	cp tmp/managed_ip_project/managed_ip_project.srcs/sources_1/ip/processing_system7_0/hdl/verilog/$(XilinxZynqCore)_processing_system7.v .; \
	rm -rf tmp/;\
	cd ../;

# Do not remove ps7 top level verilog on clean
# This will save time
clean::
	rm -rf vivado_zynq/tmp; \
	rm -f vivado_zynq/ps7.log;\


