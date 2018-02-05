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

# Fedora 25 target environment

# #### Compiler/flags ############################################## #

# export OCPI_TARGET_CXXFLAGS="$OCPI_TARGET_CXXFLAGS -std=c++0x"

# #### Shared library build settings ###################################### #

export OCPI_SHARED_LIBRARIES_FLAGS="-m64"
# "-m elf_x86_64" is invalid on gcc included with CentOS 7

if test "$OCPI_BUILD_SHARED_LIBRARIES" = ""; then
  export OCPI_BUILD_SHARED_LIBRARIES=0
fi
