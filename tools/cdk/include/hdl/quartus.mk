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
#   / / / / __ \/ _ \/ __ \/ /   / /_/ /c / /  / __ \/ ___/ __ `/
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

# This file has the HDL tool details for altera quartus

include $(OCPI_CDK_DIR)/include/hdl/altera.mk

################################################################################
# $(call HdlToolLibraryRefFile,libname,target)
# Function required by toolset: return the file for dependencies on the library
# This is attached to a directory, with a slash
# Thus it is empty if the library is a diretory full of stuff that doesn't
# have a file name we can predict.

HdlToolLibraryRefFile=

################################################################################
# $(call HdlToolLibraryFile,target,libname)
# Function required by toolset: return the file to use as the file that gets
# built when the library is built or touched when the library is changed or rebuilt.
#
# For quartus, no precompilation is available, so it is just a directory
# full of links whose name is the name of the library
HdlToolLibraryFile=$(LibName)
################################################################################
# Function required by toolset: given a list of targets for this tool set
# Reduce it to the set of library targets.
#
# For quartus, it is generic since there is no processing
HdlToolLibraryTargets=altera
################################################################################
# Variable required by toolset: what is the name of the file or directory that
# is the thing created when a library is created. The thing that will be installed
HdlToolLibraryResult=$(LibName)
################################################################################
# Variable required by toolset: HdlToolCoreLibName
# What library name should we give to the library when a core is built from
# sources
HdlToolCoreLibName=$(Core)
################################################################################
# Variable required by toolset: HdlBin
# What suffix to give to the binary file result of building a core
HdlBin=.qxp
################################################################################
# Variable required by toolset: HdlToolRealCore
# Set if the tool can build a real "core" file when building a core
# I.e. it builds a singular binary file that can be used in upper builds.
# If not set, it implies that only a library containing the implementation is
# possible
HdlToolRealCore=yes
################################################################################
# Variable required by toolset: HdlToolNeedBB=yes
# Set if the tool set requires a black-box library to access a core
HdlToolNeedBB=
################################################################################
# Function required by toolset: $(call HdlToolLibRef,libname)
# This is the name after library name in a path
# It might adjust (genericize?) the target
#
HdlToolLibRef=$(or $3,$(call HdlGetFamily,$2))

# This kludgery is because it seems that Quartus cannot support entities and architectures in 
# separate files, so we have to combine them - UGH UGH UGH - I hope I'm wrong...
QuartusVHDLWorker=$(and $(findstring worker,$(HdlMode)),$(findstring VHDL,$(HdlLanguage)))
ifdef QuartusVHDLWorkerx
QuartusCombine=$(OutDir)target-$(HdlTarget)/$(Worker)-combine.vhd
QuartusSources=$(filter-out $(Worker).vhd $(ImplHeaderFiles),$(HdlSources)) $(QuartusCombine)
$(QuartusCombine): $(ImplHeaderFiles) $(Worker).vhd
	cat $(ImplHeaderFiles) $(Worker).vhd > $@
else
QuartusSources=$(HdlSources)
endif
# ?? What is the difference between SEARCH_PATH and USER_LIBRARIES???: answer is SEARCH_PATH is one at a time
# Libraries can be built for specific targets, which just is for syntax checking
# Note that a library can be designed for a specific target
QuartusFamily_stratix4:=Stratix IV
QuartusFamily_stratix5:=Stratix V
QuartusMakePart1=$(firstword $1)$(word 3,$1)$(word 2,$1)
QuartusMakePart=$(call QuartusMakePart1,$(subst -, ,$1))

# Make the file that lists the files in order when we are building a library
QuartusMakeExport= \
 $(and $(findstring library,$(HdlMode)), \
   (echo '\#' Source files in order for $(strip \
     $(if $(findstring $(HdlMode),library),library: $(LibName),core: $(Core))); \
    $(foreach s,$(QuartusSources),echo $(notdir $s);) \
   ) > $(LibName)-sources.mk;)

xxx=$(and $(ComponentLibraries),echo '\#' Search paths for component libraries;) \
    $(foreach l,$(ComponentLibraries),\
      echo set_global_assignment -name SEARCH_PATH '\"'$(strip \
      $(foreach found,\
        $(foreach t,$(sort $(HdlTarget) $(call HdlGetFamily,$(HdlTarget))),\
	  $(realpath $l/lib/hdl/$t)), \
        $(if $(found),\
          $(call FindRelative,$(TargetDir),$(found))'\"';,\
	  $(error No component library at $(abspath $t)))))) \
    $(eval HdlWorkers:=$$(strip $$(foreach i,$$(shell grep -v '\\\#' $$(ImplWorkersFile)),\
                         $$(if $$(filter $$(firstword $$(subst :, ,$$i)),$$(HdlPlatformWorkers)),,$$i))))

# Make the settings file
# Note that the local source files use notdir names and search paths while the
# remote libraries use pathnames so that you can have files with the same names.
QuartusMakeQsf=\
 if test -f $(Core).qsf; then cp $(Core).qsf $(Core).qsf.bak; fi; \
 $(and $(findstring $(HdlMode),library),\
   echo 'module onewire(input  W_IN, output W_OUT); assign W_OUT = W_IN; endmodule' > onewire.v;) \
 (echo '\#' Common assignments whether a library or a core; \
  echo set_global_assignment -name FAMILY '\"'$(QuartusFamily_$(call HdlGetFamily,$(HdlTarget)))'\"'; \
  echo set_global_assignment -name DEVICE $(call ToUpper,$(if $(findstring $(HdlTarget),$(HdlAllFamilies)),AUTO,\
                                            $(if $(findstring $(HdlMode),platform assembly container),\
                                             $(call QuartusMakePart,$(HdlPart_$(HdlPlatform))),\
                                             $(HdlTarget)))); \
  echo set_global_assignment -name TOP_LEVEL_ENTITY $(or $(Top),$(Core)); \
  \
  echo '\# Search path(s) for local files'; \
  $(foreach d,$(call Unique,$(patsubst %/,%,$(dir $(QuartusSources)) $(VerilogIncludeDirs))), \
    echo set_global_assignment -name SEARCH_PATH '\"'$(strip \
     $(call FindRelative,$(TargetDir),$d))'\"';) \
  \
  $(and $(HdlLibrariesInternal),echo '\#' Assignments for adding libraries to search path;) \
  $(foreach l,$(HdlLibrariesInternal),\
    $(foreach hlr,$(call HdlLibraryRefDir,$l,$(HdlTarget)),\
      $(if $(realpath $(hlr)),,$(error No altera library for $l at $(abspath $(hlr))))\
      echo set_global_assignment -name SEARCH_PATH '\"'$(call FindRelative,$(TargetDir),$(hlr))'\"'; \
      $(foreach f,$(wildcard $(hlr)/*_pkg.vhd),\
        echo set_global_assignment -name VHDL_FILE -library $(notdir $l) '\"'$f'\"';\
        $(foreach b,$(subst _pkg.vhd,_body.vhd,$f),\
          $(and $(wildcard $b),\
	     echo set_global_assignment -name VHDL_FILE -library $(notdir $l) '\"'$b'\"';))))) \
  \
  echo '\#' Assignment for local source files using search paths above; \
  $(foreach s,$(QuartusSources), \
    echo set_global_assignment -name $(if $(filter %.v,$s),VERILOG_FILE,VHDL_FILE -library $(LibName)) \
       '\"'$(notdir $s)'\"';) \
  \
  $(and $(filter assembly container,$(HdlMode)), \
    echo '\#' Import qxp files for each worker used in the assembly; \
    $(eval $(HdlSetWorkers)) \
    $(foreach w,$(HdlWorkers),\
      $(foreach f,$(call HdlFindWorkerCoreFile,$w),\
       echo set_global_assignment -name QXP_FILE '\"'$(call FindRelative,$(TargetDir),$f)'\"';)) \
    echo '\#' Define partitions for each worker instance in the assembly; \
    $(foreach i,$(HdlInstances),\
      $(foreach f,$(call HdlFindWorkerCoreFile,$(firstword $(subst :, ,$i))),\
        echo set_global_assignment -name PARTITION_IMPORT_FILE \
        '\"'$(call FindRelative,$(TargetDir),$f)'\"'\
        -section_id '\"'$i'\"';\
      echo set_instance_assignment -name PARTITION_HIERARCHY db/$(subst :,_,$i) -to '\"$i\"' \
      -section_id '\"'$i'\"';)))\
  $(and $(Cores),echo '\#' Import QXP file for each core;) \
  $(foreach l,$(Cores),\
    echo set_global_assignment -name QXP_FILE \
      '\"'$(call FindRelative,$(TargetDir),$(call HdlCoreRef,$l,$(HdlTarget)))'\"';)\
  $(and $(findstring $(HdlMode),platform),\
    echo '\#' Make sure the container is defined as an empty partition. ;\
    echo set_instance_assignment -name PARTITION_HIERARCHY container -to '\"mkFTop_alst4:ftop|mkCTop4B:ctop|mkOCApp4B:app\"' \
      -section_id '\"'container'\"'; \
    echo set_global_assignment -name PARTITION_NETLIST_TYPE -section_id '\"'container'\"' EMPTY; \
    echo set_instance_assignment -name PARTITION_HIERARCHY root_partition -to '|' \
      -section_id Top; \
    echo set_global_assignment -name PARTITION_NETLIST_TYPE -section_id '\"'Top'\"' POST_SYNTH; ) \
  $(if $(findstring $(HdlMode),core worker platform assembly container),\
    echo '\#' Assignments for building cores; \
    echo set_global_assignment -name AUTO_EXPORT_INCREMENTAL_COMPILATION on; \
    echo set_global_assignment -name INCREMENTAL_COMPILATION_EXPORT_FILE $(Core)$(HdlBin); \
    echo set_global_assignment -name INCREMENTAL_COMPILATION_EXPORT_NETLIST_TYPE POST_SYNTH;) \
  $(if $(findstring $(HdlMode),library),\
    echo '\#' Assignments for building libraries; \
    echo set_global_assignment -name SYNTHESIS_EFFORT fast;) \
  $(if $(findstring $(HdlMode),platform),\
    echo '\#' Include the platform-related assignments. ;\
    echo source ../$(Worker).qsf;) \
 ) > $(Core).qsf;
# Be safe for now - remove all previous stuff
HdlToolCompile=\
  echo '  'Creating $@ with top == $(Top)\; details in $(TargetDir)/quartus-$(Core).out.;\
  rm -r -f db incremental_db $(Core).qxp $(Core).*.rpt $(Core).*.summary $(Core).qpf $(Core).qsf $(notdir $@); \
  $(QuartusMakeExport) $(QuartusMakeQsf) cat -n $(Core).qsf;\
  set -e; $(call OcpiDbgVar,HdlMode,xc) \
  $(if $(findstring $(HdlMode),core worker platform assembly container),\
    $(call DoAltera,quartus_map --write_settings_files=off $(Core)); \
    $(call DoAltera,quartus_cdb --merge --write_settings_files=off $(Core)); \
    $(call DoAltera,quartus_cdb --incremental_compilation_export --write_settings_files=off $(Core))) \
  $(if $(findstring $(HdlMode),library),\
    $(call DoAltera,quartus_map --analysis_and_elaboration --write_settings_files=off $(Core))); \

ifdef sdf
# We can't trust xst's exit code so we conservatively check for zero errors
# Plus we create the edif all the time...
HdlToolPost=\
  if test $$HdlExit == 0; then \
    touch $(LibName);\
  fi;
endif

# When making a library, quartus still wants a "top" since we can't precompile 
# separately from synthesis (e.g. it can't do what vlogcomp can with isim)
# Need to be conditional on libraries
ifeq ($(HdlMode),library)
ifndef Core
Core=onewire
Top=onewire
endif
endif

HdlToolFiles=\
  $(foreach f,$(QuartusSources),\
     $(call FindRelative,$(TargetDir),$(dir $f))/$(notdir $f))

# To create the "library result", we create a directory full of source files
# that have the quartus library directive inserted as the first line to
# force them into the correct library when they are "discovered" via SEARCH_PATH.
ifeq ($(HdlMode),library)
HdlToolPost=\
  if test $$HdlExit = 0; then \
    if ! test -d $(LibName); then \
      mkdir $(LibName); \
    else \
      rm -f $(LibName)/*; \
    fi;\
    for s in $(HdlToolFiles); do \
      if [[ $$s == *.vhd ]]; then \
        echo -- synthesis library $(LibName) | cat - $$s > $(LibName)/`basename $$s`; \
      else \
        ln -s ../$$s $(LibName); \
      fi; \
    done; \
  fi;
endif

################################################################################
# Final bitstream building support, given that the "container" core is built
# HdlToolDoPlatform takes platform as $1
QuartusMakeTopQsf=\
 if test -f $(Worker).qsf; then cp $(Worker).qsf $(Worker).qsf.bak; fi; \
 (echo '\#' Common assignments whether a library or a core; \
  echo set_global_assignment -name FAMILY '\"'$(QuartusFamily_$(call HdlGetFamily,$1))'\"'; \
  echo set_global_assignment -name DEVICE $(call ToUpper,$(call QuartusMakePart,$(HdlPart_$1))); \
  echo set_global_assignment -name TOP_LEVEL_ENTITY fpgaTop; \
  echo set_global_assignment -name QXP_FILE '"'$(HdlPlatformsDir)/$1/target-$(call HdlGetPart,$1)/$1$(HdlBin)'"'; \
   echo set_global_assignment -name SDC_FILE '"'$(HdlPlatformsDir)/$1/$1.sdc'"'; \
  echo set_global_assignment -name PARTITION_IMPORT_FILE '"'$(ContainerModule)$(HdlBin)'"' \
        -section_id container; \
 echo set_global_assignment -name PARTITION_ALWAYS_USE_QXP_NETLIST -section_id container On\
 ) > $(call AppName,$1).qsf

# echo set_global_assignment -name QXP_FILE '"'$(ContainerModule)$(HdlBin)'"'; \
#  echo set_instance_assignment -name PARTITION_HIERARCHY db/container -to '"'fpgaTop|mkFTop_alst4:ftop|mkCTop4B:ctop|mkOCApp4B:app'"' \
        -section_id container; \
#  echo set_instance_assignment -name PARTITION_ALWAYS_USE_QXP_NETLIST -to '"'fpgaTop|mkFTop_alst4:ftop|mkCTop4B:ctop|mkOCApp4B:app'"' -section_id container On\
#  echo set_global_assignment -name PARTITION_IMPORT_FILE '"'$(ContainerModule)$(HdlBin)'"' \
        -section_id container; \
#  echo set_instance_assignment -name PARTITION_HIERARCHY db/container -to '"'mkFTop_alst4:ftop|mkCTop4B:ctop|mkOCApp4B:app'"' \
        -section_id container; \
#  echo set_global_assignment -name PARTITION_IMPORT_FILE '"'$(HdlPlatformsDir)/$1/target-$(call HdlGetPart,$1)/$1$(HdlBin)'"' \
        -section_id Top; \
#  echo set_instance_assignment -name PARTITION_HIERARCHY db/top -to '|' -section_id Top; \
# echo 'module onewire(input  W_IN, output W_OUT); assign W_OUT = W_IN; endmodule' > onewire.v; \
#  echo set_global_assignment -name VERILOG_FILE onewire.v; \
#  echo set_global_assignment -name PARTITION_IMPORT_FILE '"'$(ContainerModule)$(HdlBin)'"' -section_id "app"; \
  echo set_global_assignment -name PARTITION_HIERARCHY dp/app -to '"mkFTop_alst4:ftop|mkCTop4B:ctop|mkOCApp4B:app"' -section_id "app"; \
  echo set_global_assignment -name PARTITION_IMPORT_FILE '"'$(HdlPlatformsDir)/$1/target-$(call HdlGetPart,$1)/$1$(HdlBin)'"' -section_id "plat"; \
  echo set_global_assignment -name PARTITION_HIERARCHY db/plat -to '"fpgaTop"' -section_id "plat" ; \

BitName=$(call PlatformDir,$1)/$(call AppName,$1).sof
QuartusCmd=\
	set -e; \
	rm -r -f db incremental_db ; \
	$(call QuartusMakeTopQsf,$1) ; \
	cp $(call AppName,$1).qsf $(call AppName,$1).qsf.pre-fit ; \
	$(call DoAltera,quartus_map $(call AppName,$1)); \
	$(call DoAltera,quartus_cdb --merge=on --override_partition_netlist_type=container=import --write_settings_files=off $(call AppName,$1)); \
	$(call DoAltera,quartus_fit $(call AppName,$1)); \
	QuartusStatus=$$$$? ; echo QuartusStatus after fit is $$$$QuartusStatus; \
	cp $(call AppName,$1).qsf $(call AppName,$1).qsf.post-fit; \
	$(call DoAltera,quartus_asm $(call AppName,$1))


#	QuartusStatus=$$$$? ; echo QuartusStatus after asm is $$$$QuartusStatus


define HdlToolDoPlatform

# Generate bitstream
$$(call BitName,$1): override TargetDir=$(call PlatformDir,$1)
$$(call BitName,$1): HdlToolCompile=$(QuartusCmd)
$$(call BitName,$1): HdlToolSet=quartus
$$(call BitName,$1): override HdlTarget=$(call HdlGetFamily,$1)
$$(call BitName,$1): $(HdlPlatformsDir)/$1/target-$(call HdlGetPart,$1)/$1$(HdlBin) \
	             $(OutDir)target-$1/$(ContainerModule)$(HdlBin)
	$(AT)echo Creating Quartus/$$(HdlTarget) bitstream: $$@.  Details in $$(call AppName,$1).out
	$(AT)$$(HdlCompile)

endef

