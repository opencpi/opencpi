# #####
#
#  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2011
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

# Makefile for OCL common definitions not specific to "workers"
ifndef OCL_MAKE_MK
OCL_MAKE_MK:=xxx
include $(OCPI_CDK_DIR)/include/util.mk

ifndef OclTargets
  OclTargets:=$(OclTarget)
  ifndef OclTargets
    OclTargets:=all
  endif
endif
ifeq ($(OclTargets),all)
  override OclTargets:=$(shell $(ToolsDir)/ocpiocl targets)
endif

$(call OcpiDbgVar,OclTarget)
$(call OcpiDbgVar,OclTargets)

OclOs=opencl
OclOsVersion=$(word 1,$(subst -, ,$1))
OclArch=$(word 2,$(subst -, ,$1))

endif
