-- This file is protected by Copyright. Please refer to the COPYRIGHT file
-- distributed with this source distribution.
--
-- This file is part of OpenCPI <http://www.opencpi.org>
--
-- OpenCPI is free software: you can redistribute it and/or modify it under the
-- terms of the GNU Lesser General Public License as published by the Free
-- Software Foundation, either version 3 of the License, or (at your option) any
-- later version.
--
-- OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
-- WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
-- A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
-- details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program. If not, see <http://www.gnu.org/licenses/>.

library ieee; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
library axi; use axi.axi_pkg.all;
library work; use work.sdp.all;
package sdp_axi is

component sdp2axi is
  generic(
    ocpi_debug : boolean;
    sdp_width  : natural;
    axi_width  : natural);
  port(
    clk          : in  std_logic;
    reset        : in  bool_t;
    sdp_in       : in  s2m_t;
    sdp_in_data  : in  dword_array_t(0 to sdp_width-1);
    sdp_out      : out m2s_t;
    sdp_out_data : out dword_array_t(0 to sdp_width-1);
    axi_in       : in  s_axi_hp_out_t;
    axi_out      : out s_axi_hp_in_t;
    axi_error    : out bool_t;
    dbg_state    : out ulonglong_t;
    dbg_state1   : out ulonglong_t;
    dbg_state2   : out ulonglong_t
    );
end component sdp2axi;

end package sdp_axi;
