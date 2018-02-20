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

ifneq ($(OCPI_HAVE_OPENCL),1)
ifeq ($(filter clean%,$(MAKECMDGOALS)),)
$(info The vsadd example requires OpenCL to be present, and it isn't)
$(info It is usually found as an libOpenCL.so in the standard library path)
endif
all:
run:
clean:
else

OcpiApp=test_vsadd

OCPI_LD_FLAGS+= $(OCPI_OCL_LIBS)

include $(OCPI_CDK_DIR)/include/util.mk
export OCPI_LIBRARY_PATH:=$(OcpiAbsPathToContainingProject)/components/lib/ocl

include $(OCPI_CDK_DIR)/include/application.mk

endif
