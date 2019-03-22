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

# This Makefile fragment is for the "assets_ts" project

# Package identifier is used in a hierachical fashion from Project to Libraries....
# The PackageName, PackagePrefix and Package variables can optionally be set here:
# PackageName defaults to the name of the directory
# PackagePrefix defaults to local
# Package overrides the other variables and defaults to PackageName.PackagePrefix
PackageName=assets_ts
PackagePrefix=ocpi

ProjectDependencies=ocpi.assets

# These assignments support building from any directory.
ComponentLibraries+=components misc_comps util_comps dsp_comps comms_comps base_comps devices cards
# Note: the 'components' library is listed above to ensure the local library is checked before the
# libraries outside of the project. This distinction is important because specs/workers in this 
# project have the same names as specs/workers in the assets project. 
#example remote system: note there can be multiple remote systems, colon-separated.
#export OCPI_REMOTE_TEST_SYSTEMS:=10.0.1.16=root=root=/mnt/net/workspace/git/ocpi.assets
