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

# This file has the HDL tool details for verilator
# Verilator has no real ability to pre-compile into something usable
# at any higher level, although it begs for it since it is so completely
# designed for "compile-for-sim", and generates C++ classes per module
# For the time being we will use modular building only for modular syntax
# checking, and the exported/installed libraries will sadly be copies of
# source code.

ifndef OCPI_VERILATOR_DIR
OCPI_VERILATOR_DIR:=/opt/opencpi/prerequisites/verilator/$(HostTarget)
endif
$(call OcpiDbgVar,$(OCPI_VERILATOR_DIR))

################################################################################
# $(call HdlToolLibraryFile,target,libname)
# Function required by toolset: return the file to use as the file that gets
# built when the library is built.
# In verilator the result is a library directory full of links
# So there not a specific file name we can look for
HdlToolLibraryFile=$2
################################################################################
# Function required by toolset: given a list of targets for this tool set
# Reduce it to the set of library targets.
HdlToolLibraryTargets=verilator
################################################################################
# Variable required by toolset: HdlBin
# What suffix to give to the binary file result of building a core
# Note we can't build cores for further building, only simulatable "tops"
HdlBin=
################################################################################
# Variable required by toolset: HdlToolRealCore
# Set if the tool can build a real "core" file when building a core
# I.e. it builds a singular binary file that can be used in upper builds.
# If not set, it implies that only a library containing the implementation is
# possible
HdlToolRealCore=
################################################################################
# Variable required by toolset: HdlToolNeedBB=yes
# Set if the tool set requires a black-box library to access a core
HdlToolNeedBB=
################################################################################
# Function required by toolset: $(call HdlToolLibRef,libname)
# This is the name after library name in a path
# It might adjust (genericize?) the target
HdlToolLibRef=$(HdlTarget)

ifeq ($(HdlMode),library)
CompiledSourceFiles:= $(CompiledSourceFiles) $(OCPI_CDK_DIR)/include/hdl/onewire.v
MyTop=onewire
else
MyTop=$(Core)
endif
HdlToolFiles=\
  $(foreach f,$(HdlSources),\
     $(call FindRelative,$(TargetDir),$(dir $(f)))/$(notdir $(f)))
# The verilator script requires it being in your path...
Verilator=PATH=$(OCPI_VERILATOR_DIR)/bin:$$PATH;$(OCPI_VERILATOR_DIR)/bin/verilator
# This is just a syntax check for now
VWarnings=-Wall -Wno-STMTDLY -Wno-DECLFILENAME -Wno-UNUSED --error-limit 100
MyLibs=\
 $(foreach l,$(SimLibraries),-y $(l)) \
 $(foreach l,$(HdlLibrariesInternal),\
     -y $(call FindRelative,$(TargetDir),$(call HdlLibraryRefDir,$(l),verilator,,verilator)))
#VWarnings=-Wall -debug -debugi 10
MyIncs=$(foreach d,$(VerilogIncludeDirs),-I$(call FindRelative,$(TargetDir),$(d)))
HdlToolCompile=\
 $(Verilator) -cc --top-module $(MyTop) -Mdir . $(VWarnings) $(MyLibs) $(MyIncs) \
    $(HdlToolFiles)

$(eval $(call HdlSimNoLibraries))


