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

# helpers for opencv makefiles
ifndef OCPI_OPENCV_HOME
ifeq ($(filter clean%,$(MAKECMDGOALS)),)
$(info Set OCPI_OPENCV_HOME to the OpenCV install location to use this example (e.g. /usr/local))
$(info OpenCV not available.)
endif
all:
run:
clean:
EXIT=sdf
else
include ../setup.mk

OPENCV_LIBS=opencv_core opencv_highgui opencv_ml opencv_objdetect
OPENCV_LIB_DIR=$(OCPI_OPENCV_HOME)
OCPI_LD_FLAGS+= \
        -Xlinker -rpath -Xlinker $(OPENCV_LIB_DIR)/lib -L$(OPENCV_LIB_DIR)/lib $(OPENCV_LIBS:%=-l%)

INCS += -I$(OCPI_OPENCV_HOME)/include/opencv -I$(OCPI_OPENCV_HOME)/include

OPENCV_ENV= export $(OcpiLibraryPathEnv)=$(OPENCV_LIB_DIR)/lib;

endif
