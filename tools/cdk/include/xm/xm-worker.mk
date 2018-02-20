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

# #####
#
# This Makefile fragment converts X-Midas primitives (both C++ and
# FORTRAN primitives) into OpenCPI components.
#
# The tool ocpixm produces an OpenCPI RCC wrapper from the OpenCPI X-Midas
# XML metadata description of an X-Midas primitive. The generated RCC wrapper
# is combined with the object file for the X-Midas primitive and it is
# linked against the OpenCPI X-Midas "intercept" library which breaks
# the X-Midas primitive's dependency on the X-Midas runtime environment.
# The output of this  Makefile is a shared library that contains the OpenCPI
#  X-Midas worker which can be used outside of the X-Midas environment.
#
# See the OpenCPI X-Midas Authoring Reference and the OpenCPI X-Midas
# User's guide for more details.
#
########################################################################### #

# Makefile for an XM/RCC worker
Model=rcc
# Default is that you are building in a subdirectory of all implementations
# Target:=$(shell uname -s)=$(shell uname -r)=$(shell uname -p)=gcc=$(shell gcc -dumpversion)=$(shell gcc -dumpmachine)
ImplSuffix=_Worker.h
SkelSuffix=_skel.c
SourceSuffix=.c
OBJ:=.o

IncludeDirs += $(OCPI_CDK_DIR)/include/xm

ifeq ($(shell uname),Linux)
BF=.so
SOEXT=.so
AREXT=.a
SharedLibLinkOptions=-shared
SharedLibCompileOptions=-fPIC
else
ifeq ($(shell uname),Darwin)
BF=.dylib
SOEXT=.dylib
AREXT=.a
SharedLibLinkOptions=-dynamiclib
SharedLibCompileOptions=
endif
endif
DispatchSourceFile = $(GeneratedDir)/$(word 1,$(Workers))_dispatch.c
GeneratedSourceFiles += $(DispatchSourceFile)
ArtifactFile=$(BinaryFile)
ArtifactXmlFile = $(GeneratedDir)/$(word 1,$(Workers))_art.xml
GCC=gcc
GXX=g++
GCCLINK=$(GXX)
$(info Target is $(Target))
ifeq ($(Target),Linux-MCS_864x)
GCC=/opt/timesys/toolchains/ppc86xx-linux/bin/ppc86xx-linux-gcc -include sal.h
GXX=/opt/timesys/toolchains/ppc86xx-linux/bin/ppc86xx-linux-g++ -include sal.h
GCCLINK=$(GXX)
OtherLibraries += /opt/mercury/linux-MPC8641D/lib/libsal.a
IncludeDirs += /opt/mercury/include
else ifeq ($(Target),Linux-x86_32)
GCC=gcc -m32
GXX=g++ -m32
GCCLINK=$(GXX) -m elf_i386
else
Target=$(HostTarget)
endif

CFLAGS += -Wall -Wextra -Wshadow  -g -D"OCPI_XM_MAINROUTINE=$(MainRoutine)"
CXXFLAGS += -Wall -Wextra -Wshadow -g -D"OCPI_XM_MAINROUTINE=$(MainRoutine)"
OcpiLibDir=$(OCPI_CDK_DIR)/../lib/$(Target)-bin
LinkBinary=$(GCCLINK) $(SharedLibLinkOptions) -o $@ $(ObjectFiles) \
$(OtherLibraries) $(AEPLibraries) \
$(foreach ol,$(OcpiLibraries),$(or $(wildcard $(OcpiLibDir)/lib$(ol)$(SOEXT)),$(OcpiLibDir)/lib$(ol)$(AREXT)))
MainRoutine=ocpi_xm_mainroutine_$(word 1,$(Workers))
Compile_c=$(GCC) -MMD -MP -MF $(GeneratedDir)/$$(@F).deps -c $(CFLAGS) $(SharedLibCompileOptions) $(IncludeDirs:%=-I%) -o $$@ $$<
Compile_cc=$(GXX) -MMD -MP -MF $(GeneratedDir)/$$(@F).deps -c $(CXXFLAGS) $(SharedLibCompileOptions) $(IncludeDirs:%=-I%) -o $$@ $$<

FC = /usr/bin/pfc

Compile_for = \
	$(FC) $$< R$(OCPI_CDK_DIR)/include/xm/xmpfc.cnf  Z$(OCPI_CDK_DIR)/../../xmidas_ppc/xm/inc  > /dev/null 2>&1; \
	mv $$(basename $$(@F)).c $$(basename $$(@F)).cxxx  > /dev/null 2>&1; \
	(sh -c 'echo -e "\n\#include \"ocpi_xm_intercept_fortran.h\"\n\#include \"ocpi_xm_fortran_buffers.h\"\n\n\#ifndef INCLUDED_FORTRAN_H\n\#define INCLUDED_FORTRAN_H\ntypedef int bool;\n\#define LPROTOTYPE\n\#include \"fortran.h\"\n\#endif\n\n"'; cat $$(basename $$(@F)).cxxx | sed 's/U[0-9]*\.//g' | grep -v "\#include" | grep -v LPROTOTYPE) > $$(basename $$(@F)).c; \
	$(GCC) -MMD -MP -MF $(GeneratedDir)/$$(basename $$(@F)).c.deps -c $(CFLAGS) $(SharedLibCompileOptions) $(IncludeDirs:%=-I%) -o $$@ $$(basename $$(@F)).c > /dev/null 2>&1; \
	rm -f $$(basename $$(@F)).c $$(basename $$(@F)).cxxx $$(basename $$(@F)).hf;

ifndef Application

Xm2OcpiGen=$(ToolsDir)/ocpixm

ModelSpecificBuildHook :=  $(word 1,$(Workers))$(SourceSuffix)

endif

include $(OCPI_CDK_DIR)/include/xxx-worker.mk

$(ModelSpecificBuildHook):
	$(AT)echo Generating the XM wrapper file: $@; \
	$(Xm2OcpiGen) $(word 1,$(Workers))_xm.xml;

RccAssemblyFile=$(GeneratedDir)/$(word 1,$(Workers))_assy.xml
$(RccAssemblyFile): | $(GeneratedDir)
	$(AT)(echo "<RccAssembly Name=\""$(word 1,$(Workers))"\">"; \
	  for w in $(Workers); do echo "<Worker File=\"$$w.xml\"/>"; done; \
	  echo "</RccAssembly>") > $@

$(ArtifactXmlFile): $(RccAssemblyFile)
	@echo Generating artifact/runtime xml file \($(ArtifactXmlFile)\) for all workers in one binary
	$(AT)$(call OcpiGen, -A $(RccAssemblyFile))

#disable builtin suffix rules
%.o : %.c

$(DispatchSourceFile):
	$(AT)echo Generating dispatch file: $@
	$(AT)(echo "/* Auto generated OpenCPI framework file - DO NOT EDIT - created by xm-worker.mk $(DateStamp) */";\
	  echo "#include <RCC_Worker.h>";\
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
	      echo "  {.name=STR(RCC_FILE_WORKER_$$w), .dispatch=&RCC_FILE_WORKER_$$w},";\
	  done; \
	  echo "  {.name=0}};";\
	 ) > $@

ModelSpecificCleanupHook := \
	  rm -f $(foreach w,$(Workers),$(w).c $(w)_Worker.h);



