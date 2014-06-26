
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



#
# When called from test/Makefile.ocpi, MakeVars.ocpi has already been loaded.
# Else (when called directly), load MakeVars.ocpi.
#

include $(OCPI_BASE_DIR)/MakeVars.ocpi

#
# These must exist natively on the build host.  Assume that the build host
# is Linux, and that it was built using $(OCPI_OUT_DIR)=linux-bin.
#

OCPI_RCC_BINDER = $(OCPI_BASE_DIR)tools/local/binder/$(OCPI_OUT_DIR)/ocpi-rcc-binder
DO_OCPI_RCC_BINDER=\
  export $(OCPI_DYLIB_ENV)=$(OCPI_BASE_DIR)lib/$(OCPI_OUT_DIR);\
  $(OCPI_RCC_BINDER)
OCPI_RCC_GEN = $(OCPI_BASE_DIR)tools/local/binder/$(OCPI_OUT_DIR)/ocpi-rcc-gen
DO_OCPI_RCC_GEN=\
  export $(OCPI_DYLIB_ENV)=$(OCPI_BASE_DIR)lib/$(OCPI_OUT_DIR);\
   $(OCPI_RCC_GEN)
CFLAGS += -I.

ifeq ($(shell if test -x $(OCPI_RCC_BINDER) ; then echo "ok" ; fi),ok)
ifeq ($(OCPI_OS),linux)
EXT=so
ENV=LD_LIBRARY_PATH
else ifeq ($(OCPI_OS),darwin)
EXT=dylib
ENV=DYLD_LIBRARY_PATH
else
EXT=out
endif

ifeq ($(OCPI_OUT_DIR),)
OCPI_OUT_DIR=.
endif

all: $(OCPI_OUT_DIR)/FileCopy.zip | $(OCPI_OUT_DIR)

ifeq ($(OCPI_OS),vxworksdkm)
SGAC_FILES = $(OCPI_BASE_DIR)/core/sca/sgac/$(OCPI_OUT_DIR)/ocpi-generic-assy-controller.out \
	$(OCPI_BASE_DIR)/core/sca/sgac/$(OCPI_OUT_DIR)/libsgac.out
else
SGAC_FILES = $(OCPI_BASE_DIR)/core/sca/sgac/$(OCPI_OUT_DIR)/ocpi-generic-assy-controller
endif

$(OCPI_OUT_DIR):
	$(AT)echo Creating object directory $(OCPI_OUT_DIR); mkdir -p $(OCPI_OUT_DIR)

$(OCPI_OUT_DIR)/FileCopy.zip: $(OCPI_OUT_DIR)/FileSource.zip $(OCPI_OUT_DIR)/FileSink.zip $(OCPI_OUT_DIR)/FileTee.zip $(SGAC_FILES)
	-rm -f $@
	zip -j $@ output/* $^ $(SGAC_FILES)

$(OCPI_OUT_DIR)/FileSource.zip: $(OCPI_OUT_DIR)/FileSource.$(EXT) output/FileSource.scd.xml
	$(DO_OCPI_RCC_BINDER) \
		--output=$@ \
		--workerDll=$(OCPI_OUT_DIR)/FileSource.$(EXT) \
		--worker=FileSourceWorker=output/FileSource.scd.xml

$(OCPI_OUT_DIR)/FileSink.zip: $(OCPI_OUT_DIR)/FileSink.$(EXT) output/FileSink.scd.xml
	$(DO_OCPI_RCC_BINDER) \
		--output=$@ \
		--workerDll=$(OCPI_OUT_DIR)/FileSink.$(EXT) \
		--worker=FileSinkWorker=output/FileSink.scd.xml

$(OCPI_OUT_DIR)/FileTee.zip: $(OCPI_OUT_DIR)/FileTee.$(EXT) output/FileTee.scd.xml
	$(DO_OCPI_RCC_BINDER) \
		--output=$@ \
		--workerDll=$(OCPI_OUT_DIR)/FileTee.$(EXT) \
		--worker=FileTeeWorker=output/FileTee.scd.xml

FileSource.h: output/FileSource.scd.xml output/FileSourceComponent.prf.xml
	$(DO_OCPI_RCC_GEN) --name FileSource --gen-header-file=$@ $<

FileSink.h: output/FileSink.scd.xml output/FileSinkComponent.prf.xml
	$(DO_OCPI_RCC_GEN) --name FileSink --gen-header-file=$@ $<

FileTee.h: output/FileTee.scd.xml output/FileTeeComponent.prf.xml
	$(DO_OCPI_RCC_GEN) --name FileTee --gen-header-file=$@ $<

$(OCPI_OUT_DIR)/FileSource.o: FileSource.c FileSource.h | $(OCPI_OUT_DIR)
$(OCPI_OUT_DIR)/FileSink.o: FileSink.c FileSink.h | $(OCPI_OUT_DIR)
$(OCPI_OUT_DIR)/FileTee.o: FileTee.c FileTee.h | $(OCPI_OUT_DIR)

ifneq ($(filter linux darwin,$(OCPI_OS)),)

ifeq ($(OCPI_CROSS_HOST),)
CC = gcc
else
CC = $(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-gcc
endif

$(OCPI_OUT_DIR)/%.$(EXT) : $(OCPI_OUT_DIR)/%.o
	$(CC) -shared -o $@ $(LDFLAGS) $<

$(OCPI_OUT_DIR)/%.o : %.c
	$(CC) -fPIC -c -o $@ $(CFLAGS) $<
else
WIND_HOME := /opt/WindRiver
WIND_BASE := $(WIND_HOME)/vxworks-6.3
WIND_HOST_TYPE := x86-linux2
WIND_LIC_PROXY := $(WIND_HOME)/setup/$(WIND_HOST_TYPE)/bin
export WIND_HOME WIND_BASE WIND_LIC_PROXY

$(OCPI_OUT_DIR)/%.out : $(OCPI_OUT_DIR)/%.o
	/opt/WindRiver/gnu/3.4.4-vxworks-6.3/x86-linux2/bin/ccppc \
		-o $@ -r -nostdlib -Wl,-X \
                -T /opt/WindRiver/vxworks-6.3/target/h/tool/gnu/ldscripts/link.OUT \
		$<

$(OCPI_OUT_DIR)/%.o : %.c
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
# $(OCPI_RCC_BINDER) does not exist.  Don't build this test.
#
all:
endif

clean:
	rm -r -f target-$(or $(CleanTarget),*)
	rm -rf FileSink.h FileSource.h FileTee.h
	rm -rf *.zip *.out *~
