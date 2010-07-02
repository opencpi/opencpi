# Copyright (c) 2009 Mercury Federal Systems.
# 
# This file is part of OpenCPI.
# 
# OpenCPI is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# OpenCPI is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

#
# When called from test/Makefile.cpi, MakeVars.cpi has already been loaded.
# Else (when called directly), load MakeVars.cpi.
#

ifeq ($(SYSTEM),)
include ../../../MakeVars.cpi
endif

#
# These must exist natively on the build host.  Assume that the build host
# is Linux, and that it was built using $(OUTDIR)=linux-bin.
#

CPI_RCC_BINDER = $(CPIDIR)/tools/local/binder/$(SYSTEM)-bin/cpi-rcc-binder
DO_CPI_RCC_BINDER=echo 0 is $$0;\
  export $(ENV)=$(CPIDIR)/lib/$(SYSTEM)-bin;\
  $(CPI_RCC_BINDER)
CPI_RCC_GEN = $(CPIDIR)/tools/local/binder/$(SYSTEM)-bin/cpi-rcc-gen
CFLAGS += -I.

ifeq ($(shell if test -x $(CPI_RCC_BINDER) ; then echo "ok" ; fi),ok)
ifeq ($(SYSTEM),linux)
EXT=so
ENV=LD_LIBRARY_PATH
else ifeq ($(SYSTEM),macos)
EXT=dylib
ENV=DYLD_LIBRARY_PATH
else
EXT=out
endif

ifeq ($(OUTDIR),)
OUTDIR=.
endif

all: objdir $(OUTDIR)/FileCopy.zip

ifeq ($(SYSTEM),vxworksdkm)
SGAC_FILES = $(CPIDIR)/core/sca/sgac/$(OUTDIR)/cpi-generic-assy-controller.out \
	$(CPIDIR)/core/sca/sgac/$(OUTDIR)/libsgac.out
else
SGAC_FILES = $(CPIDIR)/core/sca/sgac/$(OUTDIR)/cpi-generic-assy-controller
endif

objdir:
	if test "x$(OUTDIR)" != "x." ; then mkdir -p $(OUTDIR) ; fi

$(OUTDIR)/FileCopy.zip: $(OUTDIR)/FileSource.zip $(OUTDIR)/FileSink.zip $(OUTDIR)/FileTee.zip $(SGAC_FILES)
	-rm -f $@
	zip -j $@ output/* $^ $(SGAC_FILES)

$(OUTDIR)/FileSource.zip: $(OUTDIR)/FileSource.$(EXT) output/FileSource.scd.xml
	$(DO_CPI_RCC_BINDER) \
		--output=$@ \
		--workerDll=$(OUTDIR)/FileSource.$(EXT) \
		--worker=FileSourceWorker=output/FileSource.scd.xml

$(OUTDIR)/FileSink.zip: $(OUTDIR)/FileSink.$(EXT) output/FileSink.scd.xml
	$(DO_CPI_RCC_BINDER) \
		--output=$@ \
		--workerDll=$(OUTDIR)/FileSink.$(EXT) \
		--worker=FileSinkWorker=output/FileSink.scd.xml

$(OUTDIR)/FileTee.zip: $(OUTDIR)/FileTee.$(EXT) output/FileTee.scd.xml
	$(DO_CPI_RCC_BINDER) \
		--output=$@ \
		--workerDll=$(OUTDIR)/FileTee.$(EXT) \
		--worker=FileTeeWorker=output/FileTee.scd.xml

FileSource.h: output/FileSource.scd.xml output/FileSourceComponent.prf.xml
	$(CPI_RCC_GEN) --name FileSource --gen-header-file=$@ $<

FileSink.h: output/FileSink.scd.xml output/FileSinkComponent.prf.xml
	$(CPI_RCC_GEN) --name FileSink --gen-header-file=$@ $<

FileTee.h: output/FileTee.scd.xml output/FileTeeComponent.prf.xml
	$(CPI_RCC_GEN) --name FileTee --gen-header-file=$@ $<

$(OUTDIR)/FileSource.o: FileSource.c FileSource.h
$(OUTDIR)/FileSink.o: FileSink.c FileSink.h
$(OUTDIR)/FileTee.o: FileTee.c FileTee.h

ifneq ($(filter linux macos,$(SYSTEM)),)

ifeq ($(CROSS_HOST),)
CC = gcc
else
CC = $(CROSS_BUILD_BIN)/$(CROSS_HOST)-gcc
endif

$(OUTDIR)/%.$(EXT) : $(OUTDIR)/%.o
	$(CC) -shared -o $@ $(LDFLAGS) $<

$(OUTDIR)/%.o : %.c
	$(CC) -fPIC -c -o $@ $(CFLAGS) $<
else
WIND_HOME := /opt/WindRiver
WIND_BASE := $(WIND_HOME)/vxworks-6.3
WIND_HOST_TYPE := x86-linux2
WIND_LIC_PROXY := $(WIND_HOME)/setup/$(WIND_HOST_TYPE)/bin
export WIND_HOME WIND_BASE WIND_LIC_PROXY

$(OUTDIR)/%.out : $(OUTDIR)/%.o
	/opt/WindRiver/gnu/3.4.4-vxworks-6.3/x86-linux2/bin/ccppc \
		-o $@ -r -nostdlib -Wl,-X \
                -T /opt/WindRiver/vxworks-6.3/target/h/tool/gnu/ldscripts/link.OUT \
		$<

$(OUTDIR)/%.o : %.c
	/opt/WindRiver/gnu/3.4.4-vxworks-6.3/x86-linux2/bin/ccppc \
		-c -o $@ -g -Wall -fPIC \
		-mcpu=8540 -mabi=no-spe -msoft-float -mstrict-align \
		-mregnames -mlongcall -D_WRS_KERNEL \
		-I/opt/WindRiver/vxworks-6.3/target/h \
		-I/opt/WindRiver/vxworks-6.3/target/h/wrn/coreip \
		-I. $<
endif
else
#
# $(CPI_RCC_BINDER) does not exist.  Don't build this test.
#
all:
endif

clean:
	if test "x$(OUTDIR)" != "x." ; then rm -rf $(OUTDIR) ; fi
	rm -rf FileSink.h FileSource.h FileTee.h
	rm -rf *.zip *.out *.so *.o *~
