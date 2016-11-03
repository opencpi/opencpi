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
BF=$(BF_$(call RccOs,$1))
ifneq ($(OCPI_DEBUG),0)
RccDynamicLinkOptions=-g
endif
# Linux values
BF_linux=.so
SOEXT_linux=.so
AREXT_linux=.a
RccDynamicLinkOptions_linux=$(RccDynamicLinkOptions) -shared
RccDynamicCompilerOptions_linux=-fPIC
# macos values
BF_macos=.dylib
SOEXT_macos=.dylib
AREXT_macos=.a
RccDynamicLinkOptions_macos=$(RccDynamicLinkOptions) -dynamiclib -Xlinker -undefined -Xlinker dynamic_lookup
DispatchSourceFile=$(call WkrTargetDir,$1,$2)/$(CwdName)_dispatch.c
ArtifactFile=$(BinaryFile)
# Artifacts are target-specific since they contain things about the binary
ArtifactXmlFile=$(call WkrTargetDir,$1,$2)/$(word 1,$(Workers))_assy-art.xml
ToolSeparateObjects:=yes
OcpiLibDir=$(OCPI_CDK_DIR)/lib/$$(RccTarget)$(and $(OCPI_TARGET_MODE),/$(OCPI_TARGET_MODE))
# Add the libraries we know a worker might reference.
override RccLibrariesInternal+=rcc application os

ifeq ($(OCPI_USE_TARGET_MODES),1)
  export OCPI_TARGET_MODE:=$(if $(filter 1,$(OCPI_DYNAMIC)),d,s)$(if $(filter 1,$(OCPI_DEBUG)),d,o)
endif
PatchElf=$(or $(OCPI_PREREQUISITES_INSTALL_DIR),/opt/opencpi/prerequisites)/patchelf/$(OCPI_TOOL_HOST)/bin/patchelf
LinkBinary=$(G$(OcpiLanguage)_LINK_$(RccTarget)) $(call RccPrioritize,DynamicLinkOptions,$(OcpiLanguage),$(RccTarget)) -o $@ $1 \
$(AEPLibraries) \
$(foreach l,$(RccLibrariesInternal) $(Libraries),\
  $(if $(findstring /,$l),\
    $(foreach p,$(dir $l)$(RccTarget)/lib$(notdir $l),\
       $(or $(wildcard $p$(AREXT_$(call RccOs,))),\
            $(and $(wildcard $p$(SOEXT_$(call RccOs,))),-L $(dir $l)$(RccTarget) -l $(notdir $l)),\
            $(error No RCC library found for $l, tried $p$(AREXT_$(call RccOs,)) and $p$(SOEXT_$(call RccOs,))))), \
    $(and $(filter 1,$(OCPI_DYNAMIC)),-l ocpi_$l))) \
  -L $(OCPI_CDK_DIR)/lib/$(RccTarget)$(and $(OCPI_TARGET_MODE),/d$(if $(filter 1,$(OCPI_DEBUG)),d,o))

# $1 is target, $2 is configuration
RccStaticName=$(WkrBinaryName)_s$(BF)
RccStaticPath=$(call WkrTargetDir,$1,$2)/$(RccStaticName)
define RccWkrBinary
  $$(infox RccWkrBinary:$1:$2:$$(call RccOs,))
  ifeq ($$(call RccOs,$1),linux)
    $$(call RccStaticPath,$1,$2): $$(call WkrBinary,$1,$2)
	$(AT)$(OCPI_CDK_DIR)/scripts/makeStaticWorker $$< \
	  $$(foreach l,$$(RccLibrariesInternal),libocpi_$$l$$(call BF,$1))

    all: $$(call RccStaticPath,$1,$2)
  endif
endef

define RccWkrBinaryLink
  $$(info RccWkrBinaryLink:$1:$2:$3:$4:$5 name:$$(call RccStaticName,$1,$4):$(LibDir)/$1/$5_s$$(call BF,$1))
  ifeq ($$(call RccOs,$1),linux)
    $(LibDir)/$1/$5_s$$(call BF,$1): $$(call RccStaticPath,$1,$4) | $(LibDir)/$1
	$(AT)echo Exporting worker binary for static executables: $$@ '->' $$<
	$(AT)$$(call MakeSymLink2,$$<,$$(dir $$@),$$(notdir $$@))
    LibLinks+=$(LibDir)/$1/$5_s$$(call BF,$1)
  endif
endef


CompilerWarnings= -Wall -Wextra
CompilerDebugFlags=-g
CompilerOptimizeFlags=-O
ifeq ($(OCPI_DEBUG),1)
RccCompilerOptions=$(CompilerDebugFlags)
else
RccCompilerOptions=$(CompilerOptimizeFlags)
endif
# Prepare the parameters for compile-command-line injection into the worker compilation
RccParams=\
  $(foreach n,$(WorkerParamNames),\
	     '-DPARAM_$n()=$(Param_$(ParamConfig)_$n)')

# Given flag name, target and langauge and flag name, prioritize the flags, as defined:
# target and language
# target
# generic
# E.g. $(call RccPrioritize,CompilerOptions,C,target)
OcpiDefined=$(filter-out undefined,$(origin $1))
RccPrioritize=\
  $(if $(call OcpiDefined,Rcc$1$2_$3),\
    $(Rcc$1$2_$3),\
    $(if $(call OcpiDefined,Rcc$1_$3),\
      $(Rcc$1_$3),\
      $(if $(call OcpiDefined,Rcc$1$2_$(call RccOs,$3)),\
        $(Rcc$1$2_$(call RccOs,$3)),\
        $(if $(call OcpiDefined,Rcc$1_$(call RccOs,$3)),\
          $(Rcc$1_$(call RccOs,$3)),\
          $(if $(call OcpiDefined,Rcc$1$2),\
            $(Rcc$1$2),\
            $(Rcc$1))))))

# RccCompilerOptions(language,target)
RccFinalCompilerOptions=\
  $(call RccPrioritize,CompilerWarnings,$1,$2) \
  $(call RccPrioritize,CompilerOptions,$1,$2) \
  $(call RccPrioritize,DynamicCompilerOptions,$1,$2) \
  $(call RccPrioritize,ExtraCompilerOptions,$1,$2) \
  $(call RccPrioritize,DynamicCompilerExtraCompilerOptions,$1,$2) \

Compile_c=\
  $$(Gc_$$(RccTarget)) -MMD -MP -MF $$@.deps -c \
  $$(call RccFinalCompilerOptions,C,$$(RccTarget)) \
  $(RccIncludeDirsInternal:%=-I%) -o $$@ $$(RccParams) $$<
Compile_cc=\
  $$(Gc++_$$(RccTarget)) -MMD -MP -MF $$@.deps -c \
  $$(call RccFinalCompilerOptions,CC,$$(RccTarget)) \
  $$(ExtraCompilerOptionsCC_$$(RccTarget)) $(ignore for legacy)\
  $(RccIncludeDirsInternal:%=-I%) -o $$@ $$(RccParams) $$<
Compile_cpp=$(Compile_cc)
Compile_cxx=$(Compile_cc)

include $(OCPI_CDK_DIR)/include/xxx-worker.mk

RccAssemblyFile=$(call WkrTargetDir,$1,$2)/$(word 1,$(Workers))_assy.xml

define DoRccArtifactFile

TargetSourceFiles += $(call DispatchSourceFile,$1,$2)
$(call WkrMakeObject,$(call DispatchSourceFile,$1,$2),$1,$2)

$(call DispatchSourceFile,$1,$2): $$(ImplHeaderFiles) | $$(call WkrTargetDir,$1,$2)
	$(AT)echo Generating dispatch file: $$@
	$(AT)(echo "/* Auto generated OpenCPI framework file - DO NOT EDIT - created by rcc-worker.mk $(OcpiDateStamp) */";\
	  echo "#include <RCC_Worker.h>";\
	  echo "#define STR(foo) _STR(foo)";\
	  echo "#define _STR(foo) #foo";\
	  for w in $(Workers); do \
	      echo "#include \"$$$${w}_map.h\"";\
	  done; \
	  for w in $(Workers); do \
	      echo "extern RCCDispatch RCC_FILE_WORKER_ENTRY_$$$$w;";\
	  done; \
	  echo "RCCEntryTable ocpi_EntryTable[] = {";\
	  for w in $(Workers); do \
	      echo "  {";\
	      echo "    .name=STR(RCC_FILE_WORKER_$$$${w}$(and $(filter-out 0,$2),-$2)),";\
	      echo "    .dispatch=&RCC_FILE_WORKER_ENTRY_$$$$w,";\
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
             -H $(call RccArch,$1) \
	     -P $3 \
	     -D $(call WkrTargetDir,$1,$2) $(XmlIncludeDirsInternal:%=-I%) -A $(RccAssemblyFile)

endef

$(foreach p,\
  $(RccPlatforms),$(foreach c,$(ParamConfigurations),\
     $(eval $(call DoRccArtifactFile,$(RccTarget_$p),$c,$p))))

#$(OcpiGen) -A $(RccAssemblyFile)

#disable builtin suffix rules
%.o : %.c
%.o : %.cc
%.o : %.cpp
%.o : %.cxx



endif
