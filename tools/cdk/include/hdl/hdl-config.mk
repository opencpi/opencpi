# This is the makefile for platform configuration directories where the platform is elsewhere.
# If configurations are built in subdirectories of platforms, then the platform is ..
# One configuration will be built here.
# The HdlPlatformWorker variable must be set to point to the relative or absolute path
# to the platform's directory, ending in the name of the platform.
HdlMode:=config
Model:=hdl
include $(OCPI_CDK_DIR)/include/hdl/hdl-make.mk
# These next lines are similar to what worker.mk does
override Workers:=$(CwdName:config-%=%)
override Worker:=$(Workers)
XmlName:=$(Worker).xml
Worker_$(Worker)_xml:=$(or $(wildcard $(XmlName)),\
                           $(wildcard $(HdlPlatformWorker)/$(XmlName)),\
                           $(wildcard $(HdlPlatformWorker)/gen/$(XmlName)),\
                           $(wildcard $(GeneratedDir)/$(XmlName)))
Worker_xml:=$(Worker_$(Worker)_xml)
ifndef Worker_xml
  $(error The XML for the platform configuration, $(Worker).xml, is missing)
endif
OcpiLanguage:=vhdl
override HdlLibraries+=platform
PlatformName=$(notdir $(HdlPlatformWorker))
LibDir=$(HdlPlatformWorker)/lib/hdl
override HdlPlatforms:=$(notdir $(HdlPlatformWorker))
override HdlPlatform:=$(HdlPlatforms)
override HdlPart:=$(HdlPart_$(HdlPlatforms))
override HdlTargets:=$(call HdlGetFamily,$(HdlPart))
override HdlTarget:=$(HdlTargets)
override Platform:=$(HdlPlatform)
override XmlIncludeDirsInternal+=$(HdlPlatformDir_$(Platform)) $(HdlPlatformDir_$(Platform))/hdl
# There are two cases for component libraries relevant to platform configurations
# 1. We are called below/in the platform's directory (and ComponentLibrariesInternal is defined)
# 2. We are remote from the platform's directory
# In both cases we start with the platform directory itself acting as a component library, where
# the platform is built.
ifdef ComponentLibrariesInternal
  ComponentLibraries=$(HdlPlatformDir_$(Platform)) $(ComponentLibrariesInternal)
else
  ComponentLibraries=$(HdlPlatformDir_$(Platform)) \
   $(wildcard $(HdlPlatformDir_$(Platform))/devices) \
   $(and $(ComponentLibraries_$(Platform)),\
     $(join $(HdlPlatformDir_$(Platform)),$(ComponentLibraries_$(Platform))))\
   devices cards
endif
ComponentLibraries:=$(call Unique,$(ComponentLibraries))

include $(OCPI_CDK_DIR)/include/hdl/hdl-pre.mk
ifneq ($(MAKECMDGOALS),clean)
  ifndef HdlSkip
    $(eval $(OcpiProcessBuildFiles))
    $(eval $(HdlPrepareAssembly))
    include $(OCPI_CDK_DIR)/include/hdl/hdl-worker.mk
  endif # skip from hdl-pre.mk
endif # cleaning
