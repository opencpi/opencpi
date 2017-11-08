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

# TODO: This content can be used for generating package-name files from the worker
# or hdl-platform level:
#
# Determine whether you are in a library or parent is a library
# If so, package suffix is '.<lib-name>' unless the library is named components.
# For other directory types (e.g. platforms/platform), no suffix is auto-appended
# based on directory name as is done with libraries
#GetPackageSuffix:=$(strip \
#  $(if $(filter $(call OcpiGetDirType,.),library),\
#    $(if $(filter-out components,$(CwdName)),.$(CwdName)),\
#    $(if $(filter $(call OcpiGetDirType,..),library),\
#      $(if $(filter-out components,$(notdir $(abspath ..))),.$(notdir $(abspath ..))))))

# Determine where the 'lib' directory is relative to the current location
#LibIsInCwd=$(filter $(call OcpiGetDirType,.),lib library hdl-platforms)
#LibIsInParent=$(filter $(call OcpiGetDirType,../),lib library hdl-platforms)

###############################################################################
# Determine the package of the current asset
#
# Hierarchy of Package naming
# Generally, the full package qualifier is PackagePrefix.PackageName
# PackagePrefix defaults to full package qualifier of the parent
# PackageName defaults to notdir of the current directory
#   (unless notdir is components)
# If Package is set, it overrides the full package qualifier
#
# For legacy support:
#   One exception where Package does not override PackagePrefix.PackageName
#   is if Package starts with a '.'. In this case, Prefix.Package is used.

# If the PackagePrefix is not set, set it to ProjectPackage
ifeq ($(PackagePrefix),)
  PackagePrefix:=$(ProjectPackage)
endif

# If the PackageName is not set, set it to dirname
#   (or blank if dirname == components)
ifeq ($(PackageName),)
   PackageName:=$(if $(filter-out components,$(notdir $(call OcpiAbsDir,.))),$(notdir $(call OcpiAbsDir,.)))
endif
# If PackageName is nonempty, prepend it with '.'
PackageName:=$(if $(PackageName),.$(patsubst .%,%,$(PackageName)))

# If package is not set, set it to Package
#   set it to $(PackagePrefix)$(PackageName)
# Otherwise, if Package starts with '.',
#   set it to$(CurrentPackagePrefix)$(Package)
ifeq ($(Package),)
  Package:=$(PackagePrefix)$(PackageName)
else
  ifneq ($(filter .%,$(Package)),)
    Package:=$(PackagePrefix)$(Package)
  endif
endif
###############################################################################

# Check/Generate the package-name file
#
# Do nothing about packages if we are cleaning
ifeq ($(filter clean%,$(MAKECMDGOALS)),)
  PackageFile:=lib/package-name
  ifeq ($(call OcpiExists,lib),)
    $(shell mkdir -p lib)
  endif
  # If package-name file does not yet exist, create it based on Package
  ifeq ($(call OcpiExists,$(PackageFile)),)
    $(shell echo $(Package) > $(PackageFile))
  else
    # If package-name file already exists, make sure its contents match Package
    PackageName:=$(shell cat $(PackageFile))
    ifneq ($(Package),$(PackageName))
      $(error Package "$(Package)" and "$(PackageName)" do not match. You must make clean after changing the package name.)
    endif
  endif
endif
