
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

# Makefile for an hdl assembly, which is a lot like a worker...
include $(OCPI_CDK_DIR)/include/util.mk

AppName=$(CwdName)
Worker=$(AppName)
Application=yes
GeneratedSourceFiles+=$(GeneratedDir)/$(Worker)$(SourceSuffix)
LibDir=$(OutDir)lib/hdl
$(LibDir): | $(OutDir)lib
$(LibDir)/$(Target): | $(LibDir)
include $(OCPI_CDK_DIR)/include/hdl/hdl-worker.mk
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
	$(AT)$(MAKE) -C $(OutDir)gen/hdl -L \
		-f $(call AdjustRelative2,$(OCPI_CDK_DIR))/include/hdl/hdl-lib.mk LibName=work \
		SourceFiles=$(call AdjustRelative2,$(DefsFiles)) \
		OCPI_CDK_DIR=$(call AdjustRelative2,$(OCPI_CDK_DIR)) \
		Targets=$(call LibraryAccessTarget,$(Target)) LibName=app
	$(AT)$(foreach f,$(call LibraryAccessTarget,$(Target)),\
		echo Exporting the stub library for the app $(AppName) target $(f);\
		rm -r -f $(LibDir)/$(f);\
		mkdir $(LibDir)/$(f);\
		cp -r -p $(GeneratedDir)/hdl/$(f)/app/* $(LibDir)/$(f);)

