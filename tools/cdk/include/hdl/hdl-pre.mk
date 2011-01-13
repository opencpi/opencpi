
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

ifndef __HDL_PRE_MK__
include $(OCPI_CDK_DIR)/include/util.mk
# Makefile fragment for HDL primitives, cores, and workers etc.

Model=hdl
SourceSuffix=.v
# The directory of the library (given a lib and a target)
LibraryRefDir=$(if $(findstring /,$(1)),$(1),$(OCPI_CDK_DIR)/lib/hdl/$(1))/$(call LibraryAccessTarget,$(2))
CoreRefDir=$(if $(findstring /,$(1)),$(1),$(OCPI_CDK_DIR)/lib/hdl/$(1))/$(2)
LibraryAccessTarget=$(if $(findstring xc5,$(1))$(findstring virtex5,$(1)),virtex5,$(if $(findstring xc6,$(1))$(findstring virtex6,$(1)),virtex6,$(error No family can be determined from "$(1)")))
LibraryFileTarget2=$(if $(filter virtex5,$(1))$(findstring xc5,$(1)),hdllib.ref,$(2).sdbl)
LibraryFileTarget=$(if $(filter virtex5,$(1))$(findstring xc5,$(1)),hdllib.ref,$(LibName).sdbl)
LibraryCleanTargets=$(if $(filter virtex5,$(1))$(findstring xc5,$(1)),hdllib.ref vlg??,*.sdb?)
TargetFamily=$(call LibraryAccessTarget,$(1))
BF:=.ngc
endif
