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

if test "$#" -ne 1; then
    echo "Correct usage is <name of hdl platform>"
    exit 1
fi

export OCPI_LIBRARY_PATH=../cust_test00
./generate.py
ocpidev -d ../cust_test00 build --hdl-assembly cust_test00 --hdl-platform $1
ocpirun --dump-file "cust_test00.capture_v2.hdl.props" -v  cust_test_app.xml
./verify.py cust_test00.capture_v2.hdl.props
