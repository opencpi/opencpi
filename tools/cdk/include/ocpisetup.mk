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

# This make file fragment establishes environment variables for user Makefiles,
# all based on OCPI_CDK_DIR, which might be set already, but may not be.
# If not set, it is assumed that this file is being included from its proper location
# in a CDK and the CDK location is inferred from that.
# This means that a user makefile could simply include this file however it wants and
# OCPI_CDK_DIR and friends will all be set properly.
# The goal here to have *no* environment setup requirement for a user app on the
# development system with default settings.
ifndef OCPI_TARGET_PLATFORM
  export OCPI_TARGET_PLATFORM=$(OCPI_TOOL_PLATFORM)
endif
include $(OCPI_CDK_DIR)/include/setup-target-platform.mk
