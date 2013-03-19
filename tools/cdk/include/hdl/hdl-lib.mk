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

# This file should be include in makefiles for hdl primitive libraries,
# compiled from sources.  The idea is to do as much precompilation as possible.
# Some tools do more than others, but this is a LIBRARY, which means we are not
# combining things together such that they can't be used separately.

# Tell all the general purpose and toolset make scripts we are building libraries
HdlMode:=library
include $(OCPI_CDK_DIR)/include/hdl/hdl-pre.mk
.PHONY: stublibrary
ifndef HdlSkip
ifndef LibName
LibName=$(CwdName)
endif
ifdef HdlToolNeedBB
stublibrary: install
else
stublibrary:
	$(AT)echo No stub library necessary for: $(HdlActualTargets)
endif
include $(OCPI_CDK_DIR)/include/hdl/hdl-lib2.mk

# only install if there is an install dir
ifneq ($(HdlInstallDir)$(HdlInstallLibDir),)

# This can be overriden
HdlInstallLibDir=$(HdlInstallDir)/$(LibName)
$(HdlInstallLibDir):
	$(AT)echo Creating directory $@ for library $(LibName)
	$(AT)mkdir -p $@

install: $(OutLibFiles) | $(HdlInstallLibDir)
	$(AT)for f in $(HdlActualTargets); do \
	  $(call ReplaceIfDifferent,$(strip \
             $(OutDir)target-$$f/$(HdlToolLibraryResult)),$(strip \
             $(HdlInstallLibDir)/$$f)); \
	done

else
install:

endif # end of installation if there is an install dir

endif

ifneq ($(Imports)$(ImportCore)$(ImportBlackBox),)
include $(OCPI_CDK_DIR)/include/hdl/hdl-import.mk
endif
