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

#$(info start $(MAKECMDGOALS))
APP=hello
include ../setup.mk

#$(info beforex $(MAKECMDGOALS))
#.PHONY: run
run: all
	OCPI_LIBRARY_PATH=lib $(VG) $(PROG) $(OUT)

#$(info aftery $(MAKECMDGOALS))

# These make the worker library for this application
Implementations = hello.rcc
include $(OCPI_CDK_DIR)/include/lib.mk
