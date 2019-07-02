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

[ -z "$1" ] && echo "Do not run this command manually!" && exit 1

make clean
[ "$?" != "0" ] && echo "FAILED: make clean had non-zero exit status" && exit 1

make
[ "$?" != "0" ] && echo "FAILED: make had non-zero exit status" && exit 1

check_golden() {
  echo -n "Testing $1... "
  diff $2.inc golden/$2
  [ "$?" != "0" ] && echo "FAILED: $2" && exit 1
  echo "PASSED"
}

check_golden "Component Spec Properties" component_spec_properties

check_golden "Component Ports" component_ports

check_golden "Worker Properties" worker_properties

check_golden "Worker Interfaces" worker_interfaces

echo "OUTPUT FILE: $1.pdf"
# Not sure how rubber would pass but still have a PDF out but why not?
[ ! -f $1.pdf ] && echo "FAILED: $1.pdf missing" && exit 1

echo "PASSED"
