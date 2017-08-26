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

if test -f /etc/redhat-release && test ! -L /etc/redhat-release; then
  read v0 v1 <<EOF
`sed < /etc/redhat-release 's/^\(.\).*release \([0-9]\+\).*/\1 \2/' | tr A-Z a-z`
EOF
  if test "$v0" = "r" -a "$v1" = "5"; then
    echo $1 r5 $2
    exit 0
  fi
  echo Cannot parse redhat release from /etc/redhat-release 1>&2
fi
exit 1
