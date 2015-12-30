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

# Makefile for an RCC worker
ifndef RCC_WORKER_MK
RCC_WORKER_MK:=xxx
ifneq ($(words $(Workers)),1)
BinaryName:=$(CwdName)
endif
include $(OCPI_CDK_DIR)/include/rcc/rcc-make.mk
Model=rcc
# Default is that you are building in a subdirectory of all implementations
# Target:=$(shell uname -s)=$(shell uname -r)=$(shell uname -p)=gcc=$(shell gcc -dumpversion)=$(shell gcc -dumpmachine)
RccSourceSuffix:=.$(Suffix_rcc_$(OcpiLanguage))
RccImplSuffix=$(if $(filter c++,$(OcpiLanguage)),-worker.hh,_Worker.h)
RccSkelSuffix=-skel$(RccSourceSuffix)
OBJ:=.o
override RccIncludeDirsInternal+=../include gen $(OCPI_CDK_DIR)/include/rcc
BF=$(BF_$(call RccOs,))
ifneq ($(OCPI_DEBUG),0)
SharedLibLinkOptions=-g
endif
SharedLibLinkOptions+=\
  $(or $(SharedLibLinkOptions_$(HdlTarget)),$(SharedLibLinkOptions_$(call RccOs,)))
SharedLibCompileOptions=\
  $(or $(SharedLibCompileOptions_$(HdlTarget)),$(SharedLibCompileOptions_$(call RccOs,)))
# Linux values
BF_linux=.so
SOEXT_linux=.so
AREXT_linux=.a
SharedLibLinkOptions_linux=-shared
SharedLibCompileOptions_linux=-fPIC
# macos values
BF_macos=.dylib
SOEXT_macos=.dylib
AREXT_macos=.a
SharedLibLinkOptions_macos=-dynamiclib
SharedLibCompileOptions_macos=
DispatchSourceFile=$(call WkrTargetDir,$1,$2)/$(CwdName)_dispatch.c
ArtifactFile=$(BinaryFile)
# Artifacts are target-specific since they contain things about the binary
ArtifactXmlFile=$(call WkrTargetDir,$1,$2)/$(word 1,$(Workers))_assy-art.xml
ToolSeparateObjects:=yes
OcpiLibDir=$(OCPI_CDK_DIR)/lib/$$(RccTarget)$(and $(OCPI_TARGET_MODE),/$(OCPI_TARGET_MODE))
# Add the libraries we know a worker might reference.
override RccLibrariesInternal+=rcc application os

ifdef OCPI_USE_TOOL_MODES
  ifndef OCPI_TOOL_MODE
    export OCPI_TARGET_MODE:=$(if $(filter 1,$(OCPI_BUILD_SHARED_LIBRARIES)),d,s)$(if $(filter 1,$(OCPI_DEBUG)),d,o)
  endif
endif
PatchElf=$(or $(OCPI_PREREQUISITES_INSTALL_DIR),/opt/opencpi/prerequisites)/patchelf/$(OCPI_TOOL_HOST)/bin/patchelf
LinkBinary=$$(G$(OcpiLanguage)_LINK_$$(RccTarget)) $(SharedLibLinkOptions) -o $$@ $1 \
$(AEPLibraries) \
$(foreach l,$(RccLibrariesInternal) $(Libraries),\
  $(if $(findstring /,$l),\
    $(foreach p,$(dir $l)$(RccTarget)/lib$(notdir $l),\
       $(or $(wildcard $p$(AREXT_$(call RccOs,))),\
            $(and $(wildcard $p$(SOEXT_$(call RccOs,))),-L $(dir $l)$(RccTarget) -l $(notdir $l)),\
            $(error No RCC library found for $l, tried $p$(AREXT_$(call RccOs,)) and $p$(SOEXT_$(call RccOs,))))), \
    $(and $(filter 1,$(OCPI_BUILD_SHARED_LIBRARIES)),-l ocpi_$l))) \
  -L $(OCPI_CDK_DIR)/lib/$$(RccTarget)$(and $(OCPI_TARGET_MODE),/d$(if $(filter 1,$(OCPI_DEBUG)),d,o))

# $1 is target, $2 is configuration
RccStaticName=$(WkrBinaryName)_s$(BF)
RccStaticPath=$(call WkrTargetDir,$1,$2)/$(RccStaticName)
define RccWkrBinary
  $$(infox RccWkrBinary:$1:$2:$$(call RccOs,))
  ifeq ($$(call RccOs,),linux)
    $(call RccStaticPath,$1,$2): $(call WkrBinary,$1,$2)
	$(AT)$(OCPI_CDK_DIR)/scripts/makeStaticWorker $$< \
	  $$(foreach l,$$(RccLibrariesInternal),libocpi_$$l$$(BF))

    all: $(call RccStaticPath,$1,$2)
  endif
endef

define RccWkrBinaryLink
  $$(infox RccWkrBinaryLink:$1:$2:$3:$4:$5 name:$$(call RccStaticName,$1,$4):$(LibDir)/$1/$5_s$(BF))
  ifeq ($$(call RccOs,),linux)
    $(LibDir)/$1/$5_s$(BF): $(call RccStaticPath,$1,$4) | $(LibDir)/$1
	$(AT)echo Exporting worker binary for static executables: $$@ '->' $$<
	$(AT)$$(call MakeSymLink2,$$<,$$(dir $$@),$$(notdir $$@))
    LibLinks+=$(LibDir)/$1/$5_s$(BF)
  endif
endef


CompilerWarnings= -Wall -Wextra
CompilerDebugFlags=-g
CompilerOptimizeFlags=-O
ifeq ($(OCPI_DEBUG),1)
CompilerOptions=$(CompilerDebugFlags)
else
CompilerOptions=$(CompilerOptimizeFlags)
endif
# Prepare the parameters for compile-command-line injection into the worker compilation
RccParams=\
  $(foreach n,$(WorkerParamNames),\
	     '-DPARAM_$n()=$(Param_$(ParamConfig)_$n)')
Compile_c=\
  $$(Gc_$$(RccTarget)) -MMD -MP -MF $$@.deps -c \
  $$(CompilerWarnings_$$(RccTarget)) $$(CompilerOptions_$$(RccTarget)) \
  $(call SharedLibCompileOptions) $$(ExtraCompilerOptionsC_$$(RccTarget)) \
  $(RccIncludeDirsInternal:%=-I%) -o $$@ $$(RccParams) $$<
Compile_cc=\
  $$(Gc++_$$(RccTarget)) -MMD -MP -MF $$@.deps -c \
  $$(CompilerWarnings_$$(RccTarget)) $$(CompilerOptions_$$(RccTarget)) \
  $(call SharedLibCompileOptions) \
  $$(ExtraCompilerOptions_$$(RccTarget)) $$(ExtraCompilerOptionsCC_$$(RccTarget)) \
  $(RccIncludeDirsInternal:%=-I%) -o $$@ $$(RccParams) $$<

include $(OCPI_CDK_DIR)/include/xxx-worker.mk

RccAssemblyFile=$(call WkrTargetDir,$1,$2)/$(word 1,$(Workers))_assy.xml

define DoRccArtifactFile

TargetSourceFiles += $(call DispatchSourceFile,$1,$2)
$(call WkrMakeObject,$(call DispatchSourceFile,$1,$2),$1,$2)

$(call DispatchSourceFile,$1,$2): $$(ImplHeaderFiles) | $$(call WkrTargetDir,$1,$2)
	$(AT)echo Generating dispatch file: $$@
	$(AT)(echo "#include <RCC_Worker.h>";\
	  echo "#define STR(foo) _STR(foo)";\
	  echo "#define _STR(foo) #foo";\
	  for w in $(Workers); do \
	      echo "#include \"$$$${w}_map.h\"";\
	  done; \
	  for w in $(Workers); do \
	      echo "extern RCCDispatch RCC_FILE_WORKER_$$$$w;";\
	  done; \
	  echo "RCCEntryTable ocpi_EntryTable[] = {";\
	  for w in $(Workers); do \
	      echo "  {";\
	      echo "    .name=STR(RCC_FILE_WORKER_$$$${w}$(and $(filter-out 0,$2),-$2)),";\
	      echo "    .dispatch=&RCC_FILE_WORKER_$$$$w,";\
	      echo "    .type=STR($(OcpiLanguage))";\
	      echo "  },";\
	  done; \
	  echo "  {.name=0}};";\
	 ) > $$@

$(call RccAssemblyFile,$1,$2): | $(call WkrTargetDir,$1,$2)
	$(AT)(echo "<RccAssembly>"; \
	  for w in $$(Workers); do echo "<Instance worker=\"$$$$w.xml\" paramconfig=\"$2\"/>"; done; \
	  echo "</RccAssembly>") > $$@

# Different since it is in the targetdir
# note the dependency on the object files so that there will be a new UUID
# whenever the object files are changed.
# FIXME: it is theoretically better to generate the XML as part of the final link phase.
$(call ArtifactXmlFile,$1,$2): $(call RccAssemblyFile,$1,$2) $$(ObjectFiles_$1_$2)
	$(AT)echo Generating artifact/runtime xml file $$@ for all workers in one binary
	$(AT)$(DYN_PREFIX) $(ToolsDir)/ocpigen -M $(call WkrTargetDir,$1,$2)/$$(@F).deps \
	     -O $(call RccOs,$1) \
             -V $(call RccOsVersion,$1) \
             -P $(call RccArch,$1) \
	     -D $(call WkrTargetDir,$1,$2) \
             $(XmlIncludeDirsInternal:%=-I%) \
             -A $(call RccAssemblyFile,$1,$2)

endef

$(foreach t,$(RccTargets),$(foreach c,$(ParamConfigurations),$(eval $(call DoRccArtifactFile,$t,$c))))

#$(OcpiGen) -A $(RccAssemblyFile)

#disable builtin suffix rules
%.o : %.c
%.o : %.cc



endif
