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

puts "Generating wrapper verilog file for zynq_ultra_ps_e version: 3.2.1"
#TODO: need to get the part programatically
create_project managed_ip_project managed_ip_project -part xczu28dr-ffvg1517-2-e -ip -force

create_ip -name zynq_ultra_ps_e -vendor xilinx.com -library ip -version 3.2 -module_name zynq_ultra_ps_e_0

generate_target all [get_files managed_ip_project/managed_ip_project.srcs/sources_1/ip/zynq_ultra_ps_e_0/zynq_ultra_ps_e_0.xci]
