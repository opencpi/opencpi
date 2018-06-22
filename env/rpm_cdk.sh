#!/bin/sh
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

##########################################################################################
# This script is intended to be dropped in to the /etc/profile.d directory so that after
# installation all users get their environment initialized the same, with no action on
# their part.
# Note that these /etc/profile.d scripts actually get called from /etc/profile for login
# shells AND get called from /etc/bashrc for interactive shells.  This is to allow
# these drop-ins to do either type of initialization (environment in .profile or
# aliases and functions in .bashrc).
# Since we are only doing the former, we do not run if we are being called later
# in interactive shells.
shopt -q login_shell || return 0
source /opt/opencpi/cdk/opencpi-setup.sh -
# Import any user configuration files
for i in /opt/opencpi/cdk/env.d/*.sh ; do
  if [ -r "$i" ]; then
    . "$i"
  fi
done
