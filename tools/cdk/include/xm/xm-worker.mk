# Makefile for an X-Midas wrapped worker
Model=xm
# Default is that you are building in a subdirectory of all implementations
ifeq "$(Target)" ""
Target:=$(shell uname -s)=$(shell uname -r)=$(shell uname -p)=gcc=$(shell gcc -dumpversion)=$(shell gcc -dumpmachine)
endif
ImplSuffix=_Worker.h
SkelSuffix=_skel.c
SourceSuffix=.c
OBJ:=.o
ifeq ($(shell uname),Linux)
BF=.so
SharedLibLinkOptions=-shared
SharedLibCompileOptions=-fPIC
else
ifeq ($(shell uname),Darwin)
BF=.dylib
SharedLibLinkOptions=-dynamiclib
SharedLibCompileOptions=
endif
endif
DispatchSourceFile = $(GeneratedDir)/$(word 1,$(Workers))_dispatch.c
GeneratedSourceFiles += $(DispatchSourceFile)
ArtifactXmlFile = $(GeneratedDir)/$(word 1,$(Workers))_art.xml
MainRoutine=ocpi_xm_mainroutine_$(word 1,$(Workers))
GCC=gcc
GCCLINK=gcc
$(info Target is $(Target))
ifeq ($(Target),Linux-MCS_864x)
GCC=/opt/timesys/toolchains/ppc86xx-linux/bin/ppc86xx-linux-gcc
GCCLINK=$(GCC)
else ifeq ($(Target),Linux-x86_32)
GCC=gcc -m32
GCCLINK=gcc -m32 -m elf_i386
else
Target=$(HostTarget)
endif
LinkBinary=$(GCCLINK) $(SharedLibLinkOptions) -o $@ $(ObjectFiles) $(OtherLibraries) $(AEPLibraries)
Compile=$(GCC) -MMD -MP -MF $(GeneratedDir)/$(basename $(notdir $@)).deps -c -Wall -g $(SharedLibCompileOptions) $(IncludeDirs:%=-I%) -I$(OCPI_DIR)/include/rcc -D"OCPI_XM_MAINROUTINE=$(MainRoutine)" -o $@ $(patsubst %.for,%.c,$<)

FC = /usr/bin/pfc

CompileFortran = \
	$(FC) $*.for R$(OCPI_DIR)/include/xm/xmpfc.cnf; \
	mv $*.c $*.cxxx; \
	(sh -c 'echo -e "\n\#include \"ocpi_xm_intercept_fortran.h\"\n\#include \"ocpi_xm_fortran_buffers.h\"\n\n\#ifndef INCLUDED_FORTRAN_H\n\#define INCLUDED_FORTRAN_H\n\#define LPROTOTYPE\n\#include \"fortran.h\"\n\#endif\n\n"'; cat $*.cxxx | sed 's/U[0-9]*\.//g' | grep -v "\#include" | grep -v LPROTOTYPE) > $*.c; \
	rm -f $*.cxxx $*.hf;

include $(OCPI_DIR)/include/xxx-worker.mk

XmAssemblyFile=$(GeneratedDir)/$(word 1,$(Workers))_assy.xml
$(XmAssemblyFile):
	$(AT)(echo "<RccAssembly Name=\""$(word 1,$(Workers))"\">"; \
	  for w in $(Workers); do echo "<Worker File=\"$$w.xml\"/>"; done; \
	  echo "</RccAssembly>") > $@

$(ArtifactXmlFile): $(XmAssemblyFile)
	@echo Generating artifact/runtime xml file \($(ArtifactXmlFile)\) for all workers in one binary
	$(AT)$(OcpiGen) -A $(XmAssemblyFile)

#disable builtin suffix rules
%.o : %.c

$(DispatchSourceFile):
	$(AT)echo Generating dispatch file: $@
	$(AT)(echo "#include <RCC_Worker.h>";\
	  echo "extern RCCDispatch "`echo $(strip $(Workers))|tr ' ' ','`";";\
	  echo "RCCEntryTable ocpi_EntryTable[] = {";\
	  for w in $(Workers); do \
	      echo "  {.name=\""$$w"\", .dispatch=&"$$w"},";\
	  done; \
	  echo "  {.name=0}};";\
	 ) > $@

.PHONY: generate_xm_wrapper
