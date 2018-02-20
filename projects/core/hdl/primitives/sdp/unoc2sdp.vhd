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

library ieee, ocpi, platform;
use IEEE.std_logic_1164.all, IEEE.numeric_std.all,
  ocpi.all, ocpi.types.all, ocpi.util.all,
  platform.all, work.sdp.all;
entity unoc2sdp is
  generic(ocpi_debug   :     bool_t;
          sdp_width    :     natural);
  port (  unoc_in      : in  platform_pkg.unoc_master_out_t;
          unoc_out     : out platform_pkg.unoc_master_in_t;
          sdp_in       : in s2m_t;
          sdp_out      : out m2s_t;
          sdp_in_data  : in dword_array_t(0 to sdp_width-1);
          sdp_out_data : out dword_array_t(0 to sdp_width-1));
end entity;
architecture rtl of unoc2sdp is
begin
end rtl;
