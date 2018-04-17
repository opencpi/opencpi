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
RccPrereqLibs=$(RccStaticPrereqLibs) $(RccDynamicPrereqLibs)
# This allows component library level RCC libraries to be fed down to workers,
# via RccLibrariesInternal, while allowing the worker itself to have more libraries
# via setting Libraries or RccLibraries.
# Library level RCC libraries also are specified via RccLibraries.
override RccLibrariesInternal := $(RccLibraries) $(Libraries) $(RccLibrariesInternal)
override RccIncludeDirsInternal := \
  $(foreach l,$(RccLibrariesInternal),$(dir $l)include) \
  $(RccIncludeDirs) $(IncludeDirs) $(RccIncludeDirsInternal)
$(call OcpiDbgVar,RccLibrariesInternal)
$(call OcpiDbgVar,RccIncludeDirsInternal)
# This is evaluated late, when RccTarget and RccPlatform are defined.
RccIncludeDirsActual=$(RccIncludeDirsInternal)\
 $(if $(OcpiBuildingACI),. include,../include gen) \
 $(OCPI_CDK_DIR)/include/$(if $(OcpiBuildingACI),aci,rcc) \
 $(call OcpiGetRccPlatformDir,$(RccRealPlatform))/include \
 $(foreach l,$(RccPrereqLibs),\
   $(OCPI_PREREQUISITES_DIR)/$l/$(RccRealPlatform)/include\
   $(OCPI_PREREQUISITES_DIR)/$l/include)

BF=$(BF_$(call RccOs,$1))
# This is for backward compatibility
RccLinkOptions=$(SharedLibLinkOptions)
RccCompileWarnings=-Wall
ifneq ($(OCPI_DEBUG),0)
RccLinkOptions+= -g
endif
# Linux values
BF_linux=.so
SOEXT_linux=.so
AREXT_linux=.a
RccLinkOptions_linux=$(RccLinkOptions) -shared
RccCompileOptions_linux=$(RccCompileOptions) -fPIC
# macos values
BF_macos=.dylib
SOEXT_macos=.dylib
AREXT_macos=.a
RccMainLinkOptions_macos=$(RccLinkOptions) -Xlinker -rpath -Xlinker $(OCPI_CDK_DIR)/$(RccPlatform)/lib
RccLinkOptions_macos=$(RccLinkOptions) -dynamiclib -Xlinker -undefined -Xlinker dynamic_lookup
DispatchSourceFile=$(call WkrTargetDir,$1,$2)/$(CwdName)_dispatch.c
ArtifactFile=$(BinaryFile)
# Artifacts are target-specific since they contain things about the binary
ArtifactXmlFile=$(call WkrTargetDir,$1,$2)/$(word 1,$(Workers))_assy-art.xml
ToolSeparateObjects:=yes 
OcpiLibDir=$(OCPI_CDK_DIR)/$(RccPlatform)/lib
OcpiIsDynamic=$(if $(findstring d,$(word 2,$(subst -, ,$1))),1,0)
# Add the libraries we know a worker might reference.
ifdef OcpiBuildingACI
  RccSpecificLinkOptions=\
    $(call RccPrioritize,MainLinkOptions,$(OcpiLanguage),$(RccTarget),$(RccPlatform))
  override RccLibrariesInternal+=\
    application container library transport xfer util \
    msg_driver_interface foreign os   
else
  RccSpecificLinkOptions=\
    $(call RccPrioritize,DynamicLinkOptions,$(OcpiLanguage),$(RccTarget),$(RccPlatform)) \
    $(call RccPrioritize,LinkOptions,$(OcpiLanguage),$(RccTarget),$(RccPlatform))
  override RccLibrariesInternal+=rcc application os
endif

Comma=,
RccLibDir=$(OCPI_CDK_DIR)/$(RccPlatform)/lib
LinkBinary=$(G$(OcpiLanguage)_LINK_$(RccRealPlatform)) \
$(G$(OcpiLanguage)_MAIN_FLAGS_$(RccRealPlatform)) \
$(RccSpecificLinkOptions) \
$(call RccPrioritize,ExtraLinkOptions,$(OcpiLanguage),$(RccTarget),$(RccPlatform)) \
-o $@ $1 \
$(AEPLibraries) \
$(call RccPrioritize,CustomLibs,$(OcpiLanguage),$(RccTarget),$(RccPlatform)) \
$(call RccPrioritize,LocalLibs,$(OcpiLanguage),$(RccTarget),$(RccPlatform)) \
$(foreach l,$(RccLibrariesInternal) $(Libraries),\
  $(if $(findstring /,$l),\
    $(foreach p,$(dir $l)$(RccPlatform)/lib$(notdir $l),\
       $(or $(wildcard $p$(AREXT_$(call RccOs,))),\
            $(and $(wildcard $p$(SOEXT_$(call RccOs,))),-L $(dir $l)$(RccPlatform) -l $(notdir $l)),\
            $(error No RCC library found for $l, tried $p$(AREXT_$(call RccOs,)) and $p$(SOEXT_$(call RccOs,))))), \
    $(if $(filter 1,$(call OcpiIsDynamic,$(RccPlatform))),\
       -l ocpi_$l,\
       $(and $(OcpiBuildingACI),$(RccLibDir)/libocpi_$l$(AREXT_$(call RccOs,)))))) \
  -L $(RccLibDir) \
  $(foreach l,$(RccStaticPrereqLibs),\
    $(OCPI_PREREQUISITES_DIR)/$l/$(RccRealPlatform)/lib/lib$l.a) \
  $(and $(RccDynamicPrereqLibs),-Wl$(Comma)-rpath -Wl$(Comma)'$$ORIGIN') \
  $(foreach l,$(RccDynamicPrereqLibs),\
    $(OCPI_PREREQUISITES_DIR)/$l/$(RccRealPlatform)/lib/lib$l$(SOEXT_$(call RccOs,))) \
  $(and $(OcpiBuildingACI),$(G$(OcpiLanguage)_MAIN_LIBS_$(RccRealPlatform):%=-l%)) \
  $(if $(filter 1,$(call OcpiIsDynamic,$(RccPlatform))),,\
     && $(OCPI_CDK_DIR)/scripts/makeStaticWorker.sh $(call RccOs,) $@ \
	  $(foreach l,$(RccLibrariesInternal),libocpi_$l$(call BF,$(call RccOs,)))) \
  $(foreach l,$(RccDynamicPrereqLibs),\
    && cp $(OCPI_PREREQUISITES_DIR)/$l/$(RccRealPlatform)/lib/lib$l$(SOEXT_$(call RccOs,)) $(@D))

# $1 is target, $2 is configuration
RccStaticName=$(WkrBinaryName)_s$(SOEXT_$(call RccOs,$1))
RccStaticPath=$(call WkrTargetDir,$1,$2)/$(call RccStaticName,$1)

CompilerWarnings= -Wall -Wextra
CompilerDebugFlags=-g
CompilerOptimizeFlags=-O -DNDEBUG=1
ifeq ($(OCPI_DEBUG),1)
RccCompileOptions=$(CompilerDebugFlags)
else
RccCompileOptions=$(CompilerOptimizeFlags)
endif
$(foreach v,$(filter ExtraCompilerOptionsCC_%,$(.VARIABLES)),\
  $(foreach t,$(v:ExtraCompilerOptionsCC_%=%),\
    $(foreach p,$(RccPlatforms),\
      $(and $(filter $(RccTarget_$p),$t),\
	 $(eval RccExtraCompileOptionsCC_$p=$($v))))))
# Prepare the parameters for compile-command-line injection into the worker compilation
RccParams=\
  $(foreach n,$(WorkerParamNames),\
	     '-DPARAM_$n()=$(Param_$(ParamConfig)_$n)')

# Given flag name, target and language and flag name, prioritize the flags, as defined:
# target and language
# target
# generic
# E.g. $(call RccPrioritize,CompileOptions,C,target,platform)
OcpiDefined=$(filter-out undefined,$(origin $1))
RccPrioritize=\
  $(if $(call OcpiDefined,Rcc$1$2_$3),\
    $(Rcc$1$2_$3),\
    $(if $(call OcpiDefined,Rcc$1_$3),\
      $(Rcc$1_$3),\
      $(if $(call OcpiDefined,Rcc$1$2_$4),\
        $(Rcc$1$2_$4),\
        $(if $(call OcpiDefined,Rcc$1_$4),\
          $(Rcc$1_$4),\
          $(if $(call OcpiDefined,Rcc$1$2_$(call RccOs,$3)),\
            $(Rcc$1$2_$(call RccOs,$3)),\
            $(if $(call OcpiDefined,Rcc$1_$(call RccOs,$3)),\
              $(Rcc$1_$(call RccOs,$3)),\
              $(if $(call OcpiDefined,Rcc$1$2),\
                 $(Rcc$1$2),\
                 $(Rcc$1))))))))

# RccCompileOptions(language,target)
RccFinalCompileOptions=\
  $(call RccPrioritize,CompileWarnings,$1,$2,$3) \
  $(call RccPrioritize,ExtraCompileWarnings,$1,$2,$3) \
  $(call RccPrioritize,CompileOptions,$1,$2,$3) \
  $(call RccPrioritize,ExtraCompileOptions,$1,$2,$3) \

Compile_c=$$(call OcpiFixPathArgs,\
  $$(Gc_$$(RccRealPlatform)) -MMD -MP -MF $$@.deps -c \
  $$(call RccFinalCompileOptions,C,$$(RccTarget),$$(RccPlatform)) \
  $$(RccIncludeDirsActual:%=-I%) -o $$@ $$(RccParams) $$<)
Compile_cc=$$(call OcpiFixPathArgs,\
  $$(Gc++_$$(RccRealPlatform)) -MMD -MP -MF $$@.deps -c \
  $$(call RccFinalCompileOptions,CC,$$(RccTarget),$$(RccPlatform)) \
  $$(RccIncludeDirsActual:%=-I%) -o $$@ $$(RccParams) $$<)
Compile_cpp=$(Compile_cc)
Compile_cxx=$(Compile_cc)

ifndef OcpiBuildingACI
include $(OCPI_CDK_DIR)/include/xxx-worker.mk
endif

RccAssemblyFile=$(call WkrTargetDir,$1,$2)/$(word 1,$(Workers))_assy.xml

define DoRccArtifactFile

TargetSourceFiles_$2 += $(call DispatchSourceFile,$1,$2)
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
	$(AT)$(DYN_PREFIX) $(ToolsDir)/ocpigen $(call OcpiFixPathArgs,-M $(call WkrTargetDir,$1,$2)/$$(@F).deps \
	     -O $(call RccOs,$1) \
             -V $(call RccOsVersion,$1) \
             -H $(call RccArch,$1) \
	     -P $(call RccRealPlatforms,$3) \
	     -Z $(call OcpiIsDynamic,$3) \
	     -D $(call WkrTargetDir,$1,$2) $(XmlIncludeDirsInternal:%=-I%) -A $(RccAssemblyFile))

endef

ifndef OcpiBuildingACI
$(foreach p,$(RccPlatforms),\
  $(foreach c,$(ParamConfigurations),\
    $(eval $(call DoRccArtifactFile,$(RccTarget_$p),$c,$p))))
endif

# ShowVar(varname)
RccNeq=$(if $(subst x$1,,x$2)$(subst x$2,,x$1),x,)
RccShowVar=\
  echo '  'Rcc$1 = '$(value Rcc$1)' \
  $(and $(value Rcc$1),$(call RccNeq,$(value Rcc$1),$(Rcc$1)),;echo "    expands to:" $(Rcc$1))

RccLanguage=$(if $(filter $1,c),C,CC)
RccShowGeneric=\
  $(call RccShowVar,$1);\
  $(call RccShowVar,$1$(RccLanguage));\
# ShowVarSuffixes(varname, platform)
RccShowVarSuffixes=\
  $(call RccShowVar,$1_$(call RccOs,$(RccTarget_$2)));\
  $(call RccShowVar,$1$(RccLanguage)_$(call RccOs,$(RccTarget_$2)));\
  $(call RccShowVar,$1_$2);\
  $(call RccShowVar,$1$(RccLanguage)_$2);\
  echo '  Actual Rcc$1 = '$(call RccPrioritize,$1,$(RccTarget_$2),$2);

RccAllVars:=\
  CompileWarnings ExtraCompileWarnings \
  CompileOptions ExtraCompileOptions \
  LinkOptions ExtraLinkOptions \
  CustomLibs

# Show the variables relevant to documented/supported options
showvars:
	$(AT)echo Variables used for all platforms unless overridden '(for '$(OcpiLanguage) language')':; \
	  $(foreach v,$(RccAllVars),$(call RccShowGeneric,$v)) \
	  $(foreach p,$(RccPlatforms),echo Variables applicable to platform:'  '$p;$(foreach v,$(RccAllVars),$(call RccShowVarSuffixes,$v,$p)))

#disable builtin suffix rules
%.o : %.c
%.o : %.cc
%.o : %.cpp
%.o : %.cxx

endif
