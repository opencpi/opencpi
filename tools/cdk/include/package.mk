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
include $(OCPI_CDK_DIR)/include/util.mk

###############################################################################
# Determine the package of the current asset
#
# Hierarchy of Package naming
# Generally, the full package qualifier is PackagePrefix.PackageName
# PackagePrefix defaults to full package qualifier of the parent (ParentPackage)
# PackageName defaults to notdir of the current directory
#   (unless notdir is components, in which case it is empty)
# If Package is set, it overrides the full package qualifier.
#   This means that if Package is set, all other *Package* variables are ignored
#
# For legacy support:
#   One exception where Package does not override PackagePrefix.PackageName
#   is if Package starts with a '.'. In this case, Prefix.Package is used.
#
# Arg1 is directory to operate on
###############################################################################

define OcpiCreatePackageId

###############################################################################
# Determine where the 'lib' directory is relative to the current location
LibIsInCwd=$$(filter lib library %-platform %-platforms,$$(call OcpiGetDirType,$$(or $1,./)))
LibIsInParent=$$(filter lib library %-platform %-platforms,$$(call OcpiGetDirType,$$(or $1,.)/../))

# Determine the directory containing the 'lib' directory to place package-id
DirContainingLib=$$(if $$(LibIsInCwd),$$(or $1,.)/,$$(and $$(LibIsInParent),$$(or $1,.)/../))

# For legacy support, we need to expect Package= in library/Makefile
# So, we grep for it here to include if building from the worker level
#   (if building from the worker level, <library>/Library.mk is included,
#    but not <library>/Makefile)
ifneq ($$(LibIsInParent),)
  $$(eval $$(shell grep "^\s*Package\s*:\?=\s*.*" $$(or $1,.)/../Makefile))
endif

###############################################################################
# If ParentPackage is unset, assume the parent is the project
ifeq ($$(ParentPackage),)
  export ParentPackage:=$$(ProjectPackage)
endif

# If the PackagePrefix is not set, set it to ParentPackage
ifeq ($$(PackagePrefix),)
  export PackagePrefix:=$$(ParentPackage)
endif

###############################################################################
# If the PackageName is not set, set it to dirname
#   (or blank if dirname == components)
ifeq ($$(PackageName),)
  # Get the 'notdir' of the directory containing 'lib' (CWD or parent)
  # Return that 'notdir' unless it is components in which case return empty
  export PackageName:=$$(filter-out components,$$(strip $$(if $$(LibIsInCwd),\
                                                          $$(notdir $$(call OcpiAbsDir,$$(or $1,.))),\
                                                          $$(if $$(LibIsInParent),\
                                                            $$(notdir $$(call OcpiAbsDir,$$(or $1,.)/..))))))
endif

# If PackageName is nonempty, prepend it with '.'
export PackageName:=$$(if $$(PackageName),.$$(patsubst .%,%,$$(PackageName)))

###############################################################################
# If package is not set, set it to Package
#   set it to $$(PackagePrefix)$$(PackageName)
# Otherwise, if Package starts with '.',
#   set it to $$(CurrentPackagePrefix)$$(Package)
ifeq ($$(Package),)
  export Package:=$$(PackagePrefix)$$(PackageName)
else
  ifneq ($$(filter .%,$$(Package)),)
    export Package:=$$(PackagePrefix)$$(Package)
  endif
endif

###############################################################################
# Check/Generate the package-id file
#
# Do nothing about packages if we are cleaning
ifeq ($$(filter clean%,$$(MAKECMDGOALS)),)
  ifneq ($$(LibIsInCwd)$$(LibIsInParent),)
    PackageFile:=$$(DirContainingLib)lib/package-id
    ifeq ($$(call OcpiExists,$$(DirContainingLib)lib),)
      $$(shell mkdir -p $$(DirContainingLib)lib)
    endif
    # If package-id file does not yet exist, create it based on Package
    ifeq ($$(call OcpiExists,$$(PackageFile)),)
      $$(shell echo $$(Package) > $$(PackageFile))
    else
      # If package-id file already exists, make sure its contents match Package
      PackageFromFile:=$$(shell cat $$(PackageFile))
      ifneq ($$(Package),$$(PackageFromFile))
        $$(error Package "$$(Package)" and "$$(PackageFromFile)" do not match. You must make clean after changing the package name.)
      endif
    endif
  endif
endif

###############################################################################

endef # define OcpiCreatePackageId

# Create the Package-ID for dir $1 and return it (the Package variable)
OcpiSetAndGetPackageId=$(eval $(call OcpiCreatePackageId,$1))$(Package)
