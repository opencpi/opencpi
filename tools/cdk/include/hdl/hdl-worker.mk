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

# Makefile fragment for HDL workers

ifndef HdlMode
HdlMode:=worker
endif
$(call OcpiDbg,Entering hdl-worker.mk)
include $(OCPI_CDK_DIR)/include/hdl/hdl-pre.mk
ifneq ($(filter worker,$(HdlMode)),)
$(foreach t,$(HdlTargets),$(eval SubCores_$t:=$(Cores)))
endif
ifneq ($(filter skeleton generated,$(MAKECMDGOALS)),)
  HdlSkip:=
endif
Compile=$(HdlCompile)
$(call OcpiDbgVar,HdlBin)
$(infox LANGUAGE:$(HdlLanguage))
BF=$(call OBJ,$2)
#BF=$(HdlBin)
#BF=$(if $(filter vhdl,$(HdlLanguage)),_rv)$(HdlBin)
# This object suffix must include the paramconfig name because, at least for
# library-only tool chains, the library name must include the paramconfig
# name since an assembly might include multiple instances of the same
# worker but with different paramconfigs.
OBJ=$(and $1,$(if $(filter 0,$1),,_c$1))$(HdlBin)
# We don't build independent standalone worker binaries (no artifact file here)
ArtifactFile=

HdlDefs=-defs
HdlImpl=-impl
HdlSkelSuffix=-skel$(HdlSourceSuffix)
HdlDefsSuffix=$(HdlDefs)$(HdlIncSuffix)
HdlOtherDefsSuffix=$(HdlDefs)$(HdlOtherIncSuffix)
HdlImplSuffix=-impl$(HdlIncSuffix)
HdlOtherImplSuffix=-impl$(HdlOtherIncSuffix)

HdlIsDevice:=
ifeq ($(HdlMode),worker)
  ifneq ($(shell egrep -i '<hdldevice' $(Worker_$(Worker)_xml)),)
    HdlIsDevice:=1
  endif
else ifneq ($(filter config container,$(HdlMode)),)
    HdlIsDevice:=1
endif

ifndef Tops
  ifdef Top
    Tops:=$(Top)
#  else 
#   ifeq ($(HdlLanguage),vhdl)
#    Tops=$(Worker) $(Worker)_rv
  else ifeq ($(HdlMode),worker)
# FIXME: when we create assemblies in VHDL, we can finally nuke this
    ifneq ($(HdlIsDevice),)
      Tops:=$(Worker)_rv
    else
      ifneq ($(shell egrep -i '<hdlworker' $(Worker_$(Worker)_xml)),)
        Tops:=$(Worker)
      else # usually HdlImplementation - legacy
        Tops:=$(Worker) $(Worker)_rv
      endif
    endif
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
# Specify the name of the primary binary result for the worker.
# There are three interacting issues:
# 1. The language of this worker
# 2. The langauge of any assembly this worker might be used in.
# 3. The mode we are building in
ifneq ($(filter $(HdlMode),config assembly),)
WkrBinaryName=$(Worker)_rv
else
WkrBinaryName=$(Worker)
endif
include $(OCPI_CDK_DIR)/include/xxx-worker.mk
override VerilogIncludeDirs=$(call Unique, . $(GeneratedDir) $(HdlIncludeDirs) $(IncludeDirs) $(HdlIncludeDirsInternal) $(OCPI_CDK_DIR)/include/hdl)
$(call OcpiDbgVar,VerilogIncludeDirs)
ImplXmlFile=$(firstword $(ImplXmlFiles))
################################################################################
# Generated files: impl depends on defs, worker depends on impl
DefsFile=$(Worker:%=$(GeneratedDir)/%$(HdlDefsSuffix))
WDefsFile=$(Worker:%=$(GeneratedDir)/%$(HdlOtherDefsSuffix))
VHDLDefsFile=$(Worker:%=$(GeneratedDir)/%$(HdlDefs)$(HdlVHDLSuffix))
VHDLImplFile=$(Worker:%=$(GeneratedDir)/%$(HdlImpl)$(HdlVHDLSuffix))
VerilogDefsFile=$(Worker:%=$(GeneratedDir)/%$(HdlDefs)$(HdlVerilogSuffix))
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
HdlVHDLTargetImpl=$(call WkrTargetDir,$1,$2)/$(Worker)$(HdlImpl)$(HdlVHDLSuffix)

# CoreBlackBoxFiles=$(foreach d,$(DefsFile) \
#                               $(if $(filter $(HdlMode),container config),,$(WDefsFile)),\
#                               $(infoxx CBBF:$d)\
#                      $(if $(filter 0,$2),$d,$(call WkrTargetDir,$1,$2)/$(notdir $d)))\
#                      $(call WkrTargetDir,$1,$2)/generics.vhd \
#                      $(call WkrTargetDir,$1,$2)/generics.vh
CoreBlackBoxFiles=$(and $(filter-out library core,$(HdlMode)),\
  $(call HdlVHDLTargetDefs,$1,$2)\
  $(call HdlVerilogTargetDefs,$1,$2) \
  $(call WkrTargetDir,$1,$2)/generics$(HdlVHDLIncSuffix))\

OcpiHdl=$(DYN_PREFIX) $(ToolsDir)/ocpihdl 

$(WDefsFile): $(Worker_$(Worker)_xml) | $(GeneratedDir)
	$(AT)echo Generating the opposite language definition file: $@
	$(AT)$(call OcpiGen, -D $(GeneratedDir) $(and $(Package),-p $(Package))  \
	  $(and $(HdlPlatform),-P $(HdlPlatform)) \
          $(and $(Assembly),-S $(Assembly)) \
          $(and $(PlatformDir), -F $(PlatformDir)) \
	  $(HdlVhdlLibraries) -w -d $<)

$(DefsFile): $(Worker_$(Worker)_xml) | $(GeneratedDir)
	$(AT)echo Generating the definition file: $@
	$(AT)$(call OcpiGen, -D $(GeneratedDir) $(and $(Package),-p $(Package)) \
           $(and $(Assembly),-S $(Assembly)) \
	   $(and $(HdlPlatform),-P $(HdlPlatform)) \
           $(and $(PlatformDir), -F $(PlatformDir)) \
	   $(HdlVhdlLibraries) -d $<)

$(HdlOtherImplSourceFile): $(WDefsFile) $$(Worker_$(Worker)_xml) | $(GeneratedDir)
	$(AT)echo Generating the $(HdlOtherLanguage) implementation file: $@ from $(Worker_$(Worker)_xml)
	$(AT)$(call OcpiGen, -D $(GeneratedDir) $(and $(Package),-p $(Package)) \
        $(and $(Assembly),-S $(Assembly)) \
	$(and $(HdlPlatform),-P $(HdlPlatform)) \
        $(and $(PlatformDir), -F $(PlatformDir)) \
	$(HdlVhdlLibraries) -w -i $(Worker_$(Worker)_xml))

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
$(call OcpiDbgVar,GeneratedSourceFiles,before vhdl)
ifeq (,$(filter $(HdlMode),container config))
#  GeneratedSourceFiles+=$(WDefsFile) $(HdlOtherImplSourceFile)
endif
$(call OcpiDbgVar,GeneratedSourceFiles,after vhdl)

ifdef HdlToolRealCore
  $(WkrExportNames): $(GeneratedSourceFiles)
endif
$(call OcpiDbgVar,WkrExportNames)
$(call OcpiDbgVar,GeneratedSourceFiles)
LibName=$(Worker)

HdlVHDLParamSignalDecls=$(call WkrTargetDir,$1,$2)/parameterized_signal_decls.vhd
HdlVHDLParamSignalMap=$(call WkrTargetDir,$1,$2)/parameterized_signal_map.vhd
HdlVerilogParamSignalDecls=$(call WkrTargetDir,$1,$2)/parameterized_signal_decls.v

define DoImplConfig
  $(call HdlVHDLParamSignalDecls,$1,$2): $(call WkrTargetDir,$1,$2)/generics.vhd \
	                                 | $(call WkrTargetDir,$1,$2)
	$(AT)sed -n -e '/^ *--__decl/s///p' $$< >$$@

  $(call HdlVHDLParamSignalMap,$1,$2): $(call WkrTargetDir,$1,$2)/generics.vhd \
	                               | $(call WkrTargetDir,$1,$2)
	$(AT)sed -n -e '/^ *--__map/s///p' $$< >$$@

  $(call HdlVerilogParamSignalDecls,$1,$2): $(call WkrTargetDir,$1,$2)/generics.vh \
	                               | $(call WkrTargetDir,$1,$2)
	$(AT)sed -n -e '+^ *//__decl+s+++p' $$< >$$@

  $(call HdlVHDLTargetImpl,$1,$2) $(call HdlVHDLTargetDefs,$1,$2): \
                     $(call WkrTargetDir,$1,$2)/%: $(GeneratedDir)/% \
	             $(and $(HdlIsDevice),\
                           $(call HdlVHDLParamSignalMap,$1,$2) \
                           $(call HdlVHDLParamSignalDecls,$1,$2)) \
                     $(call WkrTargetDir,$1,$2)/generics.vhd | $(call WkrTargetDir,$1,$2)
	$(AT)sed -e :x \
              $(if $(filter 0,$2),,-e "s/--__/_c$2/") \
              $(and $(HdlIsDevice),\
                -e "/--_parameterized_signal_decls/r $(call HdlVHDLParamSignalDecls,$1,$2)" \
                -e "/--_parameterized_signal_map/r $(call HdlVHDLParamSignalMap,$1,$2)" \
	       ) \
              $$< >$$@

  # For Verilog we must insert the constants file into the defs file so that we don't
  # need to export the "include" file.
  $(call HdlVerilogTargetDefs,$1,$2): $(call WkrTargetDir,$1,$2)/% : \
                                      $(GeneratedDir)/% $(call WkrTargetDir,$1,$2)/generics.vh
	$(AT)sed $(and $2,$(filter-out 0,$2),-e s-//__-_c$2-) \
              -e '/`include.*"generics.vh"/r $(call WkrTargetDir,$1,$2)/generics.vh' \
              -e '/`include.*"generics.vh"/d' \
              $(and $(HdlIsDevice),\
                -e '/\/\/_parameterized_signal_decls/r $(call HdlVerilogParamSignals,$1,$2)') \
              $$< > $$@

endef
ifneq ($(MAKECMDGOALS),clean)
  ifneq ($(MAKECMDGOALS),skeleton)
    $(foreach c,$(ParamConfigurations),\
      $(foreach t,$(HdlActualTargets),\
        $(eval $(call DoImplConfig,$t,$c))))
  endif
endif
################################################################################
# Include this to build the core or the library
include $(OCPI_CDK_DIR)/include/hdl/hdl-core2.mk

endif # HdlSkip
$(call OcpiDbg,After skipping)
generated: skeleton  $(GeneratedSourceFiles)
################################################################################
# If not an assembly or container, we have to contribute to the exports for the
# component library we are a part of.
#ifneq ($(HdlMode),assembly)
# Expose the implementation xml file for apps that instantiate this worker core
ifdef LibDir
$(call OcpiDbg,Before all: "$(LibDir)/$(ImplXmlFile)")

genlinks: $(LibDir)/$(notdir $(ImplXmlFile))

$(LibDir)/$(notdir $(ImplXmlFile)): | $(LibDir)
	$(AT)echo Creating link from $(LibDir) -\> $(ImplXmlFile) to expose the $(CwdName) implementation xml.
	$(AT)$(call MakeSymLink,$(ImplXmlFile),$(LibDir))

# Export the stub/defs files by providing links to the defs files that are
# in the target directories.  This allows assemblies to compile with these source files
# to instantiate cores.

$(call OcpiDbgVar,DefsFile)
# Macro to generate a links for a target $1 and a configuration $2
HdlDefsDir=$(call WkrTargetDir,$1,$2)
define DoDefsLinks

$(LibDir)/$1/$(Worker)$3$(HdlSourceSuffix): \
                 $(call HdlDefsDir,$1,$2,$(HdlLanguage))/$(notdir $(DefsFile)) | $(LibDir)/$1
	$(AT)echo Creating link from $$@ -\> $$< to expose the definition of worker "$(Worker)$3".
	$(AT)$$(call MakeSymLink2,$$<,$$(dir $$@),$$(notdir $$@))

$(LibDir)/$1/$(Worker)$3$(HdlOtherSourceSuffix): \
                 $(call HdlDefsDir,$1,$2,$(HdlOtherLanguage))/$(notdir $(WDefsFile)) | $(LibDir)
	$(AT)echo Creating link from $$@ -\> $$< to expose the other-language stub for worker "$(Worker)$3".
	$(AT)$$(call MakeSymLink2,$$<,$$(dir $$@),$$(notdir $$@))

$(call OcpiDbg,Before all: "$(LibDir)/$(Worker)$(HdlSourceSuffix)")
genlinks: $(LibDir)/$1/$(Worker)$3$(HdlSourceSuffix) $(LibDir)/$1/$(Worker)$3$(HdlOtherSourceSuffix)

endef

$(foreach t,$(HdlTargets),\
  $(foreach c,$(ParamConfigurations),\
    $(eval $(call DoDefsLinks,$t,$c,$(if $(filter $c,0),,_c$c)))))

all: links

endif
#endif # if not an assembly

