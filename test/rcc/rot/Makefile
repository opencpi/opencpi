
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

OCPI_RCC_BINDER=$(OCPI_BASE_DIR)tools/local/binder/$(OCPI_OUT_DIR)/ocpi-rcc-binder
DO_OCPI_RCC_BINDER=\
  export $(OCPI_DYLIB_ENV)=$(OCPI_BASE_DIR)lib/$(OCPI_OUT_DIR);\
  $(OCPI_RCC_BINDER)
OCPI_RCC_GEN = $(OCPI_BASE_DIR)tools/local/binder/$(OCPI_OUT_DIR)/ocpi-rcc-gen
DO_OCPI_RCC_GEN=\
  export $(OCPI_DYLIB_ENV)=$(OCPI_BASE_DIR)lib/$(OCPI_OUT_DIR);\
  $(OCPI_RCC_GEN)
CFLAGS += -I.
EXT=$(OCPI_LIB_EXT)

all: $(OCPI_OUT_DIR)/rot.zip | objdir $(OCPI_OUT_DIR)

objdir $(OCPI_OUT_DIR):
	$(AT)if test "x$(@)" != "x." ; then echo Creating object directory $@; mkdir -p $@ ; fi


$(OCPI_OUT_DIR)/rot.zip: $(OCPI_OUT_DIR)/rot.$(EXT) output/rot.scd.xml output/rot_SCD.prf.xml
	$(DO_OCPI_RCC_BINDER) \
		--output=$@ \
		--workerDll=$(OCPI_OUT_DIR)/rot.$(EXT) \
		--worker=rotWorker=output/rot.scd.xml

rot.h: output/rot.scd.xml output/rot_SCD.prf.xml
	$(DO_OCPI_RCC_GEN) --name Rot --gen-header-file=$@ $<

$(OCPI_OUT_DIR)/rot.o: rot.c rot.h | $(OCPI_OUT_DIR)

ifeq ($(OCPI_CROSS_HOST),)
CC = gcc
else
CC = $(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-gcc
endif

$(OCPI_OUT_DIR)/%.$(EXT) : $(OCPI_OUT_DIR)/%.o
	$(CC) -shared -o $@ $(LDFLAGS) $<

$(OCPI_OUT_DIR)/%.o : %.c
	$(CC) -fPIC -c -o $@ $(CFLAGS) $<
ifneq (,)
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

clean:
	if test "x$(OCPI_OUT_DIR)" != "x." ; then rm -rf $(OCPI_OUT_DIR) ; fi
	rm -rf rot.h *.zip *.out *.dylib *.so *.o *~
