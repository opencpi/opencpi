
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

# This makefile is for building applications and bitstreams.

HdlMode:=application
include $(OCPI_CDK_DIR)/include/hdl/hdl-make.mk

################################################################################
# Deal with platform issues if there are specified platforms
# Filter platforms
HdlPlatformsBeforeOnly:=$(HdlPlatforms)
ifdef HdlOnlyPlatforms
override HdlPlatforms:=$(filter $(HdlOnlyPlatforms),$(HdlPlatforms))
$(call OcpiDbgVar,HdlPlatforms,After filtering out HdlOnlyPlatforms )
endif

ifndef HdlPlatforms
ifdef HdlOnlyPlatforms
$(info No platforms in HdlPlatforms ($(HdlPlatformsBeforeOnly)) built.)
$(info This application only builds for these platforms: $(HdlOnlyPlatforms))
clean::
endif
else

# Skip synthesis targets that aren't for any specified platform
# (and mention what we are skipping)
$(call OcpiDbgVar,HdlTargets,Before filtering out non-platform targets )
$(call OcpiDbgVar,HdlPlatforms,Before filtering out non-platform targets )
override HdlTargets:=$(sort\
   $(foreach ht,$(HdlTargets),\
     $(or \
       $(foreach p,$(HdlPlatforms),\
         $(foreach part,$(call HdlGetPart,$p),\
           $(or $(findstring $(part),$(ht)),\
                $(findstring $(call HdlGetFamily,$(part)),$(ht))))),\
      $(info Skipping target $(ht), not in specified platforms: $(HdlPlatforms)))))

$(call OcpiDbgVar,HdlTargets,After filtering out non-platform targets )

# Establish where the platforms are
ifndef HdlPlatformsDir
  HdlPlatformsDir:=$(OCPI_CDK_DIR)/lib/hdl/platforms
  ifeq ($(realpath $(HdlPlatformsDir)),)
    HdlPlatformsDir:=$(OCPI_BASE_DIR)/hdl/platforms
    ifeq ($(realpath $(HdlPlatformsDir)),)
      $(error No HDL platforms found.  Looked in $(OCPI_CDK_DIR)/lib/hdl/platforms and $(OCPI_BASE_DIR)/hdl/platforms.)
    endif
  endif
endif
$(call OcpiDbgVar,HdlPlatformsDir)

# Make sure all the platforms are present
$(foreach p,$(HdlPlatforms),\
  $(if $(realpath $(HdlPlatformsDir)/$(p)),,\
     $(error No $(p) platform found in $(HdlPlatformsDir))))

################################################################################
# We are a lot like an HDL worker (and thus a core)
# The implementation xml file is HdlAssembly, not HdlImplementation
# Otherwise we pretty much just build the worker -
# which is the "app" without container or the platform
Core:=$(CwdName)
# In case the tools need a black box to find this core we are building,
# The empty module will just be a link to the defs file.
CoreBlackBoxFile:=$(GeneratedDir)/$(Core)_bb$(HdlSourceSuffix)
$(CoreBlackBoxFile): $$(DefsFile) | $(OutDir)gen
	$(AT)$(call MakeSymLink2,$(DefsFile),$(GeneratedDir),$(notdir $(CoreBlackBoxFile)))

# Generate the source code for this "assembly worker" implementation file.
ImplFile:=$(GeneratedDir)/$(CwdName)$(HdlSourceSuffix)
$(ImplFile): $$(ImplXmlFile) | $$(GeneratedDir)
	$(AT)echo Generating the application source file: $@
	$(AT)$(OcpiGen) -a  $<
	$(AT)mv $(GeneratedDir)/$(Worker)_assy.v $@
# The source code for this "worker" is the generated assembly file.
SourceFiles:=$(ImplFile)

# When parsing the HdlAssembly file, we need access to the xml from the 
# workers in the assembly, both at the implementation level and the spec level
override XmlIncludeDirs += \
  $(ComponentLibraries:%=%/lib/hdl) \
  $(ComponentLibraries:%=%/lib)

include $(OCPI_CDK_DIR)/include/hdl/hdl-worker.mk

################################################################################
# At this point we have basically built the app cores, and if needed, the
# black box libraries, and we have recursed for the various tools.
# (i.e. have run "make" per tool, with all the targets for a tool built in one "make").
# Now we build the container core on top of the app core, only for synthesis.
ifndef HdlSkip
ifdef HdlSimTool
$(info No container core built for simulation tool $(HdlToolSet))
else
ifndef Container
Container=mkOCApp-noADC-3w.v
endif
ContainerRtl=$(HdlPlatformsDir)/../containers/$(Container)
# Too bad we can't reuse what is in hdl-core2.mk a second time here
ContainerModule:=mkOCApp4B


# Test: is this needed?
#$(OutDir)target-$1/$(Worker)_bb/$(call HdlToolLibRef,$(Worker)_bb,$1):
#	$(AT)ln -s . $(OutDir)target-$1/$(Worker)_bb/$1

# The container (core) is built in the platform directory
# First arg is platform
# Second arg is part
# Third arg is family
define DoContainer
$(call OcpiDbg,DoContainer($1,$2,$3,$(HdlBin)))

ifdef HdlToolNeedBB
# The link that makes the bb library be in the right place
# namely under the family
$(OutDir)target-$3/$(Worker)_bb/$3:
	$(AT)ln -s . $(OutDir)target-$3/$(Worker)_bb/$3
endif
# The link that makes the app core be in the right place
# namely: target-<family>/<family>/<foo>.ngc
# so we make a simple link for the family name
$(OutDir)target-$3/$3: $(OutDir)target-$3/$(Core)$(HdlBin)
	$(AT)pwd;ln -s . $(OutDir)target-$3/$3

$(OutDir)target-$1/$(Worker):
	$(AT)mkdir $$@

Containers+=$(OutDir)target-$1/$(ContainerModule).v
$(OutDir)target-$1/container.v: | $(OutDir)target-$1
ContainerCores+=$(OutDir)target-$1/$(ContainerModule)$(HdlBin)
$(OutDir)target-$1/$(ContainerModule)$(HdlBin): Core=$(ContainerModule)
$(OutDir)target-$1/$(ContainerModule)$(HdlBin): Top=$(ContainerModule)
$(OutDir)target-$1/$(ContainerModule)$(HdlBin): Cores=$(OutDir)target-$3/$(Worker)
$(OutDir)target-$1/$(ContainerModule)$(HdlBin): LibName=$(ContainerModule)
$(OutDir)target-$1/$(ContainerModule)$(HdlBin): override HdlTarget:=$2
$(OutDir)target-$1/$(ContainerModule)$(HdlBin): TargetDir=$(OutDir)target-$1
$(OutDir)target-$1/$(ContainerModule)$(HdlBin): | $$$$(TargetDir)
$(OutDir)target-$1/$(ContainerModule)$(HdlBin): HdlSources=$(OutDir)target-$1/$(ContainerModule).v $(OutDir)target-$1/$(Worker)_UUID.v
$(OutDir)target-$1/$(ContainerModule)$(HdlBin): $(OutDir)target-$1/$(ContainerModule).v
$(OutDir)target-$1/$(ContainerModule)$(HdlBin): $(OutDir)target-$1/$(Worker)_UUID.v
# Need the link to the bb lib
$(OutDir)target-$1/$(ContainerModule)$(HdlBin): | $(OutDir)target-$3/$(Worker)_bb/$3
# Need the link to the core
$(OutDir)target-$1/$(ContainerModule)$(HdlBin): | $(OutDir)target-$3/$3
	$(AT)echo Building container core \"$(ContainerModule)\" for target \"$$(HdlTarget)\"
	$(AT)$$(HdlCompile)
endef

#$(info HAT=$(HdlActualTargets)=)
$(foreach t,$(HdlPlatforms),$(eval $(call DoContainer,$t,$(call HdlGetPart,$t),$(call HdlGetFamily,$(call HdlGetPart,$t)))))
# Make copies of the container for each target dir changing
# the app module name from "ocpi_app" to the specific one.
$(Containers): $(ContainerRtl) Makefile
	$(AT)sed s/ocpi_app/$(Worker)/ $(ContainerRtl) > $@
all: $(ContainerCores)

################################################################################
# Generate the per-platform files into platform-specific target dirs
InitXilinx=. $(OCPI_XILINX_TOOLS_DIR)/settings64.sh > /dev/null
XilinxAfter=grep -i error $1.out|grep -v '^WARNING:'|grep -i -v '[_a-z]error'; \
	     if grep -q $2 $1.out; then \
	       echo Time: `cat $1.time`; \
	       exit 0; \
	     else \
	       exit 1; \
	     fi
# Use default pattern to find error string in tool output
DoXilinx=$(call DoXilinxPat,$1,$2,$3,'Number of error.*s: *0')
DoXilinxPat=\
	echo " "Details in $1.out; cd $(call PlatformDir,$2); $(InitXilinx); \
	echo Command: $1 $3 > $1.out; \
	/usr/bin/time -f %E -o $1.time sh -c "$1 $3; echo $$? > $1.status" >> $1.out 2>&1;\
	(echo -n Time:; cat $1.time) >> $1.out; \
	$(call XilinxAfter,$1,$4)
PlatformDir=$(OutDir)target-$1
AppName=$(Worker)-$1
#AppBaseName=$(PlatformDir)/$(Worker)-$(HdlPlatform)
PromName=$(call PlatformDir,$1)/$(call AppName,$1).mcs
BitName=$(call PlatformDir,$1)/$(call AppName,$1).bit
ContainerXmlFile=$(Worker)_cont.xml
# FIXME: allow for multiple container mappings, possible platform-specific, possibly not
ArtifactXmlName=$(call PlatformDir,$1)/$(Worker)_cont_art.xml
ArtifactXmlDirs=$(XmlIncludeDirs) ../../devices/specs ../../devices/lib/hdl
BitZName=$(call PlatformDir,$1)/$(call AppName,$1).bit.gz
NgdName=$(call PlatformDir,$1)/$(call AppName,$1).ngd
NgcName=$(call PlatformDir,$1)/$(call AppName,$1).ngc
AppNgcName=$(call PlatformDir,$1)/$(ContainerModule).ngc
MapName=$(call PlatformDir,$1)/$(call AppName,$1)_map.ncd
ParName=$(call PlatformDir,$1)/$(call AppName,$1)_par.ncd
ChipScopeName=$(call PlatformDir,$1)/$(call AppName,$1)_csi.ngc
PcfName=$(call PlatformDir,$1)/$(call AppName,$1).pcf
TopNgcName=$(HdlPlatformsDir)/$1/target-$(call HdlGetPart,$1)/$1.ngc

define DoPlatform

$(call NgdName,$1): $(call AppNgcName,$1) $(HdlPlatformsDir)/$1/$1.ucf $(call TopNgcName,$1) | $(call PlatformDir,$1)
	$(AT)echo -n For $(Worker) on $1: creating NGD '(Xilinx Native Generic Database)' file using '"ngdbuild"'.
	$(AT)$(call DoXilinx,ngdbuild,$1,\
	        -aul -aut -uc $(HdlPlatformsDir)/$1/$1.ucf -p $(HdlPart_$1) -sd ../target-$(call HdlGetFamily,$(call HdlGetPart,$1)) \
	        $$(call FindRelative,$(call PlatformDir,$1),$(call TopNgcName,$1)) $(notdir $(call NgdName,$1)))
	$(AT)$(call DoXilinx,ngcbuild,$1,\
	        -aul -aut -uc $(HdlPlatformsDir)/$1/$1.ucf -p $(HdlPart_$1) -sd ../target-$(call HdlGetFamily,$(call HdlGetPart,$1)) \
	        $$(call FindRelative,$(call PlatformDir,$1),$(call TopNgcName,$1)) $(notdir $(call NgcName,$1)))
# Map to physical elements
$(call MapName,$1): $(call NgdName,$1)
	$(AT)echo -n For $(Worker) on $1: creating mapped NCD '(Native Circuit Description)' file using '"map"'.
	$(AT)$(call DoXilinx,map,$1,-p $(HdlPart_$1) -w -logic_opt on -xe c -mt on -t $(or $(OCPI_PAR_SEED),1) -register_duplication on \
	                         -global_opt off -ir off -pr off -lc off -power off -o $(notdir $(call MapName,$1)) \
	                         $(notdir $(call NgdName,$1)) $(notdir $(call PcfName,$1)))

# Place-and-route, and generate timing report
$(call ParName,$1): $(call MapName,$1) $(call PcfName,$1)
	$(AT)echo -n For $(Worker) on $1: creating PAR\'d NCD file using '"par"'.
	$(AT)$(call DoXilinx,par,$1,-w -xe c $(notdir $(call MapName,$1)) \
		$(notdir $(call ParName,$1)) $(notdir $(call PcfName,$1)))
	$(AT)echo -n Generating timing report '(TWR)' for $1 platform design.
	$(AT)-$(call DoXilinx,trce,$1,-v 20 -fastpaths -xml fpgaTop.twx \
		-o fpgaTop.twr \
		$(notdir $(call ParName,$1)) $(notdir $(call PcfName,$1)))

# Generate bitstream
$(call BitName,$1): $(call ParName,$1) $(call PcfName,$1)
	$(AT)echo -n For $(Worker) on $1: Generating bitstream file $$@.
	$(AT)$(call DoXilinxPat,bitgen,$1,-f $(HdlPlatformsDir)/common/bitgen_bit.ut \
                $(notdir $(call ParName,$1)) $(notdir $(call BitName,$1)) \
		$(notdir $(call PcfName,$1)), 'DRC detected 0 errors')

# Different since it is in the targetdir
$(call ArtifactXmlName,$1) $(call PlatformDir,$1)/$(Worker)_UUID.v: $(ContainerXmlFile) $(ImplXmlFile)
	@echo Generating artifact xml file \($(call ArtifactXmlName,$1)\) from $(ImplXmlFile) and $(ContainerXmlFile) files.
	$(AT)$(DYN_PREFIX) $(ToolsDir)/ocpigen -M $(call PlatformDir,$1)/$(notdir $(call ArtifactXmlName,$1)).deps \
	  -D $(call PlatformDir,$1) $(ArtifactXmlDirs:%=-I%) -A \
          -c $(ContainerXmlFile) -P $1 -e $(call HdlGetPart,$1) $(ImplXmlFile)


$(call BitZName,$1): $(call BitName,$1) $(call ArtifactXmlName,$1)
	$(AT)echo Making compressed bit file: $$@
	$(AT)gzip -c $(call BitName,$1) > $$@
	$(AT)$(ToolsDir)/../../scripts/addmeta $(call ArtifactXmlName,$1) $$@


-include $(HdlPlatformsDir)/$1/$1.mk
ifdef HdlPromArgs_$1
$(call PromName,$1): $(call BitName,$1)
	$(AT)echo -n For $(Worker) on $1: Generating PROM file $$@.
	$(AT)$(call DoXilinxPat,promgen,$1, -w -p mcs -c FF $$(HdlPromArgs_$1) $(notdir $(call BitName,$1)),'.*')
#$$(info have prom for $1=$$(HdlPromArgs_$1)=)
all: $(call PromName,$1)
endif

all: $(call BitZName,$1)

endef

$(foreach p,$(HdlPlatforms),$(eval $(call DoPlatform,$(p))))

endif # of else of ifdef HdlSimTool

clean::
	rm -r -f fpgaTop*.* *.out *.time *.status xst 

endif # end of HDL skip
endif # end of else of ifndef HdlPlatforms

