# This is the makefile for container directories where the assembly is elsewhere.
# If containers are built in subdirectories of assemblies, then the assembly one level
# up (in ..)
# One container will be built here, but it may be build for multiple build configurations.
# The HdlAssembly variable must be set to point to the relative or absolute path
# to the assembly's directory, ending in the name of the assembly.
HdlMode:=container
$(infox MYCL:$(ComponentLibraries))
include $(OCPI_CDK_DIR)/include/hdl/hdl-make.mk
# These next lines are similar to what worker.mk does
override Workers:=$(CwdName:container-%=%)
override Worker:=$(Workers)
XmlName:=$(Worker).xml
Worker_$(Worker)_xml:=$(or $(wildcard $(XmlName)),\
                           $(wildcard $(HdlAssembly)/$(XmlName)),\
                           $(wildcard $(GeneratedDir)/$(XmlName)))
Worker_xml:=$(Worker_$(Worker)_xml)
ifneq ($(MAKECMDGOALS),clean)
  ifndef Worker_xml
    $(error The XML for the container assembly, $(Worker).xml, is missing))
  endif
endif
OcpiLanguage:=vhdl
override HdlLibraries+=platform
# ComponentLibraries and XmlIncludeDirs are already passed to us on the command line.
#$(eval $(HdlSearchComponentLibraries))
#$(infox XMLI:$(XmlIncludeDirs))
#override XmlIncludeDirs+=$(HdlPlatformsDir) $(HdlPlatformsDir)/specs $(HdlAssembly)
$(infox XMLI2:$(XmlIncludeDirs))
AssemblyName=$(notdir $(HdlAssembly))
override LibDir=$(HdlAssembly)/lib/hdl
ifneq ($(MAKECMDGOALS),clean)
  # Manipulate targets before this
  $(and $(call DoShell,$(OcpiGen) -S $(AssemblyName) -x platform $(Worker_xml),HdlContPlatform),\
    $(error Processing container XML $1: $(HdlContPlatform)))
  $(and $(call DoShell,$(OcpiGen) -S $(AssemblyName) -x configuration $(Worker_xml),HdlContConfig),\
    $(error Processing container XML $1: $(HdlContConfig)))
  $(call OcpiDbgVar,HdlContPlatform)
  $(call OcpiDbgVar,HdlContConfig)
  $(if $(HdlContPlatform),,$(error Could not get HdlPlatform for container $1))
  $(if $(HdlContConfig),,$(error Could not get HdlConfiguration for container $1))
  override HdlPlatforms:=$(HdlContPlatform)
  override HdlPart:=$(HdlPart_$(HdlContPlatform))
  override HdlTargets:=$(call HdlGetFamily,$(HdlPart))
  override HdlPlatform:=$(HdlContPlatform)
  override HdlTarget:=$(HdlTargets)
  override HdlConfig:=$(HdlContConfig)
  include $(OCPI_CDK_DIR)/include/hdl/hdl-pre.mk
  ifndef HdlSkip
    override ComponentLibraries+=$(HdlPlatformsDir)/$(HdlContPlatform) $(HdlAssembly)
    $(eval $(HdlSearchComponentLibraries))
    ImplFile:=$(GeneratedDir)/$(Worker)-assy$(HdlSourceSuffix)
    WorkerSourceFiles=$(ImplFile)
    $(call OcpiDbgVar,ImplFile)
    ImplWorkersFile=$(GeneratedDir)/$(Worker).wks
    $(ImplFile): $$(ImplXmlFile) | $$(GeneratedDir)
	$(AT)echo Generating the container assembly source file: $@ from $<
	$(AT)$(call OcpiGen) -D $(GeneratedDir) -W $(Worker) -S $(AssemblyName) -a  $<
    # The workers file is actually created at the same time as the -assy.v file
    $(ImplWorkersFile): $(ImplFile)
    HdlPreCore=$(HdlGetCores)
    include $(OCPI_CDK_DIR)/include/hdl/hdl-worker.mk
    ifndef HdlSkip
      HdlContName=$(Worker)$(if $(filter 0,$1),,_$1)
      ArtifactXmlName=$(call WkrTargetDir,$(HdlTarget),$1)/$(Worker)-art.xml
      UUIDFileName=$(call WkrTargetDir,$(HdlTarget),$1)/$(Worker)_UUID.v
      MetadataRom=$(call WkrTargetDir,$(HdlTarget),$1)/metadatarom.dat
      HdlContPreCompile=\
        echo Generating UUID, artifact xml file and metadata ROM file for container $(Worker) "($1)". && \
        (cd .. && \
         $(OcpiGen) -D $(call WkrTargetDir,$(HdlTarget),$1) -A -S $(AssemblyName) -P $(HdlPlatform) -e $(HdlPart) $(ImplXmlFile) && \
         $(OcpiHdl) bram $(call ArtifactXmlName,$1) $(call MetadataRom,$1) \
        )
      # Now we need to make a bit file from every paramconfig for this worker
      HdlBitName=$(call BitFile_$(HdlToolSet_$(HdlTarget)),$(call HdlContName,$1))
      HdlContBitName=$(call WkrTargetDir,$(HdlTarget),$1)/$(call HdlBitName,$1)
      HdlContBitZName=$(call HdlContBitName,$1).gz
      HdlContBitZ=$(basename $(call HdlContBitName,$1)).bitz
      define ContDoConfig
        $(infox ART:$(call ArtifactXmlName,$1):UUID:$(call UUIDFileName,$1):BIT:$(call HdlBitName,$1):ROM:$(call MetadataRom,$1):BIN:$(call WkrBinary,$(HdlTarget),$1))
        $(infox HCBF:$(call HdlContBitName,$1))
        $(call UUIDFileName,$1):
        $(call WkrBinary,$(HdlTarget),$1): HdlPreCompile=$(call HdlContPreCompile,$1)
        $(call WkrBinary,$(HdlTarget),$1): TargetSourceFiles+=$(call UUIDFileName,$1)
        $(call HdlContBitName,$1): $(call WkrBinary,$(HdlTarget),$1)
        $(call HdlContBitZName,$1): $(call HdlContBitName,$1)
	   $(AT)echo Making compressed bit file: $$@ from $$< and $(call ArtifactXmlName,$1)
	   $(AT)gzip -c $(call HdlContBitName,$1) > $$@
	   $(AT)$(ToolsDir)/../../scripts/addmeta $(call ArtifactXmlName,$1) $$@

        $(call HdlContBitZ,$1): | $(call HdlContBitZName,$1)
	    $(AT)ln -s $(notdir $(call HdlContBitZName,$1)) $$@
        all: $(call HdlContBitZName,$1) $(call HdlContBitZ,$1)

        # Invoke tool build: <target-dir>,<assy-name>,<core-file-name>,<config>,<platform>
        $(eval $(call HdlToolDoPlatform_$(HdlToolSet_$(HdlTarget)),$(call WkrTargetDir,$(HdlTarget),$1),$(AssemblyName),$(Worker),$(HdlConfig),$(HdlPlatform),$1))
      endef
      $(call OcpiDbgVar,ParamConfigurations)
      $(foreach c,$(ParamConfigurations),$(eval $(call ContDoConfig,$c)))
    endif # skip from hdl-worker.mk
  endif # skip from hdl-pre.mk
endif # cleaning
