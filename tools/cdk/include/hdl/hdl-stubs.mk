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

# This internal file is for building a "hdl stubs" library for a component library
# LibName is expected to be set correctly

# Tell all the general purpose and toolset make scripts we are building libraries
HdlMode:=library
include $(OCPI_CDK_DIR)/include/hdl/hdl-pre.mk
ifndef HdlSkip
ifndef LibName
LibName=$(CwdName)
endif
# include the shared file with stuff needed for some core builds too
include $(OCPI_CDK_DIR)/include/hdl/hdl-lib2.mk

# Installation is not recursive
install: | $(HdlInstallDir)
	$(AT)for f in $(HdlActualTargets); do \
	  $(call ReplaceIfDifferent,$(strip \
             $(OutDir)target-$$f/$(LibName)),$(strip \
             $(HdlInstallDir)/$(LibName)/$$f)); \
	done

ifneq ($(Imports)$(ImportCore)$(ImportBlackBox),)
include $(OCPI_CDK_DIR)/include/hdl/hdl-import.mk
endif
endif
