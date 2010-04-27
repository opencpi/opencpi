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

CPI_RCC_BINDER = $(CPIDIR)/tools/local/binder/linux-bin/cpi-rcc-binder
CPI_RCC_GEN = $(CPIDIR)/tools/local/binder/linux-bin/cpi-rcc-gen
CFLAGS += -I.

ifeq ($(shell if test -x $(CPI_RCC_BINDER) ; then echo "ok" ; fi),ok)
ifeq ($(SYSTEM),linux)
EXT=so
else
EXT=out
endif

ifeq ($(OUTDIR),)
OUTDIR=.
endif

all: objdir $(OUTDIR)/rot.zip

objdir:
	if test "x$(OUTDIR)" != "x." ; then mkdir -p $(OUTDIR) ; fi

$(OUTDIR)/rot.zip: $(OUTDIR)/rot.$(EXT) \
		output/rot.scd.xml output/rot_SCD.prf.xml
	$(CPI_RCC_BINDER) \
		--output=$@ \
		--workerDll=$(OUTDIR)/rot.$(EXT) \
		--worker=rotWorker=output/rot.scd.xml

rot.h: output/rot.scd.xml output/rot_SCD.prf.xml
	$(CPI_RCC_GEN) --name Rot --gen-header-file=$@ $<

$(OUTDIR)/rot.o: rot.c rot.h

ifeq ($(SYSTEM),linux)

ifeq ($(CROSS_HOST),)
CC = gcc
else
CC = $(CROSS_BUILD_BIN)/$(CROSS_HOST)-gcc
endif

$(OUTDIR)/%.so : $(OUTDIR)/%.o
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
	rm -rf rot.h *.zip *.out *.so *.o *~
