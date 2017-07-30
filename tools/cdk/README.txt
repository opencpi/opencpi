This file is protected by Copyright. Please refer to the COPYRIGHT file
distributed with this source distribution.

This file is part of OpenCPI <http://www.opencpi.org>

OpenCPI is free software: you can redistribute it and/or modify it under the
terms of the GNU Lesser General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program. If not, see <http://www.gnu.org/licenses/>.

This cdk directory contains tools and makefile scripts that comprise
the cdk.

The "export" subdirectory is a directory of hand-create
simlinks, NOT SOURCE FILES, that can be tar'd to export the cdk.

The top level "include" directory contains make scripts, and other
constant files.  Tools are in their own subdirectories built normally,
starting with ocpigen.  The target directory names for the cdk are
based on the `uname -s`-`uname -m` formula, while the overall OCPI
build files use things like linux-bin and macos-bin.  The export link
tree converts one to the other.

Thus the export tree is expected to be copied into a real
"installation" tree that might include stuff build from other areas.




