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

# Makefile fragment for HDL workers

ifndef HdlMode
HdlMode:=worker
endif
include $(OCPI_CDK_DIR)/include/hdl/hdl-pre.mk
ifdef HdlSkip
$(call OcpiDbg, Skipping)
else
HdlVerilogSuffix:=.v
HdlVerilogIncSuffix:=.vh
HdlVHDLSuffix:=.vhd
HdlVHDLIncSuffix:=.vhd
HdlSkelSuffix=_skel$(HdlSourceSuffix)
HdlDefsSuffix=_defs$(HdlIncSuffix)
HdlImplSuffix=_impl$(HdlIncSuffix)
ifneq ($(word 2,$(Workers)),)
$(error Only one HDL worker can be built.  Workers is: $(Workers))
endif
ifndef Core
Core=$(Worker)
endif

Compile=$(HdlCompile)
$(call OcpiDbgVar,HdlBin)
BF:=$(HdlBin)
# This could change someday if any hdl tools have separate object files.
OBJ:=$(HdlBin)
# We don't build independent standalone worker binaries (no artifact file here)
ArtifactFile=
############
# We need to figure out the language and the file suffixes before calling www-worker
# 
HdlXmlFile=$(Worker).xml
ifeq ($(realpath $(HdlXmlFile)),)
  $(error Missing XML implementation file: $(HdlXmlFile))
endif
# Ugly grab of the language attribute from the XML file
HdlLanguage=$(shell grep -i 'language *=' $(HdlXmlFile) | sed "s/^.*[lL]anguage= *['\"]\\([^\"']*\\).*$$/\1/" | tr A-Z a-z)
$(info HdlLanguage is $(HdlLanguage))
ifeq ($(HdlLanguage),)
HdlLanguage=verilog
endif
ifeq ($(HdlLanguage),verilog)
HdlSourceSuffix:=$(HdlVerilogSuffix)
HdlIncSuffix:=$(HdlVerilogIncSuffix)
else
HdlSourceSuffix:=$(HdlVHDLSuffix)
HdlIncSuffix:=$(HdlVHDLIncSuffix)
endif

include $(OCPI_CDK_DIR)/include/xxx-worker.mk
$(call OcpiDbgVar,Worker)
################################################################################
# Generated files: impl depends on defs, worker depends on impl
# map the generic "IncludeDirs" into the verilog
override VerilogIncludeDirs += $(IncludeDirs)
DefsFile=$(Workers:%=$(GeneratedDir)/%$(HdlDefsSuffix))
$(DefsFile): $(Worker).xml | $(GeneratedDir)
	$(AT)echo Generating the definition file: $@
	$(AT)$(OcpiGen) -d $<

$(ImplHeaderFiles): $(DefsFile)

################################################################################
# Include this to build the core or the library
include $(OCPI_CDK_DIR)/include/hdl/hdl-core2.mk


################################################################################
# If not an application, we have to contribute to the exports for the library
# we are a part of.
ifndef Application
# Expose the implementation xml file for apps that instantiate this worker core
ifdef LibDir
$(call OcpiDbg,Before all: "$(LibDir)/$(ImplXmlFile)")

all: $(LibDir)/$(ImplXmlFile)

$(LibDir)/$(ImplXmlFile): | $(LibDir)
	$(AT)echo Creating a link from $(LibDir) to $(ImplXmlFile) to expose the $(CwdName) xml.
	$(AT)$(call MakeSymLink,$(ImplXmlFile),$(LibDir))
endif

ifdef GenDir
$(GenDir):
	mkdir $(GenDir)
# Generate the stub files by providing a link from gen/worker.v to gen/worker_defs.v
$(call OcpiDbgVar,DefsFiles)
$(GenDir)/$(Worker)$(HdlSourceSuffix): $(DefsFile) | $(GenDir)
	$(AT)echo Creating link from $(GenDir) to $(DefsFile) to expose the stub for worker "$(Worker)" "$@".
	$(AT)$(call MakeSymLink2,$(DefsFile),$(GenDir),$(Worker)$(HdlSourceSuffix))

$(call OcpiDbg,Before all: "$(GenDir)/$(Worker)$(HdlSourceSuffix)")
all: $(GenDir)/$(Worker)$(HdlSourceSuffix)
endif
endif # if not an application

endif # HdlSkip
