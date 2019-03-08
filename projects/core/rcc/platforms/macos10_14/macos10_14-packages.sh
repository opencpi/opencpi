#!/bin/sh --noprofile
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

if ! PORT=`command -v port`; then
  # Try this in case the macports tree is not in the path
  PORT=/opt/local/bin/port
  [ ! -x $PORT ] && {
    echo 'ERROR: This software platform, $OCPI_TOOL_PLATFORM, requires that the "macports" package be installed.'
    echo '       It is usually directly installable from: https://www.macports.org/install.php'
    echo '       The "port" command was not in the PATH or in the standard location, '$PORT
    exit 1
  }
fi
PKGS="python37 coreutils gsed py37-numpy swig swig-python py27-numpy"
echo Using $PORT to install packages required by OpenCPI for $OCPI_TOOL_PLATFORM: $PKGS
sudo $PORT install $PKGS
# FIXME: somehow automate the required path additions?
# They are /opt/local/bin and /opt/local/libexec/gnubin
# At the end of the path.
