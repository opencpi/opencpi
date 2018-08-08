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

# This is included when any platforms/configurations/containers/bitstreams are built for this platform

# Define the exact part for this platform
HdlPart_matchstiq_z1=xc7z020-1-clg484
# Which other component libraries does this platform need other than the core/global "devices"
# and this platform's own "devices" subdirectory?
#ComponentLibraries_matchstiq_z1=../../devices

# Does this map to an RCC platform/compiler?
HdlRccPlatform_matchstiq_z1=xilinx13_3
