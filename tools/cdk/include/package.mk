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

# Do nothing about packages if we are cleaning
ifeq ($(filter clean%,$(MAKECMDGOALS)),)
  ifndef Package
    $(info The "Package" variable is not set. Assuming Package=local.)
    Package:=local
  endif
  PackageFile:=lib/package-name
  ifeq ($(call OcpiExists,lib),)
    $(shell mkdir -p lib)
  endif
  ifeq ($(call OcpiExists,$(PackageFile)),)
    $(shell echo $(Package) > $(PackageFile))
  else
    PackageName:=$(shell cat $(PackageFile))
    ifneq ($(Package),$(PackageName))
      $(error You must make clean after changing the package name.)
    endif
  endif
endif
