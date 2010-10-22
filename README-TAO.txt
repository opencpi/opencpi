
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

We currently only use TAO for the ocpisca tool, and we may eliminate
this need (and use OMNI instead).  To avoid a system-level
installation requirement (i.e. a "make install" in TAO), we have set things up to
assume that there is a ACE_wrappers directory at the same level as the top
level opencpi directory, and that the ACE/TAO build directories are subdirectories
of that directories with the standard opencpi subdir names (linux-x86_64-bin, darwin-i386-bin etc.).

Since we don't do a full "make install", we reference the includes and libraries
in these ACE trees.  Includes generally reference the source tree (but codegenerated includes reference the built tree), and libraries reference the built tree.

As far as execution goes, to set up the dynamlic library paths go:

On a mac do, here in this directory, in csh:

  source darwin-ldpath.csh

On linux do, here in this directory, in bash:

  . linux-ldpath.sh

These point both to the ocpi lib directory (./lib/<target>) as well as the TAO library directories necessary to run ocpisca.


