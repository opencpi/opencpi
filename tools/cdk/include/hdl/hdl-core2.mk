# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

# Internal file shared between primitive cores and workers
# This file basically imports a hard core or builds one from a library

ifndef Tops
  ifdef Top
    Tops:=$(Top)
  else
    Tops:=$(Core)
  endif
endif
ifndef LibName
  LibName:=$(word 1,$(HdlCores))
endif
ifndef WorkLib
  WorkLib:=$(LibName)
endif
$(call OcpiDbgVar,HdlToolRealCore)
$(call OcpiDbgVar,LibName)

ifndef
JOB_STARTTIME:=$(shell date +"%H:%M:%S")
endif

ifdef HdlToolRealCore
  ################################################################################
  # Build the real core if the tools can do it $(call DoCore,target,core,top,config)
  define DoCore
    CoreResults+=$(call WkrTargetDir,$1,$4)/$2$(HdlBin)
    $(call WkrTargetDir,$1,$4)/$2$(HdlBin): override HdlTarget:=$1
    $(call WkrTargetDir,$1,$4)/$2$(HdlBin): TargetDir=$(call WkrTargetDir,$1,$4)
    $(call WkrTargetDir,$1,$4)/$2$(HdlBin): | $$$$(TargetDir)
    $(call WkrTargetDir,$1,$4)/$2$(HdlBin): Core=$2
    $(call WkrTargetDir,$1,$4)/$2$(HdlBin): LibName=$(call HdlRmRv,$(word 1,$(HdlCores)))$(if $(filter 0,$4),,_c$4)
#    $(call WkrTargetDir,$1,$4)/$2$(HdlBin): Top=$(or $(filter %_rv,$3),$3$(and $(WorkerParamNames),$(filter-out 0,$4),_c$4))
    $(call WkrTargetDir,$1,$4)/$2$(HdlBin): Top=$3$(and $(filter-out 0,$4),_c$4)
    $(call WkrTargetDir,$1,$4)/$2$(HdlBin): override HdlUsesRv:=$(and $5,1)
    $(call WkrTargetDir,$1,$4)/$2$(HdlBin): ParamConfig=$4
    ifdef PreBuiltCore
      $(call WkrTargetDir,$1,$4)/$2$(HdlBin): $(PreBuiltCore)
	$(AT)if test ! -L $@; then \
		$(ECHO) -n Establishing pre-built core file $(2)$(HdlBin) for; \
	        $(ECHO) ' 'target \"$(HdlTarget)\"; \
	     fi
	$(AT)$$(call MakeSymLink2,$(strip \
                $(PreBuiltCore)),$(strip \
                $$(TargetDir)),$(2)$(HdlBin))
	$(AT)$$(HdlSourceListCompile)
	$(AT)$$(if $$(HdlToolNeedsSourceList_$(HdlToolSet)),$$(HdlPost))

    else
      $(call OcpiDbgVar,CompiledSourceFiles)
      $$(call OcpiDbgVar,CompiledSourceFiles)

      $(call WkrTargetDir,$1,$4)/$2$(HdlBin): $$(call HdlTargetSrcFiles,$1,$4)
      $(call WkrTargetDir,$1,$4)/$2$(HdlBin): \
      HdlSources=$$(call Unique,$$(filter-out %.vh,$$(call HdlTargetSrcFiles,$1,$4) $$(call HdlShadowFiles,$1,$4)))

      $(infox TARGET:$(call WkrTargetDir,$1,$4)/$2$(HdlBin))
      $(call WkrTargetDir,$1,$4)/$2$(HdlBin): \
        $$$$(foreach l,$$$$(HdlLibrariesInternal),$$$$(call HdlLibraryRefDir,$$$$l,$$$$(HdlTarget),,DoCore1))
      $(call WkrTargetDir,$1,$4)/$2$(HdlBin): $$$$(HdlPreCore) \
        $$(filter-out $$(filter-out %.vhd,$$(call CoreBlackBoxFiles,$1,$4)) $$(TargetSourceFiles_$4),$$(CompiledSourceFiles)) 
	$(AT)echo Building $(and $(filter-out core,$(HdlMode))) core \"$(2)\" for target \"$$(HdlTarget)\" $$(ParamConfig):$$(ParamMsg) $$@
ifneq (,$(JENKINS_HOME))
	$(AT)echo "============= ($$(shell date +"%H:%M:%S"), started $$(JOB_STARTTIME))"
endif
	$(AT)$$(HdlCompile)
	$(AT)$$(HdlPost)
    endif # end of else of prebuilt

    $(call OcpiDbg,Binary: $(call WkrTargetDir,$1,$4)/$2$(HdlBin))

  endef # DoCore

  #define DoCores
  #  $(foreach c,$(ParamConfigurations),$(call DoCore,$1,$2,$3,$c)$(call DoImplConfig,$1,$c))
  #endef
  $(call OcpiDbgVar,CompiledSourceFiles,b2 )
  $(call OcpiDbgVar,HdlActualTargets)
  $(call OcpiDbgVar,HdlCores)
  #$(foreach t,$(HdlActualTargets),$(eval $(call DoCore,$(t),$(Core))))
  ifneq ($(MAKECMDGOALS),clean)
   ifneq ($(MAKECMDGOALS),skeleton)
    $(foreach c,$(ParamConfigurations),\
      $(foreach t,$(HdlActualTargets),\
        $(foreach both,$(join $(HdlCores),$(Tops:%=:%)),\
          $(foreach core,$(word 1,$(subst :, ,$(both))),\
            $(foreach top,$(word 2,$(subst :, ,$(both))),\
              $(eval $(call DoCore,$t,$(core),$(top),$c,$(and $(filter %_rv,$(top)),uses_rv))))))))
   endif
  endif # end of not cleaning
  $(call OcpiDbgVar,CompiledSourceFiles,b3 )

  ifdef Imports
    $(CoreResults): $(ImportsDir)
  endif

  build: $(LibResults) $(CoreResults)
  $(call OcpiDbgVar,PreBuiltCore)
  $(call OcpiDbgVar,CoreResults)
  $(call OcpiDbgVar,LibResults)

else # end of tool real core
################################################################################
# Just build a library if the tools can't build a core
  include $(OCPI_CDK_DIR)/include/hdl/hdl-lib2.mk
endif

################################################################################
# Finally we create the black box/stub library for this core alone if needed
# On two conditions: first that we actually need to do it (e.g. platforms,cores,assemblies),
# second that the tools need it
ifneq ($(findstring $(HdlMode),assembly core platform config worker),)
ifdef HdlToolNeedBB

################################################################################
# Make the black box library
# Black box libraries for cores are built when it is necessary to have a library containing
# the black box module in order for higher level designs to find the core during synthesis.
# To enable instantiation from verilog this means a foo_bb.v file with an empty module.
# To enable instantiation from VHDL this just means the foo_pkg.vhd file with the module definition
# The bb library is built using the same library name, but in the bb subdirectory
# BUT: it is installed under the foo_bb name
# FIXME: we don't yet build with the VHDL pkg file for instantiating cores from VHDL
$(call OcpiDbgVar,HdlToolNeedBB)

# A function taking a target-dir-name, a libname, and a build target
BBLibFile=$(infox BBF:$1,$2,$3,$4)$(strip\
            $(foreach f,$(call WkrTargetDir,$1,$3)/bb/$(call HdlToolLibraryFile,$4,$(basename $2)),$(infox BBFile:$f)$f))

# $(call DoBBLibraryTarget,target-dir-name,libname,config,target,bbfiles)
define DoBBLibraryTarget
BBLibResults+=$(call BBLibFile,$1,$2,$3,$4)
$(call BBLibFile,$1,$2,$3,$4): | $(call WkrTargetDir,$1,$3)
$(call BBLibFile,$1,$2,$3,$4): LibName=$2
$(call BBLibFile,$1,$2,$3,$4): WorkLib=$2
$(call BBLibFile,$1,$2,$3,$4): ParamConfig=$3
$(call BBLibFile,$1,$2,$3,$4): Core=onewire
$(call BBLibFile,$1,$2,$3,$4): Top=onewire
$(call BBLibFile,$1,$2,$3,$4): override HdlTarget=$4
$(call BBLibFile,$1,$2,$3,$4): TargetDir=$(call WkrTargetDir,$1,$3)/bb
$(call BBLibFile,$1,$2,$3,$4): HdlSources=$5 $(OCPI_CDK_DIR)/include/hdl/onewire.v
$(call BBLibFile,$1,$2,$3,$4): $5
$(call BBLibFile,$1,$2,$3,$4): \
  $$$$(foreach l,$$$$(HdlLibrariesInternal),$$$$(call HdlLibraryRefDir,$$$$l,$4,,DoBBLib))

$(call BBLibFile,$1,$2,$3,$4): $$$$(HdlSources) | $$$$(TargetDir)
	$(AT)$(ECHO) -n Building stub/blackbox library \($$@\) for target' '
	$(AT)$(ECHO) $$(HdlTarget) from: $5
	$(AT)$$(HdlCompile)
	$(AT)$$(HdlPost)

$(call WkrTargetDir,$1,$3)/bb:
	$(AT)mkdir -p $$@

endef

ifneq ($(MAKECMDGOALS),skeleton)
$(foreach c,$(ParamConfigurations),\
  $(foreach f,$(HdlActualTargets),\
    $(eval \
      $(call DoBBLibraryTarget,$f,$(strip\
        $(call HdlRmRv,$(word 1,$(HdlCores)))$(if $(filter 0,$c),,_c$c)),$c,$f,\
        $(call CoreBlackBoxFiles,$f,$c)))))
endif

build: $(BBLibResults)

$(call OcpiDbgVar,BBLibResults)
$(call OcpiDbgVar,HdlActualTargets)

HdlCoreBBInstallDir=$(call HdlCoreInstallDir,$(word 1,$(HdlCores)))_bb
# Install the black box library
$(HdlCoreBBInstallDir):
	$(AT)mkdir -p $@

install_bb: | $(HdlCoreBBInstallDir)
	$(AT)for f in $(HdlFamilies); do \
	   $(call ReplaceContentsIfDifferent,$(strip \
               $(OutDir)target-$$f/bb/$(LibName)),$(strip \
	       $(HdlCoreBBInstallDir)/$$f)); \
	   done
install: install_bb
endif # of making and installing the black box library
endif
