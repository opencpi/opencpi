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
IncludeDirs+=../include
ifneq ($(OCPI_DEBUG),0)
SharedLibLinkOptions+=-g
endif
ifeq ($(shell uname),Linux)
BF=.so
SOEXT=.so
AREXT=.a
SharedLibLinkOptions+=-shared
SharedLibCompileOptions=-fPIC
else
ifeq ($(shell uname),Darwin)
BF=.dylib
SOEXT=.dylib
AREXT=.a
SharedLibLinkOptions+=-dynamiclib
SharedLibCompileOptions=
endif
endif
DispatchSourceFile = $(GeneratedDir)/$(CwdName)_dispatch.c
GeneratedSourceFiles += $(DispatchSourceFile)
ArtifactFile=$(BinaryFile)
# Artifacts are target-specific since they contain things about the binary
ArtifactXmlFile=$(call WkrTargetDir,$1,$2)/$(CwdName)_assy-art.xml
ToolSeparateObjects:=yes
OcpiLibDir=$(OCPI_CDK_DIR)/lib/$$(RccTarget)
RccLibraries=rcc application
LinkBinary=$$(G$(OcpiLanguage)_LINK_$$(RccTarget)) $(SharedLibLinkOptions) -o $$@ \
$(AEPLibraries) \
$(foreach l,$(RccLibraries) $(Libraries),-l ocpi_$l -L $(OcpiLibDir))

#  $$(or $$(wildcard $(OcpiLibDir)/lib$(ol)$(SOEXT)),$(OcpiLibDir)/lib$(ol)$(AREXT)))

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
  $(CompilerWarnings) $(CompilerOptions) \
  $(SharedLibCompileOptions) $(ExtraCompilerOptions) $(IncludeDirs:%=-I%) -o $$@ $$(RccParams) $$<
Compile_cc=\
  $$(Gc++_$$(RccTarget)) -MMD -MP -MF $$@.deps -c \
  $(CompilerWarnings) $(CompilerOptions) \
  $(SharedLibCompileOptions) $(ExtraCompilerOptions) $(IncludeDirs:%=-I%) -o $$@ $$(RccParams) $$<

include $(OCPI_CDK_DIR)/include/xxx-worker.mk

RccAssemblyFile=$(call WkrTargetDir,$1,$2)/$(word 1,$(Workers))_assy.xml

define DoRccArtifactFile

$(call RccAssemblyFile,$1,$2): | $(call WkrTargetDir,$1,$2)
	$(AT)(echo "<RccAssembly>"; \
	  for w in $$(Workers); do echo "<Instance worker=\"$$$$w.xml\" paramconfig=\"$2\"/>"; done; \
	  echo "</RccAssembly>") > $$@

# Different since it is in the targetdir
# note the dependency on the object files so that there will be a new UUID
# whenever the object files are changed.
# FIXME: it is theoretically better to generate the XML as part of the final link phase.
$(call ArtifactXmlFile,$1,$2): $(call RccAssemblyFile,$1,$2) $$(ObjectFiles_$1_$2)
	@echo Generating artifact/runtime xml file $$@ for all workers in one binary
	$(AT)$(DYN_PREFIX) $(ToolsDir)/ocpigen -M $(call WkrTargetDir,$1,$2)/$$(@F).deps \
	     -O $(call RccOs,$1) \
             -V $(call RccOsVersion,$1) \
             -P $(call RccArch,$1) \
	     -D $(call WkrTargetDir,$1,$2) $(XmlIncludeDirs:%=-I%) -A $(RccAssemblyFile)

endef

$(foreach t,$(RccTargets),$(foreach c,$(ParamConfigurations),$(eval $(call DoRccArtifactFile,$t,$c))))

#$(OcpiGen) -A $(RccAssemblyFile)

#disable builtin suffix rules
%.o : %.c
%.o : %.cc

$(DispatchSourceFile):
	$(AT)echo Generating dispatch file: $@
	$(AT)(echo "#include <RCC_Worker.h>";\
	  echo "#define STR(foo) _STR(foo)";\
	  echo "#define _STR(foo) #foo";\
	  for w in $(Workers); do \
	      echo "#include \"$${w}_map.h\"";\
	  done; \
	  for w in $(Workers); do \
	      echo "extern RCCDispatch RCC_FILE_WORKER_$$w;";\
	  done; \
	  echo "RCCEntryTable ocpi_EntryTable[] = {";\
	  for w in $(Workers); do \
	      echo "  {";\
	      echo "    .name=STR(RCC_FILE_WORKER_$$w),";\
	      echo "    .dispatch=&RCC_FILE_WORKER_$$w,";\
	      echo "    .type=STR($(OcpiLanguage))";\
	      echo "  },";\
	  done; \
	  echo "  {.name=0}};";\
	 ) > $@



endif
