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
create_project managed_ip_project managed_ip_project -part xczu28dr-ffvg1517-2-e -ip -force;
#TODO: need to get the part programatically

set_property target_language VHDL [current_project];

create_ip -name zynq_ultra_ps_e -vendor xilinx.com -library ip -version 3.2 -module_name zynq_ultra_ps_e_0;

set_property -dict [list CONFIG.PSU__USE__M_AXI_GP0 {1} CONFIG.PSU__MAXIGP0__DATA_WIDTH {32} CONFIG.PSU__USE__M_AXI_GP1 {1} CONFIG.PSU__MAXIGP1__DATA_WIDTH {32} CONFIG.PSU__USE__M_AXI_GP2 {0} CONFIG.PSU__USE__S_AXI_GP2 {1} CONFIG.PSU__SAXIGP2__DATA_WIDTH {64} CONFIG.PSU__USE__S_AXI_GP3 {1} CONFIG.PSU__SAXIGP3__DATA_WIDTH {64} CONFIG.PSU__USE__S_AXI_GP4 {1} CONFIG.PSU__SAXIGP4__DATA_WIDTH {64} CONFIG.PSU__USE__S_AXI_GP5 {1} CONFIG.PSU__SAXIGP5__DATA_WIDTH {64} CONFIG.PSU__DDRC__ENABLE {0}] [get_ips zynq_ultra_ps_e_0];

generate_target all [get_files managed_ip_project/managed_ip_project.srcs/sources_1/ip/zynq_ultra_ps_e_0/zynq_ultra_ps_e_0.xci];
