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

APP=bestfit
include ../opencv.mk
ifndef EXIT

run: $(PROG) all
	$(OPENCV_ENV) \
	export OCPI_LIBRARY_PATH=lib; \
	($(VG) $(PROG) 9999 .001 10 && \
	$(VG) $(PROG) 24000 .002 1000 && \
	$(VG) $(PROG) 40000 .001 100 )  > /dev/null

# These make the worker library for this application
Implementations = impl1.rcc impl2.rcc impl3.rcc
include $(OCPI_CDK_DIR)/include/lib.mk
endif
