# Makefile for an hdl assembly, which is a lot like a worker...
include $(OCPI_DIR)/include/util.mk

AppName=$(CwdName)
Worker=$(AppName)
Application=yes
GeneratedSourceFiles+=$(GeneratedDir)/$(Worker)$(SourceSuffix)
LibDir=$(OutDir)lib/hdl
$(LibDir): | $(OutDir)lib
$(LibDir)/$(Target): | $(LibDir)
include $(OCPI_DIR)/include/hdl/hdl-worker.mk
$(LibDir) $(OutDir)lib $(LibDir)/$(Target) $(GeneratedDir)/hdl:
	mkdir $@
all: $(LibDir)/$(Target)/ocpi_app.ngc

$(LibDir)/$(Target)/ocpi_app.ngc:
	ln -s $(notdir $(BinaryFile)) $@

CompiledSourceFiles+=$(GeneratedDir)/$(Worker)$(SourceSuffix)
override XmlIncludeDirs += $(OcpiLibraries:%=%/lib/hdl) $(OcpiLibraries:%=%/lib)

# We generate the verilog assembly based on the "implementation XML" which is HdlAssembly
$(GeneratedSourceFiles): $(ImplXmlFiles) | $(GeneratedDir)
	$(AT)echo Generating the application source file: $@
	$(AT)$(OcpiGen) -a  $<
	$(AT)mv $(GeneratedDir)/$(Worker)_assy.v $@

all: | $(LibDir)/$(Target)

ifdef Containers
define doContainer
$(GeneratedDir)/$(1)_art.xml: $(1).xml $(AppName).xml
	$(AT)echo Generating the container/bitstream runtime xml file: $$@
	$(AT)$(OcpiGen) -A -h $(1).xml $(AppName).xml

all: $(GeneratedDir)/$(1)_art.xml
endef
$(foreach c,$(Containers),$(eval $(call doContainer,$(c))))
endif

CleanFiles += $(OutDir)lib
# Build the stub library
all: $(LibDir)/$(call LibraryAccessTarget,$(Target))/$(call LibraryFileTarget,$(Target))

$(LibDir)/$(call LibraryAccessTarget,$(Target))/$(call LibraryFileTarget,$(Target)): $(DefsFiles) | $(GeneratedDir)/hdl
	$(AT)echo Building HDL stub libraries for this component library
	$(AT)$(MAKE) -C $(OutDir)gen/hdl -L -f $(abspath $(OCPI_DIR))/include/hdl/hdl-lib.mk LibName=work SourceFiles=$(call AdjustRelative2,$(DefsFiles)) OCPI_DIR=$(call AdjustRelative2,$(OCPI_DIR)) Targets=$(call LibraryAccessTarget,$(Target)) LibName=app
	$(AT)$(foreach f,$(call LibraryAccessTarget,$(Target)),\
		echo Exporting the stub library for the app $(AppName) target $(f);\
		rm -r -f $(LibDir)/$(f);\
		mkdir $(LibDir)/$(f);\
		cp -r -p $(GeneratedDir)/hdl/$(f)/app/* $(LibDir)/$(f);)

