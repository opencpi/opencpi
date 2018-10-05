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

cd odata/off_off && OCPI_LIBRARY_PATH=../../../../assemblies/ ocpirun -d -t 20 -pfile_read_0=fileName=../../idata/off.bin -pfile_read_1=fileName=../../idata/off.bin ../../app_event_in_x2_to_txen_tester_01.xml && cd -
cd odata/off_on  && OCPI_LIBRARY_PATH=../../../../assemblies/ ocpirun -d -t 20 -pfile_read_0=fileName=../../idata/off.bin -pfile_read_1=fileName=../../idata/on.bin ../../app_event_in_x2_to_txen_tester_01.xml && cd -
cd odata/on_off  && OCPI_LIBRARY_PATH=../../../../assemblies/ ocpirun -d -t 20 -pfile_read_0=fileName=../../idata/on.bin  -pfile_read_1=fileName=../../idata/off.bin ../../app_event_in_x2_to_txen_tester_01.xml && cd -
cd odata/on_on   && OCPI_LIBRARY_PATH=../../../../assemblies/ ocpirun -d -t 20 -pfile_read_0=fileName=../../idata/on.bin  -pfile_read_1=fileName=../../idata/on.bin ../../app_event_in_x2_to_txen_tester_01.xml && cd -
cd odata/onx10_off && OCPI_LIBRARY_PATH=../../../../assemblies/ ocpirun -d -t 20 -pfile_read_0=fileName=../../idata/onx10_off.bin  -pfile_read_1=fileName=../../idata/onx10_off.bin ../../app_event_in_x2_to_txen_tester_01.xml && cd -
