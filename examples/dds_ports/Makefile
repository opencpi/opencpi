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

ifneq (OCPI_HAVE_OPENSPLICE,1)
#$(info Set OCPI_OPENSPLICE_HOME to the OpenSplice install location to use this example (e.g. /usr/local))
$(info OpenSplice/DDS not available.)
all:
run:
clean:
else
APP=dds_ports
include ../setup.mk

OPENSPLICE_LIBS= dcpssacpp
OCPI_LD_FLAGS+=\
  -L$(OCPI_OPENSPLICE_HOME)/lib \
  -Xlinker -rpath -Xlinker $(OCPI_OPENSPLICE_HOME)/lib \
  $(OPENSPLICE_LIBS:%=-l%)
OCPI_API_LIBS+= msg_driver_interface msg_drivers

$(PROG): SATests

SATests:
	make -C ./standalone_dds/Test/C++/Standalone/Posix

run: $(PROG) SATests
	@if test "$(OCPI_HAVE_OPENSPLICE)" == ""; then \
	  echo "*******Cannot execute OpenSplice on this machine for dds testing. Skipping execution." ; \
	else \
	. $(OCPI_OPENSPLICE_HOME)/release.com; ospl start; \
	( OCPI_LIBRARY_PATH=lib/rcc $(PROG) & )   ; \
	sleep 2; \
	./standalone_dds/Test/C++/Standalone/Posix/exec/TestPublisher  ; \
	fi

clean::
        make -C ./standalone_dds/Test/C++/Standalone/Posix clean

# These make the worker library for this application
Implementations = dds_producer.rcc dds_consumer.rcc
include $(OCPI_CDK_DIR)/include/lib.mk
endif
