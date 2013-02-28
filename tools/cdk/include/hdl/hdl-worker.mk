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
$(call OcpiDbg,Entering hdl-worker.mk)
include $(OCPI_CDK_DIR)/include/hdl/hdl-pre.mk
ifeq ($(MAKECMDGOALS),skeleton)
  HdlSkip:=
endif
ifdef HdlSkip
$(call OcpiDbg, Skipping)
else
Compile=$(HdlCompile)
$(call OcpiDbgVar,HdlBin)
BF:=$(HdlBin)
# This could change someday if any hdl tools have separate object files.
OBJ:=$(HdlBin)
# We don't build independent standalone worker binaries (no artifact file here)
ArtifactFile=

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
# This is REDUNDANT with what is in xxx-worker.mk, but we need it to figure out the language below.
ifndef Worker
Worker=$(CwdName)
Workers=$(Worker)
endif

############
# We need to figure out the language and the file suffixes before calling xxx-worker
# 
ifeq ($(Worker_$(Worker)_xml),)
Worker_$(Worker)_xml=$(Worker).xml
HdlXmlFile=$(Worker).xml
endif
HdlXmlFile=$(Worker_$(Worker)_xml)
$(call OcpiDbgVar,HdlXmlFile)
$(call OcpiDbgVar,Worker)
$(call OcpiDbgVar,Worker_$(Worker)_xml)

#ifeq ($(realpath $(HdlXmlFile)),)
#  $(error Missing XML implementation file: $(HdlXmlFile))
#endif
ifdef Language
HdlLanguage:=$(call ToLower,$(Language))
endif
ifndef HdlLanguage
# Ugly grab of the language attribute from the XML file
HdlLanguage:=$(shell grep -i 'language *=' $(HdlXmlFile) | sed "s/^.*[lL]anguage= *['\"]\\([^\"']*\\).*$$/\1/" | tr A-Z a-z)
endif
#$(info HdlLanguage is $(HdlLanguage))
ifeq ($(HdlLanguage),)
HdlLanguage:=vhdl
else
HdlLanguage:=$(call ToLower,$(HdlLanguage))
endif
ifeq ($(HdlLanguage),verilog)
HdlSourceSuffix:=$(HdlVerilogSuffix)
HdlIncSuffix:=$(HdlVerilogIncSuffix)
else
HdlSourceSuffix:=$(HdlVHDLSuffix)
HdlIncSuffix:=$(HdlVHDLIncSuffix)
endif
ifdef sdf
# The ocpi library is required for VHDL
$(call OcpiDbgVar,HdlLibraries,Add ocpi library perhaps)
# FIXME: when tools don't really elaborate or search, capture the needed libraries for later
# FIXME: but that still means a flat library space...
HdlLibrariesInternal = \
  $(foreach l,$(call Unique,\
                $(HdlLibraries) \
                $(foreach f,$(call HdlGetFamily,$(HdlTarget)),\
	          $(foreach p,ocpi util_$f util bsv vendor_$f vendor_$(call HdlGetTop,$f), \
	              $(and $(wildcard $(call HdlLibraryRefDir,$p,$f)),$p)))),$(strip \
    $(info Hdl Library is $l)$l))

$(call OcpiDbgVar,HdlLibraries,After adding ocpi if not there )
endif

include $(OCPI_CDK_DIR)/include/xxx-worker.mk
$(call OcpiDbgVar,Worker)
ifndef Core
Core=$(Worker)
endif

# This is the utility program for hdl
ifeq ($(shell if test -x $(ToolsDir)/ocpihdl; then echo xx; fi),)
ifneq ($(MAKECMDGOALS),clean)
$(error Missing ocpihdl utility program)
endif
endif
OcpiHdl=\
  $(DYN_PREFIX) $(ToolsDir)/ocpihdl 

################################################################################
# Generated files: impl depends on defs, worker depends on impl
# map the generic "IncludeDirs" into the verilog
override VerilogIncludeDirs += $(IncludeDirs)
ImplXmlFile=$(firstword $(ImplXmlFiles))
#$(HdlDefsSuffix))
RefDefsFile=$(Workers:%=$(GeneratedDir)/%_defs.vh)
DefsFile=$(Workers:%=$(GeneratedDir)/%$(HdlDefsSuffix))

$(sort $(RefDefsFile) $(DefsFile)): $(Worker_$(Worker)_xml) | $(GeneratedDir)
	$(AT)echo Generating the definition file: $@
	$(AT)$(OcpiGen) -w $<
	$(AT)$(OcpiGen) -d $<

$(ImplHeaderFiles): $(DefsFile)

ifeq ($(HdlLanguage),vhdl)
  CompiledSourceFiles:=$(DefsFile) $(ImplHeaderFiles) $(CompiledSourceFiles)
  LibName=$(Worker)
endif

################################################################################
# Include this to build the core or the library
include $(OCPI_CDK_DIR)/include/hdl/hdl-core2.mk


################################################################################
# If not an assembly or container, we have to contribute to the exports for the
# component library we are a part of.
ifneq ($(HdlMode),assembly)
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
#$(HdlSourceSuffix))
$(call OcpiDbgVar,DefsFiles)
$(GenDir)/$(Worker)$(HdlVerilogSuffix): $(RefDefsFile) | $(GenDir)
	$(AT)echo Creating link from $(GenDir) to $(RefDefsFile) to expose the stub for worker "$(Worker)" .
	$(AT)$(call MakeSymLink2,$(RefDefsFile),$(GenDir),$(Worker)$(HdlVerilogSuffix))

$(call OcpiDbg,Before all: "$(GenDir)/$(Worker)$(HdlSourceSuffix)")
all: $(GenDir)/$(Worker)$(HdlVerilogSuffix)
endif
endif # if not an assembly

endif # HdlSkip
