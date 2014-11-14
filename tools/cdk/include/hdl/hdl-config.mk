# This is the makefile for platform configuration directories where the platform is elsewhere.
# If configurations are built in subdirectories of platforms, then the platform is ..
# One configuration will be built here, but it may be build for multiple build configurations
# for each target.
# The HdlPlatformWorker variable must be set to point to the relative or absolute path
# to the platform's directory, ending in the name of the platform.
HdlMode:=config
include $(OCPI_CDK_DIR)/include/hdl/hdl-make.mk
# These next lines are similar to what worker.mk does
override Workers:=$(CwdName:config-%=%)
override Worker:=$(Workers)
XmlName:=$(Worker).xml
Worker_$(Worker)_xml:=$(or $(wildcard $(XmlName)),\
                           $(wildcard $(HdlPlatformWorker)/$(XmlName)),\
                           $(wildcard $(GeneratedDir)/$(XmlName)))
Worker_xml:=$(Worker_$(Worker)_xml)
ifneq ($(MAKECMDGOALS),clean)
  ifndef Worker_xml
    $(error The XML for the platform configuration, $(Worker).xml, is missing: $(HdlPlatformWorker):$(XmlName):$(Cwd))
  endif
endif
OcpiLanguage:=vhdl
HdlLibraries+=platform
PlatformName=$(notdir $(HdlPlatformWorker))
LibDir=$(HdlPlatformWorker)/lib/hdl
ifneq ($(MAKECMDGOALS),clean)
  # Manipulate targets before this
  override HdlPlatforms:=$(notdir $(HdlPlatformWorker))
  override HdlPart:=$(HdlPart_$(HdlPlatforms))
  override HdlTargets:=$(call HdlGetFamily,$(HdlPart))
  override HdlPlatform:=$(HdlPlatforms)
  override HdlTarget:=
  include $(OCPI_CDK_DIR)/include/hdl/hdl-pre.mk
  ifndef HdlSkip
    $(eval $(HdlSearchComponentLibraries))
    ImplFile:=$(GeneratedDir)/$(Worker)-assy$(HdlSourceSuffix)
    WorkerSourceFiles=$(ImplFile)
    $(call OcpiDbgVar,ImplFile)
    ImplWorkersFile=$(GeneratedDir)/$(Worker).wks
    $(ImplFile): $$(ImplXmlFile) | $$(GeneratedDir)
	$(AT)echo Generating the platform configuration assembly source file: $@ from $<
	$(AT)$(call OcpiGen) -D $(GeneratedDir) -P $(PlatformName) -W $(Worker) -a  $<
    # The workers file is actually created at the same time as the -assy.v file
    $(ImplWorkersFile): $(ImplFile)
    include $(OCPI_CDK_DIR)/include/hdl/hdl-worker.mk
  endif # skip from hdl-pre.mk
endif # cleaning
