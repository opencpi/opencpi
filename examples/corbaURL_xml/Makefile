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

ifeq (,)
run:
clean::
else
ifndef OCPI_CDK_DIR
# if not set, assume we are either in the development tree or in an exported cdk
OCPI_CDK_DIR=$(or $(wildcard ../../export),../..)
endif
APP=corbaHello
include ../setup.mk

OCPI_LD_FLAGS+=-L$(OCPI_OMNI_LIBRARY_DIR) -lomniORB4 -lomnithread -Xlinker -rpath -Xlinker $(OCPI_OMNI_LIBRARY_DIR)

SERVER=$(OCPI_TARGET_DIR)/server
$(PROG):  $(SERVER)

hello.hh: hello.idl
	$(OCPI_OMNI_BIN_DIR)/omniidl -bcxx -Wbh=.hh -Wbd=.cxx -Wba $^

# This is just a CORBA program, so we don't use load all of opencpi
$(SERVER): OCPI_API_LIBS=$(OCPI_TRANSPORT_LIBS)
$(SERVER): hello.hh helloSK.cc server.cxx 
	g++ -Wall -g -o $@ -I$(OCPI_INC_DIR) $(OCPI_CORBA_INCLUDE_DIRS:%=-I%) $(OCPI_LD_FLAGS) server.cxx helloSK.cc

$(SERVER): server.cxx
# OCPI_LIBRARY_PATH is like LD_LIBRARY_PATH, but for OpenCPI components.
run: all
	OCPI_LIBRARY_PATH=lib/rcc $(VG) ./corbaHello > /dev/null	

clean::
	rm -f -r hello.hh helloSK.cc

# These make the worker library for this application
Implementations = hello.rcc
include $(OCPI_CDK_DIR)/include/lib.mk
endif
