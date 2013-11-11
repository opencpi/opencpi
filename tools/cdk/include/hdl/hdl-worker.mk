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

#HdlVerilogSuffix:=.v
#HdlVerilogIncSuffix:=.vh
#HdlVHDLSuffix:=.vhd
#HdlVHDLIncSuffix:=.vhd
HdlSkelSuffix=-skel$(HdlSourceSuffix)
HdlDefsSuffix=-defs$(HdlIncSuffix)
HdlOtherDefsSuffix=-defs$(HdlOtherIncSuffix)
HdlImplSuffix=-impl$(HdlIncSuffix)
HdlOtherImplSuffix=-impl$(HdlOtherIncSuffix)
#ifneq ($(word 2,$(Workers)),)
#$(error Only one HDL worker can be built.  Workers is: $(Workers))
#endif
# This is REDUNDANT with what is in xxx-worker.mk, but we need it to figure out the language below.
#ifndef Worker
#Worker=$(CwdName)
#Workers=$(Worker)
#endif

############
# We need to figure out the language and the file suffixes before calling xxx-worker
# 
#ifeq ($(Worker_$(Worker)_xml),)
#Worker_$(Worker)_xml=$(Worker).xml
#HdlXmlFile=$(Worker).xml
#endif
#HdlXmlFile=$(Worker_$(Worker)_xml)
#$(call OcpiDbgVar,HdlXmlFile)
#$(call OcpiDbgVar,Worker)
#$(call OcpiDbgVar,Worker_$(Worker)_xml)

#ifeq ($(realpath $(HdlXmlFile)),)
#  $(error Missing XML implementation file: $(HdlXmlFile))
#endif
#ifndef HdlLanguage
#  ifeq ($(HdlMode),assemblyxxxx)
#    HdlLanguage:=verilog
#  else
#    # Ugly grab of the language attribute from the XML file
#    HdlLanguage:=$(and $(wildcard $(HdlXmlFile)),$(shell grep -i 'language *=' $(HdlXmlFile) | sed "s/###########^.*[lL]anguage= *['\"]\\([^\"']*\\).*$$/\1/" | tr A-Z a-z))
#    ifdef Language
#      ifdef HdlLanguage
#        ifneq ($(call ToLower,$(Language)),$(HdlLanguage))
#          $(error The "Language" setting in the Makefile ($(Language)) is inconsistent with the settin#g in the XML/OWD file (file: $(HdlXmlFile), setting: $(HdlLanguage)))
#        endif # error check
#      else
#        HdlLanguage:= $(call ToLower,$(Language))
#      endif # found language attribute
#    else
#      ifndef HdlLanguage
#        HdlLanguage:=vhdl
#      endif
#    endif
#  endif
#endif # HdlLanguage not initially defined (probably true)
#
#$(call OcpiDbgVar,HdlLanguage)
#ifeq ($(HdlLanguage),verilog)
#HdlSourceSuffix:=$(HdlVerilogSuffix)
#HdlIncSuffix:=$(HdlVerilogIncSuffix)
#else
#HdlSourceSuffix:=$(HdlVHDLSuffix)
#HdlIncSuffix:=$(HdlVHDLIncSuffix)
#endif
#$(call OcpiDbgVar,HdlSourceSuffix)

ifndef Tops
  ifdef Top
    Tops:=$(Top)
  else ifeq ($(HdlLanguage),vhdl)
    Tops=$(Worker) $(Worker)_rv
  else
    Tops=$(Worker) $(Worker)_rv
  endif
endif
HdlCores=$(Tops)

ifndef Core
Core=$(Worker)
endif

ifdef HdlToolRealCore
WkrExportNames=$(Tops:%=%$(BF))
endif
$(call OcpiDbgVar,Top)
$(call OcpiDbgVar,Tops)
$(call OcpiDbgVar,HdlCores)
$(call OcpiDbgVar,Core)
$(call OcpiDbgVar,WkrExportNames)
include $(OCPI_CDK_DIR)/include/xxx-worker.mk

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
#RefDefsFile=$(Workers:%=$(GeneratedDir)/%-defs.vh)
DefsFile=$(Workers:%=$(GeneratedDir)/%$(HdlDefsSuffix))
WDefsFile=$(Workers:%=$(GeneratedDir)/%$(HdlOtherDefsSuffix))
HdlOtherImplSourceFile=$(GeneratedDir)/$(Worker)$(HdlOtherImplSuffix)
# We set these, but they might not be used in some modes.
CoreBlackBoxFiles=$(DefsFile) $(WDefsFile)

$(WDefsFile): $(Worker_$(Worker)_xml) | $(GeneratedDir)
	$(AT)echo Generating the opposite language definition file: $@
	$(AT)$(OcpiGen) -D $(GeneratedDir) $(and $(Package),-p $(Package))  \
	  $(and $(HdlPlatform),-P $(HdlPlatform)) \
	  $(if $(Libraries),$(foreach l,$(Libraries),-l $l)) \
	  -w -d $<

$(DefsFile): $(Worker_$(Worker)_xml) | $(GeneratedDir)
	$(AT)echo Generating the definition file: $@
	$(AT)$(OcpiGen) -D $(GeneratedDir) $(and $(Package),-p $(Package)) \
	   $(if $(Libraries),$(foreach l,$(Libraries),-l $l)) \
	   $(and $(HdlPlatform),-P $(HdlPlatform)) \
	   -d $<

$(HdlOtherImplSourceFile): $(WDefsFile) $$(Worker_$(Worker)_xml) | $(GeneratedDir)
	$(AT)echo Generating the $(HdlOtherLanguage) implementation file: $@ from $(Worker_$(Worker)_xml)
	$(AT)$(OcpiGen) -D $(GeneratedDir) $(and $(Package),-p $(Package)) \
	$(and $(HdlPlatform),-P $(HdlPlatform)) \
	$(if $(Libraries),$(foreach l,$(Libraries),-l $l)) -w -i $(Worker_$(Worker)_xml) \

$(ImplHeaderFiles): $(DefsFile)


# VHDL doesn't have header files - they are just source files
#ifeq ($(HdlLanguage),vhdl)
$(call OcpiDbgVar,GeneratedSourceFiles,before vhdl)
GeneratedSourceFiles+=$(WDefsFile) $(HdlOtherImplSourceFile)
ifeq ($(HdlLanguage),vhdl)
GeneratedSourceFiles+=$(DefsFile) $(ImplHeaderFiles)
endif
$(call OcpiDbgVar,GeneratedSourceFiles,after vhdl)


#WkrExportNames=$(Worker)$(BF) $(Worker)_rv$(BF)
ifdef HdlToolRealCore
$(WkrExportNames): $(GeneratedSourceFiles)
endif
$(call OcpiDbgVar,WkrExportNames)
$(call OcpiDbgVar,GeneratedSourceFiles)
#all: 
#else
#GeneratedSourceFiles+=$(WDefsFile)
#endif
LibName=$(Worker)

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
	$(AT)echo Creating a link from $(LibDir) to $(ImplXmlFile) to expose the $(CwdName) implementation xml.
	$(AT)$(call MakeSymLink,$(ImplXmlFile),$(LibDir))

# Generate the stub files by providing a link from gen/worker.v to gen/worker-defs.v
# This enables 2 different things:
# 1. Creating of precompiled black-box-stub libraries for, e.g. XST (build in gen/hdl)
# 2. Allow tools with no precompiled libraries to access component decls (e.g. quartus)
# Note that this is not used or needed when real cores do not get built (sim)
$(call OcpiDbgVar,DefsFile)
$(LibDir)/$(Worker)$(HdlSourceSuffix): $(DefsFile) | $(LibDir)
	$(AT)echo Creating link from $@ to $(DefsFile) to expose the stub for worker "$(Worker)".
	$(AT)$(call MakeSymLink2,$(DefsFile),$(LibDir),$(Worker)$(HdlSourceSuffix))

$(LibDir)/$(Worker)$(HdlOtherSourceSuffix): $(WDefsFile) | $(LibDir)
	$(AT)echo Creating link from $@ to $(WDefsFile) to expose the other-language stub for worker "$(Worker)".
	$(AT)$(call MakeSymLink2,$(WDefsFile),$(LibDir),$(Worker)$(HdlOtherSourceSuffix))

$(call OcpiDbg,Before all: "$(LibDir)/$(Worker)$(HdlSourceSuffix)")
all: $(LibDir)/$(Worker)$(HdlSourceSuffix) $(LibDir)/$(Worker)$(HdlOtherSourceSuffix)

endif
endif # if not an assembly

endif # HdlSkip
