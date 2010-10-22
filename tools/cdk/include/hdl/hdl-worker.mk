
# #####
#
#  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
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

ifndef Target
$(error The "Target" variable has not been set.)
endif
include $(OCPI_CDK_DIR)/include/hdl/hdl.mk
# Makefile fragment for HDL workers

ImplSuffix=_impl.v
SkelSuffix=_skel.v
DefsSuffix=_defs.v
# BF:=.edif
# LinkBinary=cp $< $@
# Compile should ultimately be compile for synthesis, not simulation
# and we should have the separate sim build.
# for now this is indeed compile for sim, but really just a syntax check
#Compile=iverilog -Wall -Wextra $(IncludeDirs:%=-I%) $(IncludeDirs:%=-y%) $(VerilogIncludeDirs:%=-I%) -o $@ $<
# Issues:
# Compile a library of primitives
# Generate a stub with outputs tied.
# Build the component, referring to library for primtives lile BSV.
# lso says which libraries to search, and the lso mapping mentiones remote dirs for libraries.
# xst (-ifn foo.xst) 

# HDL workers are one per binary
ImplXmlFile=$(firstword $(ImplXmlFiles))

DefsFiles=$(foreach w,$(Workers),$(GeneratedDir)/$(w)$(DefsSuffix))
override VerilogIncludeDirs += $(IncludeDirs)

ifeq ($(HostTarget),Darwin-i386)
OBJ=.o
Compile=iverilog -Wall -Wextra $(VerilogIncludeDirs:%=-I%) $(OcpiLibraries:%=-y%/lib/hdl) -o $@ $<
else
ifndef Core
Core=$(Worker)
endif
include $(OCPI_CDK_DIR)/include/hdl/xst.mk
endif
include $(OCPI_CDK_DIR)/include/xxx-worker.mk

ArtifactFile=
$(BinaryFile): $(AuthoredSourceFiles) $(GeneratedSourceFiles) $(DefsFiles) $(ImplHeaderFiles)
	$(Compile)

ifdef LibDir

ifndef Application
all: $(LibDir)/$(ImplXmlFile)

$(LibDir)/$(ImplXmlFile):
	$(AT)echo Creating link from $(LibDir) to $(ImplXmlFile) to expose the $(CwdName) xml.
	$(AT)$(call MakeSymLink,$(ImplXmlFile),$(LibDir))

endif
endif

# Generate the stub files
ifdef GenDir
ifndef Application
$(GenDir)/$(Worker)$(SourceSuffix): $(DefsFiles)
	$(AT)echo Creating link from $(GenDir) to $(DefsFiles) to expose the stub for worker "$(Worker)".
	$(AT)$(call MakeSymLink2,$(DefsFiles),$(GenDir),$(Worker)$(SourceSuffix))

all: $(GenDir)/$(Worker)$(SourceSuffix)

endif
endif


$(patsubst %,$(TargetDir)/%$(OBJ),$(basename $(notdir $(AuthoredSourceFiles)))): \
	 $(TargetDir)/%$(OBJ): %.v

$(DefsFiles): $(GeneratedDir)/%$(DefsSuffix): %.xml | $(GeneratedDir)
	$(AT)echo Generating the definition file: $@
	$(AT)$(OcpiGen) -d $<

$(foreach w,$(Workers),$(TargetDir)/$(w)$(OBJ)): $(TargetDir)/%$(OBJ): $(GeneratedDir)/%$(DefsSuffix) $(Libraries)


