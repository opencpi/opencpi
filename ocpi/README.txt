
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

This directory represents an installation of OpenCPI, and is the
default target of various "make install" actions.

In a real installation, without all the sources, etc.  This is what
would be installed somewhere like /opt or /usr/local etc.

The bin subdirectory is for executables, with subdirectories for each
executable platform.  It would normally be put in your PATH.  It is
currently a link into the export of the tools/sdk package since that
is the only place binaries come from that are part of the
installation.

The lib subdirectory is for shared libraries to support the
executables (when they are not statically built).  It would normally
be put in your LD_LIBRARY_PATH (or on MacOSX DYLD_LIBRARY_PATH).  The
lib subdirectory has subdirectories for each executable platform.

There is a subdirectory under "lib" for each model, where libraries
are placed build for that model.


