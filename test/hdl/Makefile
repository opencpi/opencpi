
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

#ifndef OCPI_OS
#OCPI_OS=$(shell uname -s | tr A-Z a-z)
#endif
# Use of -static requires optional glibc-static package
# Red Hat advises against the use of -static
#ifneq ($(OCPI_OS),macos)
#DOSTATIC=-static
#endif
#PROC:=$(shell uname -m)
ifeq ($(OCPI_OS),macos)
#ifeq ($(PROC),i386)
#PROC:=x86_64
#endif
else
BASICLIBS= -lrt # ocfrp_check needs this, only on linux
endif
ifeq ($(OCPI_CROSS_HOST),)
CC = gcc
CXX = c++
LD = c++
else
CC = $(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-gcc
CXX = $(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-c++
LD = $(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-c++
AR = $(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-ar
endif
#OCPI_OUT_DIR = $(OCPI_OS)-$(PROC)-bin
ALLC=$(basename $(notdir $(wildcard src/*.c)))
ALLBINC=$(ALLC:%=$(OCPI_OUT_DIR)/%)
ALLCXX=$(basename $(notdir $(wildcard src/*.cxx)))
ALLBINCXX=$(ALLCXX:%=$(OCPI_OUT_DIR)/%)
$(OCPI_OUT_DIR)/cosgen: LIBS=-lm
$(OCPI_OUT_DIR)/nft: LIBS=-lpthread
$(OCPI_OUT_DIR)/ocfrp_check: LIBS=-L ../../lib/$(OCPI_OUT_DIR) -lutil -llogger -locpios
all: $(ALLBINC) $(ALLBINCXX)
$(OCPI_OUT_DIR):
	mkdir $@

$(ALLBINC): $(OCPI_OUT_DIR)/%: src/%.c | $(OCPI_OUT_DIR)
	$(CC) -o $@ -std=c99 -I../../core/hdl/include $(DOSTATIC) -Wall -g $^ $(LIBS) $(BASICLIBS)
$(ALLBINCXX): $(OCPI_OUT_DIR)/%: src/%.cxx | $(OCPI_OUT_DIR)
	$(CXX) -o $@ \
	-I../../core/hdl/include \
	-I../../core/util/uuid/include \
	-I../../core/util/property/include \
	-I../../core/util/ezxml/include \
	-I../../core/util/misc/include \
	-I../../core/util/vfs/include \
	-I../../core/util/res/include \
	-I../../adapt/os/ocpios/interfaces/include \
	-I../../core/dataplane/rdma_driver_interface/interfaces/include \
	$(DOSTATIC) -Wall -g $^ $(LIBS) $(BASICLIBS)

clean:
	rm -r -f target-$(or $(CleanTarget),*)



