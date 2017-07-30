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

puts "Generating wrapper verilog file for processing_system7 version: [lindex $argv 0]"
create_project managed_ip_project managed_ip_project -part xc7z020-clg484-1 -ip -force

create_ip -name processing_system7 -vendor xilinx.com -library ip -version [lindex $argv 0] -module_name processing_system7_0

generate_target all [get_files  managed_ip_project/managed_ip_project.srcs/sources_1/ip/processing_system7_0/processing_system7_0.xci]
