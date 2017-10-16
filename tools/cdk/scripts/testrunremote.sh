#!/bin/bash
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


# internal script to run tests on a remote system from a foo.test directory
# usage is: ./testrunremote.sh <host> <user> <passwd> <cmd> <ssh-protocol-version>

pwfile=$1.pw
inrun=$(basename "$(dirname $PWD)")
if [ "$inrun" != run ]; then
  mkdir -p gen
  pwfile=gen/$pwfile
fi
if [ "$5" ]; then
  sshversion="-"$5
fi
rm -f $pwfile
echo echo $3 > $pwfile
chmod a+x $pwfile
SSH_ASKPASS=./$pwfile $OCPI_CDK_DIR/scripts/setsid.py ssh $sshversion \
  -o "GSSAPIAuthentication no" -o "StrictHostKeyChecking no" -o "UserKnownHostsFile /dev/null" -o "ConnectTimeout=30" \
  $2@$1 sh -l -c \'"$4"\'
