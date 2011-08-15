
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

ifndef OCPI_OS
OCPI_OS=$(shell uname -s | tr A-Z a-z)
endif
# Use of -static requires optional glibc-static package
# Red Hat advises against the use of -static
#ifneq ($(OCPI_OS),darwin)
#DOSTATIC=-static
#endif
PROC:=$(shell uname -m)
ifeq ($(OCPI_OS),darwin)
ifeq ($(PROC),i386)
PROC:=x86_64
endif
else
BASICLIBS=-luuid # ocfrp_check needs this, only on linux
endif
OCPI_OUT_DIR = $(OCPI_OS)-$(PROC)-bin
ALL=$(basename $(notdir $(wildcard src/*.c)))
ALLBIN=$(ALL:%=$(OCPI_OUT_DIR)/%)
$(OCPI_OUT_DIR)/cosgen: LIBS=-lm
$(OCPI_OUT_DIR)/nft: LIBS=-lpthread
all: $(ALLBIN)
$(OCPI_OUT_DIR):
	mkdir $@

$(ALLBIN): $(OCPI_OUT_DIR)/%: src/%.c | $(OCPI_OUT_DIR)
	gcc -o $@ -std=c99 -I../../core/container/hdl_container/impl/include $(DOSTATIC) -Wall -g $^ $(LIBS) $(BASICLIBS)

clean:
	rm -r -f *-bin


