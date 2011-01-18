
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

# This Makefile type has two distinct modes.
# The first is to loop through multiple specified platforms and targets.
# This multi-target mode uses recursion, and is entered when there is a
# Targets or Platforms variable defined in the makefile or on the command line,
# and there is NO Target or Platform defined at all.
include $(OCPI_CDK_DIR)/include/util.mk
AT=@
MyComponentLibraries:=$(ComponentLibraries)
ifeq ($(Target)$(Platform),)
ifneq ($(Targets)$(Platforms),)
$(info Platforms are \"$(Platforms)\")
# We must recurse
all: targets platforms

targets:
	$(AT)for t in $(Targets); do $(MAKE) --no-print-directory Target=$$t target; done

platforms:
	$(AT)for p in $(Platforms); do $(MAKE) --no-print-directory Platform=$$p platform; done

clean::
	rm -r -f target-* platform-* gen imports
else
$(error No Targets or Platforms specified)
endif
else
AppName=$(CwdName)
Worker=$(AppName)
ifneq ($(and $(Platforms),$(Platform)),)
ifeq ($(strip $(filter $(Platform),$(Platforms))),)
$(info Application $(Worker) not built for platform $(Platform), only for platforms: $(Platforms))
Abort=yes
endif
endif
ifneq ($(and $(Targets),$(Target)),)
ifeq ($(strip $(filter $(Target),$(Targets))),)
$(info Application $(Worker) not built for target $(Target), only for targets: $(Targets))
Abort=yes
endif
endif
ifndef Abort
all: target
# This section of the makefile is for building the single target or platform
# Makefile for an hdl assembly, which is a lot like a worker...
# We run this for one platform/target
include $(OCPI_CDK_DIR)/include/hdl/hdl.mk

Application=yes
GeneratedSourceFiles+=$(GeneratedDir)/$(Worker)$(SourceSuffix)
#LibDir=$(OutDir)lib/hdl
# If there is a platform defined, we infer the target from it
ifdef Platform
all: platform
  ifdef Target
    ifeq ($(origin Target),$(origin Plaform))
      $(error You cannot specify a "Target" ($(Target))and a "Platform" ($(Platform)).  One or the other.)
    else ifeq ($(origin Platform),command line)
      Target=
    endif
  endif
  ifndef OCPI_HDL_PLATFORMS_DIR
    OCPI_HDL_PLATFORMS_DIR=$(OCPI_CDK_DIR)/lib/hdl/platforms
    ifeq ($(wildcard $(OCPI_HDL_PLATFORMS_DIR)),)
      OCPI_HDL_PLATFORMS_DIR=$(OCPI_BASE_DIR)/hdl/platforms
      ifeq ($(wildcard $(OCPI_HDL_PLATFORMS_DIR)/*),)
        $(error No HDL platforms found.  Looked in $(OCPI_CDK_DIR)/lib/hdl/platforms and $(OCPI_BASE_DIR)/hdl/platforms.)
      endif
    endif
  endif
  PlatformBase=$(firstword $(subst +, ,$(Platform)))
  PlatformSpecDir=$(OCPI_HDL_PLATFORMS_DIR)/$(PlatformBase)
  ifeq ($(wildcard $(PlatformSpecDir)/*),)
    $(error Platform $(Platform) not found in $(OCPI_HDL_PLATFORMS_DIR))
  endif
  ifeq ($(wildcard $(PlatformSpecDir)/$(PlatformBase).mk),)
    $(error File $(PlatformBase).mk not found in $(PlatformSpecDir))
  endif
  include $(PlatformSpecDir)/$(PlatformBase).mk
  ifndef Target
    Target=$(PART)
  endif
  ifndef Target
    $(error No Target specified, either directly or inferred from platform $(Platform)).
  endif
endif
Family:=$(call TargetFamily,$(Target))
$(info Target is $(Target), Family is $(Family))
include $(OCPI_CDK_DIR)/include/hdl/hdl-worker.mk
clean::
	rm -r -f target-* platform-* gen
BBName=$(Worker)_bb.v
BBFile=$(GeneratedDir)/$(BBName)
$(BBFile): | $(GeneratedDir)/$(Worker)_defs.vh
	$(AT)ln -s $(Worker)_defs.vh $@

BBLib=$(OutDir)target-$(Family)/$(Worker)/$(call LibraryFileTarget2,$(Family),$(Worker))
$(BBLib): LibName=$(Worker)
$(BBLib): Target=$(Family)
$(BBLib): TargetDir=target-$(Family)
$(BBLib): Cores=
$(BBLib): Core=$(Worker)
$(BBLib): Top=$(Worker)
$(BBLib): $(BBFile) | $(OutDir)target-$(Family)
	$(AT)echo Building stub/blackbox library for target \"$(Target)\" from \"$(BBFile)\"
	$(Compile)
	$(AT)rm -f $(TargetDir)/$(Worker)/$(Family); ln -s . $(TargetDir)/$(Worker)/$(Family)

CoreBaseName=$(OutDir)target-$(Target)/$(Worker)$(BF)
$(CoreBaseName): LibName=work
$(CoreBaseName): Top=$(Worker)
$(CoreBaseName): Core=$(Worker)
$(CoreBaseName): Cores=
$(CoreBaseName): ComponentLibraries=$(MyComponentLibraries)
$(CoreBaseName): $(BBLib)
$(CoreBaseName): TargetDir=target-$(PART)
# Put this into xst.mk..
$(CoreBaseName): $(foreach i,$(ComponentLibraries),$(i)/lib/hdl/$(call LibraryAccessTarget,$(Family)))
CoreLink=$(OutDir)target-$(Family)/$(Worker)/$(PART)
$(CoreLink):
	$(AT)ln -s ../../target-$(PART) $@	

target: $(CoreBaseName) | $(CoreLink)
ifdef Platform
  $(info Target for platform $(Platform) is $(Target))
  PlatformDir=$(OutDir)target-$(PlatformBase)
  $(PlatformDir): | $(OutDir)
	mkdir $@
  AppBaseName=$(PlatformDir)/$(Worker)-$(Platform)
  PromName=$(AppBaseName).mcs
  BitName=$(AppBaseName).bit
  NgdName=$(AppBaseName).ngd
  NgcName=$(PlatformDir)/mkOCApp4B.ngc
  MapName=$(AppBaseName)_map.ncd
  ParName=$(AppBaseName)_par.ncd
  ChipScopeName=$(AppBaseName)_csi.ngc
  PcfName=$(AppBaseName).pcf


ifdef PROMARGS
platform: $(PromName) 
else
platform: $(BitName)
endif
ifndef Container
Container=mkOCApp-noADC-3w.v
endif
ContainerRtl=$(OCPI_HDL_PLATFORMS_DIR)/../containers/$(Container)
# FIXME:  App must specify this wrapper file until...?
APP_RTL=$(PlatformDir)/mkOCApp.v
$(APP_RTL): $(ContainerRtl) Makefile | $(PlatformDir)
	$(AT)sed s/ocpi_app/$(Worker)/ $(ContainerRtl) > $@

ifndef OCPI_XILINX_TOOLS_DIR
XilinxVersions=$(shell echo $(wildcard /opt/Xilinx/*/ISE_DS) | tr ' ' '\n' | sort -r)
OCPI_XILINX_TOOLS_DIR=$(firstword $(XilinxVersions))
endif
InitXilinx=. $(OCPI_XILINX_TOOLS_DIR)/settings64.sh > /dev/null
XilinxAfter=grep -i error $1.out|grep -v '^WARNING:'|grep -i -v '[_a-z]error'; \
	     if grep -q $2 $1.out; then \
	       echo Time: `cat $1.time`; \
	       exit 0; \
	     else \
	       exit 1; \
	     fi
# Use default pattern to find error string in tool output
DoXilinx=$(call DoXilinxPat,$1,$2,'Number of error.*s: *0')
DoXilinxPat=\
	echo " "Details in $1.out; cd $(PlatformDir); $(InitXilinx); \
	echo Command: $1 $2 > $1.out; \
	/usr/bin/time -f %E -o $1.time sh -c "$1 $2; echo $$? > $1.status" >> $1.out 2>&1;\
	(echo -n Time:; cat $1.time) >> $1.out; echo -n Time:; cat $1.time; \
	$(call XilinxAfter,$1,$3)

# This creates a path for the bb library
$(PlatformDir)/$(Worker):
	$(AT)mkdir $@
	$(AT)ln -s ../../target-$(Target)/$(Worker) $@/$(Family)

# This creates a path for the core search (-sd)
$(PlatformDir)/$(Target): | $(PlatformDir)
	$(AT)mkdir $@
	$(AT)ln -s ../../target-$(Target)/ $@

# This is the container core.  One level above the app.
$(NgcName): Top=mkOCApp4B
$(NgcName): | $(PlatformDir)/$(Worker)
$(NgcName): Core=mkOCApp4B
$(NgcName): LibName=work
$(NgcName): Cores=target-$(Family)/$(Worker)
$(NgcName): Libraries=
#$(NgcName): ComponentLibraries=
$(NgcName): TargetDir=$(PlatformDir)
$(NgcName): $(BBLib) $(CoreBaseName) $(APP_RTL) | $(PlatformDir)/$(Target) $(CoreLink)
	$(AT)echo Building $@: container core for application '"$(AppName)"' on platform '"$(Platform)"' using '"xst"'
	$(AT)$(Compile)

#$(TopNgcName): $(NgcName)
#	$(AT)echo Building top-level ngc for application '"$(AppName)"' on platform '"$(Platform)"' using '"xst"' tar =$@= +$(NgcName)+
#	$(AT)$(call DoXilinx,ngcbuild,-sd . \
                $(foreach l,$(ComponentLibraries),-sd $(call FindRelative,$(PlatformDir),$(l)/lib/hdl/$(Target)))\
		$(foreach c,$(Cores),-sd $(call FindRelative,$(PlatformDir),$(call CoreRefDir,$(c),$(Target))))\
	        -aul -aut -uc $(PlatformSpecDir)/$(Platform).ucf -p $(Target) $(notdir $(ChipScopeName)) $(notdir $(NgdName)))

#$(NgcName): XstExtraOptions=-iobuf yes -bufg 32
#$(NgcName): XstInternalOptions=$(if $(wildcard $(PlatformSpecDir)/$(Platform).xcf),-uc $(PlatformSpecDir)/$(Platform).xcf,)

#$(AT)($(foreach i,$(ALL_RTLS),echo verilog work "$(call FindRelative,$(PlatformDir),$(i))";)) > $(PlatformDir)/fpgaTop.prj
#$(AT)(echo bsv=$(call FindRelative,$(PlatformDir),$(OCPI_CDK_DIR)/lib/hdl/bsv/$(Family)) ;\
#      echo util=$(call FindRelative,$(PlatformDir),$(OCPI_CDK_DIR)/lib/hdl/util/$(Family)); \
#	      echo util_$(Family)=$(call FindRelative,$(PlatformDir),$(OCPI_CDK_DIR)/lib/hdl/util_$(Family)/$(Family)); \
#	      echo app=$(call FindRelative,$(PlatformDir),$(TargetDir)/work)) > $(PlatformDir)/fpgaTop.ini
#	$(AT)(echo bsv; echo util; echo util_$(Family); echo app; echo work) > $(PlatformDir)/fpgatop.lso
#$(call DoXilinx,xst,-ifn $(PlatformSpecDir)/$(Platform).xst)

# Chipscope insertion (optional)
$(ChipScopeName): $(NgcName)
	$(AT)$(if $(DoChipScope), \
	         echo -n For $(Worker) on $(Platform): inserting chipscope into $< to produce $@ using '"inserter"'.; \
	         $(call DoXilinx,inserter,-insert $(CSIN) -p $(Target) -i $(notdir $(NgcName)) $(notdir $(ChipScopeName))), \
	         echo Skipping chipscope insertion as it is disabled. ; \
	         cp $< $@)

# Reduce synthesis to Xilinx primitives
TopNgcName=$(PlatformSpecDir)/target-$(Platform)/$(Platform).ngc
$(NgdName): Cores=
$(NgdName): $(NgcName) $(PlatformSpecDir)/$(Platform).ucf $(TopNgcName)
	$(AT)echo -n For $(Worker) on $(Platform): creating NGD '(Xilinx Native Generic Database)' file using '"ngdbuild"'.
	$(AT)$(call DoXilinx,ngdbuild,\
	        -aul -aut -uc $(PlatformSpecDir)/$(Platform).ucf -p $(Target) \
	        $(call FindRelative,$(PlatformDir),$(PlatformSpecDir)/target-$(Platform))/$(Platform).ngc $(notdir $(NgdName)))

# Map to physical elements
$(MapName): $(NgdName)
	$(AT)echo -n For $(Worker) on $(Platform): creating mapped NCD '(Native Circuit Description)' file using '"map"'.
	$(AT)$(call DoXilinx,map,-p $(Target) -w -logic_opt on -xe n -mt on -t 1 -register_duplication on \
	                         -global_opt off -ir off -pr off -lc off -power off -o $(notdir $(MapName)) \
	                         $(notdir $(NgdName)) $(notdir $(PcfName)))

# Place-and-route, and generate timing report
$(ParName): $(MapName) $(PcfName)
	$(AT)echo -n For $(Worker) on $(Platform): creating PAR\'d NCD file using '"par"'.
	$(AT)$(call DoXilinx,par,-w -xe n $(notdir $(MapName)) $(notdir $(ParName)) $(notdir $(PcfName)))
	$(AT)echo -n Generating timing report '(TWR)' for $(Platform) platform design.
	$(AT)-$(call DoXilinx,trce,-v 20 -fastpaths -xml fpgaTop.twx -o fpgaTop.twr $(notdir $(ParName)) $(notdir $(PcfName)))

# Generate bitstream
$(BitName): $(ParName) $(PcfName)
	$(AT)echo -n For $(Worker) on $(Platform): Generating bitstream file $@.
	$(AT)$(call DoXilinxPat,bitgen,-f $(OCPI_HDL_PLATFORMS_DIR)/common/bitgen_bit.ut \
	                               $(notdir $(ParName)) $(notdir $(BitName)) $(notdir $(PcfName)),\
	                               'DRC detected 0 errors')

$(PromName): $(BitName) | $(PlatformDir)
	$(AT)echo -n For $(Worker) on $(Platform): Generating PROM file $@.
	$(AT)$(call DoXilinxPat,promgen, -w -p mcs -c FF $(PROMARGS) $(notdir $(BitName)),'.*')

clean::
	rm -r -f fpgaTop*.* *.out *.time *.status xst 



endif

ifneq (,)
$(LibDir): | $(OutDir)lib
$(LibDir)/$(Target): | $(LibDir)
$(LibDir) $(OutDir)lib $(LibDir)/$(Target) $(GeneratedDir)/hdl:
	mkdir $@

all: $(OutDir)/$(Target)/$(Worker).ngc

$(LibDir)/$(Target)/$(Worker).ngc:
	ln -s $(notdir $(BinaryFile)) $@
all: | $(LibDir)/$(Target)
endif
CompiledSourceFiles+=$(GeneratedDir)/$(Worker)$(SourceSuffix)
#$(info XML1 $(XmlIncludeDirs))
override XmlIncludeDirs += $(ComponentLibraries:%=%/lib/hdl) $(ComponentLibraries:%=%/lib) $(OCPI_HDL_PLATFORMS_DIR)/common
#$(info XML2 $(XmlIncludeDirs))
# We generate the verilog assembly based on the "implementation XML" which is HdlAssembly
$(GeneratedSourceFiles): $(ImplXmlFiles) | $(GeneratedDir)
	$(AT)echo Generating the application source file: $@
	$(AT)$(OcpiGen) -a  $<
	$(AT)mv $(GeneratedDir)/$(Worker)_assy.v $@


ifdef Containers
define doContainer
$(GeneratedDir)/$(1)_art.xml: override XmlIncludeDirs += ../../devices/specs
$(GeneratedDir)/$(1)_art.xml: $(1).xml $(AppName).xml
	$(AT)echo Generating the container/bitstream runtime xml file: $$@
	$(AT)$$(OcpiGen) -A -h $(1).xml $(AppName).xml

all: $(GeneratedDir)/$(1)_art.xml
endef
$(foreach c,$(Containers),$(eval $(call doContainer,$(c))))
endif

CleanFiles += $(OutDir)lib
# Build the stub library
ifneq (,)
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

endif
endif
endif
