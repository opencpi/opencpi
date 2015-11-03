# This is the makefile for container directories where the assembly might be elsewhere.
# If containers are built in subdirectories of assemblies, then the assembly one level
# up (in ..)
# One container will be built here, but it may be build for multiple platforms.
# The HdlAssembly variable must be set to point to the relative or absolute path
# to the assembly's directory, ending in the name of the assembly.
HdlMode:=container
$(infox MYCL:$(ComponentLibraries):$(ComponentLibrariesInternal):$(XmlIncludeDirs):$(MdlAssembly))
ifndef HdlPlatforms
HdlPlatforms:=$(HdlPlatform)
endif
include $(OCPI_CDK_DIR)/include/hdl/hdl-make.mk
# These next lines are similar to what worker.mk does
override Workers:=$(CwdName:container-%=%)
override Worker:=$(Workers)
XmlName:=$(Worker).xml
# XML file is either here or generated in the assembly or generated here
Worker_$(Worker)_xml:=$(or $(wildcard $(XmlName)),\
                           $(wildcard $(HdlAssembly)/gen/$(XmlName)),\
                           $(wildcard $(GeneratedDir)/$(XmlName)))
Worker_xml:=$(Worker_$(Worker)_xml)
Assembly:=$(notdir $(HdlAssembly))
# Unless we are cleaning, figure out our platform, its dir, and the platform config
ifneq ($(MAKECMDGOALS),clean)
  ifndef Worker_xml
    $(error The XML for the container assembly, $(Worker).xml, was not found)
  endif
  ifeq ($(wildcard $(Worker_xml)),)
    $(error Cannot find an XML file for container: $(Worker))
  endif
  $(and $(call DoShell,$(OcpiGen) -X $(Worker_xml),HdlContPfConfig),\
     $(error Processing container XML $(Worker_xml): $(HdlContPfConfig)))
  HdlContPf:=$(word 1,$(HdlContPfConfig))
  ifdef HdlPlatforms
   ifeq ($(filter $(HdlContPf),$(HdlPlatforms)),)
     $(info Nothing built since container platform is $(HdlContPf), which is not in HdlPlatforms: $(HdlPlatforms))
     HdlSkip:= 1
   endif
  endif
  ifeq ($(filter $(HdlContPf),$(HdlAllPlatforms)),)
    $(error The platform $(HdlContPfConfig) in $(Worker_xml) is unknown.)
  endif
  override HdlPlatform:=$(HdlContPf)
  override HdlPlatforms:=$(HdlPlatform)
  HdlPlatformDir:=$(HdlPlatformDir_$(HdlPlatform))
  HdlPart:=$(call HdlGetPart,$(HdlPlatform))
  override HdlTargets:=$(call HdlGetFamily,$(HdlPart))
  override HdlTarget:=$(HdlTargets)
  HdlConfig:=$(word 2,$(HdlContPfConfig))
  Platform:=$(HdlPlatform)
  PlatformDir:=$(HdlPlatformDir)
endif
OcpiLanguage:=vhdl
override HdlLibraries+=platform
# ComponentLibraries and XmlIncludeDirs are already passed to us on the command line.
# Note that the platform directory should be first XML dir since the config file name should be
# scoped to the platform.
override XmlIncludeDirsInternal:=\
   $(call Unique,$(HdlPlatformDir) $(HdlPlatformDir)/hdl $(XmlIncludeDirs) \
      $(HdlPlatformsDir)/specs $(HdlAssembly))
# We might be called from an assembly directory, in which case many of the
# component libraries are passed through to us, but we might be standalone.
# Thus we add "components" and "adapters" here again
override ComponentLibraries:=$(call Unique,\
  $(ComponentLibraries) $(ComponentLibrariesInternal) \
  $(HdlPlatformDir) $(HdlAssembly) \
  components devices adapters cards)
$(infox XMLI2:$(XmlIncludeDirsInternal):$(ComponentLibraries):$(HdlPlatform):$(HdlPlatformDir_$(HdlPlatform)))
#AssemblyName=$(notdir $(HdlAssembly))
override LibDir=$(HdlAssembly)/lib/hdl
ifneq ($(MAKECMDGOALS),clean)
  override Platform:=$(if $(filter 1,$(words $(HdlPlatforms))),$(HdlPlatforms))
  $(eval $(HdlSearchComponentLibraries))
  include $(OCPI_CDK_DIR)/include/hdl/hdl-pre.mk
  ifndef HdlSkip
    $(eval $(HdlPrepareAssembly))
    include $(OCPI_CDK_DIR)/include/hdl/hdl-worker.mk
    ifndef HdlSkip
      HdlContName=$(Worker)$(if $(filter 0,$1),,_$1)
      ArtifactXmlName=$(call WkrTargetDir,$(HdlTarget),$1)/$(Worker)-art.xml
      UUIDFileName=$(call WkrTargetDir,$(HdlTarget),$1)/$(Worker)_UUID.v
      MetadataRom=$(call WkrTargetDir,$(HdlTarget),$1)/metadatarom.dat
      HdlContPreCompile=\
        echo Generating UUID, artifact xml file and metadata ROM file for container $(Worker) "($1)". && \
        (cd .. && \
         $(OcpiGen) -D $(call WkrTargetDir,$(HdlTarget),$1) -A -S $(Assembly) -P $(HdlPlatform) \
                    -e $(HdlPart) -F $(PlatformDir) $(ImplXmlFile) && \
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
        $(eval $(call HdlToolDoPlatform_$(HdlToolSet_$(HdlTarget)),$(call WkrTargetDir,$(HdlTarget),$1),$(Assembly),$(Worker),$(HdlConfig),$(HdlPlatform),$1))
      endef
      $(call OcpiDbgVar,ParamConfigurations)
      $(foreach c,$(ParamConfigurations),$(eval $(call ContDoConfig,$c)))
    endif # skip from hdl-worker.mk
  endif # skip from hdl-pre.mk
endif # cleaning
clean::
	$(AT) rm -r -f target-* gen lib
