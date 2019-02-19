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

##################################################################################################
# This file initializes the database of possible RCC Platforms, including os/os_version/arch info.
# This file is directly included when that info is needed.
# It does NOT read/include the software platform definition files for the details of the platform,
# but only teases out the os/os_version/arch aspects.
# It should be considered a leaf, light file, but it does depend on the project context
# since the set of available platforms is based on registered projects or OCPI_PROJECT_PATH.
# If it is not called in a project context, then the default registry is used.
#
# It does not call rcc-make or rcc-worker, they call it.
#
# This file will pull RCC platform information from the environment if is already set,
# avoiding any file system interaction/overhead.
# Setting ShellRccTargetsVars=1 causes it to dump the database to standard output

ifndef RccAllPlatforms
ifdef OCPI_ALL_RCC_PLATFORMS
  # If the environment already has our database, import it into make variables
  RccAllPlatforms:=$(OCPI_ALL_RCC_PLATFORMS)
  RccAllTargets:=$(OCPI_ALL_RCC_TARGETS)
  $(foreach p,$(OCPI_RCC_PLATFORM_TARGETS),\
    $(eval RccTarget_$(word 1,$(subst =, ,$p)):=$(word 2,$(subst =, ,$p))))
else
  include $(OCPI_CDK_DIR)/include/util.mk
  # This is a big hammer here, since all we need is the registry.
  # FIXME: have a lighter weight way to do this.
  $(OcpiIncludeProject)
  RccAllPlatforms:=
  RccAllTargets:=
  # Add the platform $1 and its target $2 and directory $3 to our database
  RccAddPlatform=\
    $(eval RccAllPlatforms=$(strip $(RccAllPlatforms) $1))\
    $(eval RccAllTargets=$(strip $(RccAllTargets) $2))\
    $(eval RccTarget_$1:=$2)\
    $(eval RccPlatformDir_$1:=$3)
  # roughly for backward compatibility, same as getPlatform.sh
  RccGetPlatformTarget=$(strip\
    $(eval vars:=$(shell egrep '^ *OcpiPlatform(Os|Arch|OsVersion) *:*= *' $1/$(notdir $1).mk |\
                   sed 's/OcpiPlatform\([^ :=]*\) *:*= *\([^a-zA-Z0-9_]*\)/\1 \2/'|sort))\
    $(infox VARS:$(words $(vars)):$(vars))\
    $(if $(filter 6,$(words $(vars))),,$(error Invalid platform file $1/$(notdir $1).mk))\
    $(word 4,$(vars))-$(word 6,$(vars))-$(word 2,$(vars)))
  $(foreach d,$(OcpiGetRccPlatformPaths),\
    $(foreach p,$(wildcard $d/*),\
      $(and $(wildcard $p/$(notdir $p).mk),\
        $(call RccAddPlatform,$(notdir $p),$(call RccGetPlatformTarget,$p),$p))))
  RccAllTargets:=$(call Unique,$(RccAllTargets)) # this should not be an issue, but...
  export OCPI_ALL_RCC_PLATFORMS:=$(RccAllPlatforms)
  export OCPI_ALL_RCC_TARGETS:=$(RccAllTargets)
  export OCPI_RCC_PLATFORM_TARGETS:=$(foreach p,$(RccAllPlatforms),$p=$(RccTarget_$p))
  $(call OcpiDbgVar,RccAllPlatforms)
  $(call OcpiDbgVar,RccAllTargets)
  $(foreach p,$(RccAllPlatforms),$(call OcpiDbgVar,RccTarget_$p))
endif # end of the info not being in the environment already
# Assignments that can be used to extract make variables into bash/python...
ifdef ShellRccTargetsVars
all:
$(info RccAllPlatforms="$(sort $(RccAllPlatforms))";\
       RccPlatforms="$(sort $(RccPlatforms))";\
       RccAllTargets="$(sort $(RccAllTargets))";\
       RccTargets="$(sort $(RccTargets))";\
       $(foreach p,$(sort $(RccAllPlatforms)),\
         $(if $(RccTarget_$p),RccTarget_$p="$(RccTarget_$p)";\
		 RccPlatDir_$p="$(realpath $(call OcpiGetRccPlatformDir,$p))";)))
endif
endif # end of the info not being set
