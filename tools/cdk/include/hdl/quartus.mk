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
HdlToolCoreLibName=work
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

# ?? What is the difference between SEARCH_PATH and USER_LIBRARIES???
# Libraries can be built for specific targets, which just is for syntax checking
# Note that a library can be designed for a specific target
QuartusFamily_stratix4:=Stratix IV
QuartusFamily_stratix5:=Stratix V
QuartusMakePart1=$(firstword $1)$(word 3,$1)$(word 2,$1)
QuartusMakePart=$(call QuartusMakePart1,$(subst -, ,$1))
QuartusMakeQsf=\
 if test -f $(Core).qsf; then cp $(Core).qsf $(Core).qsf.bak; fi; \
 (echo '\#' Common assignments whether a library or a core; \
  echo set_global_assignment -name FAMILY '\"'$(QuartusFamily_$(call HdlGetFamily,$(HdlTarget)))'\"'; \
  echo set_global_assignment -name DEVICE $(if $(findstring $(HdlTarget),$(HdlAllFamilies)),AUTO,\
                                            $(if $(HdlExactPart),\
                                             $(call QuartusMakePart,$(HdlExactPart)),\
                                             $(HdlTarget))); \
  echo set_global_assignment -name TOP_LEVEL_ENTITY $(or $(Top),$(Core)); \
  $(and $(VerilogIncludeDirs),echo '\#' Search paths for verilog includes;) \
  $(foreach d,$(VerilogIncludeDirs),\
    echo set_global_assignment -name SEARCH_PATH '\"'$(call FindRelative,$(TargetDir),$d)'\"';) \
  $(and $(Libraries),echo '\#' Search paths for primitive libraries;) \
  $(foreach l,$(Libraries),\
    echo set_global_assignment -name SEARCH_PATH '\"'$(strip \
    $(foreach hlr,$(call HdlLibraryRefDir,$l,$(HdlTarget)),\
      $(if $(realpath $(hlr)),,$(error No altera library at $(abspath $(hlr))))\
        $(call FindRelative,$(TargetDir),$(hlr))))'\"';)\
  $(and $(ComponentLibraries),echo '\#' Search paths for component libraries;) \
  $(foreach l,$(ComponentLibraries),\
    echo set_global_assignment -name SEARCH_PATH '\"'$(strip \
    $(foreach found,\
      $(foreach t,$(sort $(HdlTarget) $(call HdlGetFamily,$(HdlTarget))),\
	$(realpath $l/lib/hdl/$t)), \
      $(if $(found),\
        $(call FindRelative,$(TargetDir),$(found))'\"';,\
	$(error No component library at $(abspath $t))))))\
  echo '\#' Assignments for source files; \
  $(foreach s,$(CompiledSourceFiles),$(strip \
     echo set_global_assignment -name VERILOG_FILE \
       '\"'$(call FindRelative,$(TargetDir),$(dir $s))/$(notdir $s)'\"';)) \
  $(if $(AssemblyWorkers), \
     echo '\#' Import file names for assembly workers; \
    $(foreach l,$(sort $(foreach w,$(AssemblyWorkers),$(firstword $(subst :, ,$w)))), \
      echo set_global_assignment -name QXP_FILE \
        '\"'$(call FindRelative,$(TargetDir),$(ComponentLibraries)/lib/hdl/$(HdlTarget)/$l$(HdlBin))'\"';)\
    $(foreach l,$(AssemblyWorkers),\
      echo set_global_assignment -name PARTITION_IMPORT_FILE \
        '\"'$(call FindRelative,$(TargetDir),$(ComponentLibraries)/lib/hdl/$(HdlTarget)/$(firstword $(subst :, ,$l))$(HdlBin))'\"' \
        -section_id '\"'$l'\"';\
      echo set_global_assignment -name PARTITION_HIERARCHY db/$(subst :,_,$l) -to '\"'$l'\"' \
      -section_id '\"'$l'\"';))\
  $(and $(Cores),echo '\#' Import file names for cores;) \
  $(foreach l,$(Cores),\
    echo set_global_assignment -name QXP_FILE \
    $(foreach found,$(firstword \
                       $(foreach c,$(call HdlCoreRefDir,$l,$(HdlTarget)) \
                                   $(call HdlCoreRefDir,$l,$(call HdlGetFamily,$(HdlTarget))),\
                                 $(if $(realpath $c),$c))),\
	$(if $(found),\
           $(call FindRelative,$(TargetDir),$(found))/$l.qxp,\
           $(error No altera core ($l.qxp) at $(abspath $(call HdlCoreRefDir,$l,$(call HdlGetFamily,$(HdlTarget)))))));)\
  $(if $(findstring $(HdlMode),core worker platform application),\
    echo '\#' Assignments for building cores; \
    echo set_global_assignment -name AUTO_EXPORT_INCREMENTAL_COMPILATION on; \
    echo set_global_assignment -name INCREMENTAL_COMPILATION_EXPORT_FILE $(Core)$(HdlBin); \
    echo set_global_assignment -name INCREMENTAL_COMPILATION_EXPORT_NETLIST_TYPE POST_SYNTH;) \
  $(if $(findstring $(HdlMode),library),\
    echo '\#' Assignments for building libraries; \
    echo set_global_assignment -name SYNTHESIS_EFFORT fast;) \
 ) > $(Core).qsf;

# Be safe for now - remove all previous stuff
HdlToolCompile=\
  echo '  'Creating $@ with top == $(Top)\; details in $(TargetDir)/quartus-$(Core).out.;\
  rm -r -f db incremental_db *.qxp *.rpt *.summary *.qpf *.qdf $(notdir $@); \
  $(QuartusMakeQsf) cat -n $(Core).qsf;\
  set -e; $(call OcpiDbgVar,HdlMode,xc) \
  $(if $(findstring $(HdlMode),core worker platform application),\
    $(call DoAltera,quartus_map --write_settings_files=off $(Core)); \
    $(call DoAltera,quartus_cdb --merge --write_settings_files=off $(Core)); \
    $(call DoAltera,quartus_cdb --incremental_compilation_export --write_settings_files=off $(Core))) \
  $(if $(findstring $(HdlMode),library),\
    $(call DoAltera,quartus_map --analysis_and_elaboration --write_settings_files=off $(Core))); \

#    $(call DoAltera,quartus_map --analysis_and_elaboration --write_settings_files=off $(Core)); \
#    $(call DoAltera,quartus_cdb --incremental_compilation_import=on --write_settings_files=off $(Core)); \
# When making a library, quartus still wants a "top" since we can't precompile 
# separately from synthesis (e.g. it can't do what vlogcomp can with isim)
# Need to be conditional on libraries
ifeq ($(HdlMode),library)
ifndef Core
Core=onewire
Top=onewire
endif
CompiledSourceFiles:= $(OCPI_CDK_DIR)/include/hdl/onewire.v $(CompiledSourceFiles)
endif
# We are a tool that really has no layered building at all for libraries
$(eval $(call HdlSimNoLibraries))
HdlToolFiles=\
  $(SimFiles) \
  $(foreach f,$(HdlSources),\
     $(call FindRelative,$(TargetDir),$(dir $f))/$(notdir $f))
