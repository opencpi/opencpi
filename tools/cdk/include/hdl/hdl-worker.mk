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
Compile=$(HdlCompile)
$(call OcpiDbgVar,HdlBin)
BF=$(call OBJ,$1)
# This could change someday if any hdl tools have separate object files.
OBJ=$(and $1,$(if $(filter 0,$1),,_c$1))$(HdlBin)
# We don't build independent standalone worker binaries (no artifact file here)
ArtifactFile=

HdlDefs=-defs
HdlSkelSuffix=-skel$(HdlSourceSuffix)
HdlDefsSuffix=$(HdlDefs)$(HdlIncSuffix)
HdlOtherDefsSuffix=$(HdlDefs)$(HdlOtherIncSuffix)
HdlImplSuffix=-impl$(HdlIncSuffix)
HdlOtherImplSuffix=-impl$(HdlOtherIncSuffix)

ifndef Tops
  ifdef Top
    Tops:=$(Top)
#  else 
#   ifeq ($(HdlLanguage),vhdl)
#    Tops=$(Worker) $(Worker)_rv
  else ifeq ($(HdlMode),worker)
# FIXME: when we create assemblies in VHDL, we can finally nuke this
    Tops:=$(Worker) $(Worker)_rv
  else ifeq ($(HdlMode),container)
    Tops:=$(Worker)
  else
    Tops:=$(Worker)_rv
  endif
endif
HdlCores=$(Tops)

ifndef Core
Core=$(Worker)
endif

ifdef HdlToolRealCore
WkrExportNames=$(Tops:%=%$(call BF,0))
endif
$(call OcpiDbgVar,Top)
$(call OcpiDbgVar,Tops)
$(call OcpiDbgVar,HdlCores)
$(call OcpiDbgVar,Core)
$(call OcpiDbgVar,WkrExportNames)
include $(OCPI_CDK_DIR)/include/xxx-worker.mk
override VerilogIncludeDirs += $(IncludeDirs)
ImplXmlFile=$(firstword $(ImplXmlFiles))
################################################################################
# Generated files: impl depends on defs, worker depends on impl
# map the generic "IncludeDirs" into the verilog
#$(HdlDefsSuffix))
#RefDefsFile=$(Workers:%=$(GeneratedDir)/%-defs.vh)
DefsFile=$(Worker:%=$(GeneratedDir)/%$(HdlDefsSuffix))
WDefsFile=$(Worker:%=$(GeneratedDir)/%$(HdlOtherDefsSuffix))
HdlOtherImplSourceFile=$(GeneratedDir)/$(Worker)$(HdlOtherImplSuffix)

# What source files should be put into the BB library?
# There are four cases:
# VHDL and ParamConfig 0: defs
# VHDL and ParamConfig > 0:
# Verilog and ParamConfig 0:
# Verilog and ParamConfig > 0:
# $(call CoreBlackBoxFiles,target,param-config)
HdlVerilogTargetDefs=$(call WkrTargetDir,$1,$2)/$(Worker)$(HdlDefs)$(HdlVerilogIncSuffix)
HdlVHDLTargetDefs=$(call WkrTargetDir,$1,$2)/$(Worker)$(HdlDefs)$(HdlVHDLSuffix)

CoreBlackBoxFiles=$(foreach d,$(DefsFile) $(WDefsFile),$(infoxx CBBF:$d)\
                     $(if $(filter 0,$2),$d,$(call WkrTargetDir,$1,$2)/$(notdir $d)))

OcpiHdl=$(DYN_PREFIX) $(ToolsDir)/ocpihdl 

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

# The above definitions are needed before skipping so we can export xml files
ifdef HdlSkip
$(call OcpiDbg, Skipping)
else
# This is the utility program for hdl
ifeq ($(shell if test -x $(ToolsDir)/ocpihdl; then echo xx; fi),)
ifneq ($(MAKECMDGOALS),clean)
$(error Missing ocpihdl utility program)
endif
endif


# VHDL doesn't have header files - they are just source files
#ifeq ($(HdlLanguage),vhdl)
$(call OcpiDbgVar,GeneratedSourceFiles,before vhdl)
GeneratedSourceFiles+=$(WDefsFile) $(HdlOtherImplSourceFile)
#ifeq ($(HdlLanguage),vhdl)
#GeneratedSourceFiles+=$(DefsFile)
#endif
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

endif # HdlSkip
$(call OcpiDbg,After skipping)
################################################################################
# If not an assembly or container, we have to contribute to the exports for the
# component library we are a part of.
ifneq ($(HdlMode),assembly)
# Expose the implementation xml file for apps that instantiate this worker core
ifdef LibDir
$(call OcpiDbg,Before all: "$(LibDir)/$(ImplXmlFile)")

all: $(LibDir)/$(ImplXmlFile)

$(LibDir)/$(ImplXmlFile): | $(LibDir)
	$(AT)echo Creating link from $(LibDir) -\> $(ImplXmlFile) to expose the $(CwdName) implementation xml.
	$(AT)$(call MakeSymLink,$(ImplXmlFile),$(LibDir))

# Generate the stub files by providing a link from gen/worker.v to gen/worker-defs.v
# This enables 2 different things:
# 1. Creating of precompiled black-box-stub libraries for, e.g. XST (build in gen/hdl)
# 2. Allow tools with no precompiled libraries to access component decls (e.g. quartus)
# Note that this is not used or needed when real cores do not get built (sim)
$(call OcpiDbgVar,DefsFile)
$(LibDir)/$(Worker)$(HdlSourceSuffix): $(DefsFile) | $(LibDir)
	$(AT)echo Creating link from $@ -\> $(DefsFile) to expose the stub for worker "$(Worker)".
	$(AT)$(call MakeSymLink2,$(DefsFile),$(LibDir),$(Worker)$(HdlSourceSuffix))

$(LibDir)/$(Worker)$(HdlOtherSourceSuffix): $(WDefsFile) | $(LibDir)
	$(AT)echo Creating link from $@ -\> $(WDefsFile) to expose the other-language stub for worker "$(Worker)".
	$(AT)$(call MakeSymLink2,$(WDefsFile),$(LibDir),$(Worker)$(HdlOtherSourceSuffix))

$(call OcpiDbg,Before all: "$(LibDir)/$(Worker)$(HdlSourceSuffix)")
all: $(LibDir)/$(Worker)$(HdlSourceSuffix) $(LibDir)/$(Worker)$(HdlOtherSourceSuffix) 

endif
endif # if not an assembly

