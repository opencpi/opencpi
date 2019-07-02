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

--Decreases the sample rate of x by keeping the first sample and then every Rth sample after the first.
library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions

architecture rtl of worker is

  signal s_dec_cnt   : ushort_t;
  signal s_idata_vld : std_logic;
  signal s_odata_vld : std_logic;

begin

  -- Enable input to primitive (idata_vld) and take from input (in_out.take)
  -- when data is present/valid and output is allowed
  s_idata_vld <= in_in.valid and out_in.ready;
  in_out.take <= s_idata_vld;

  proc_dec_cnt : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        s_dec_cnt <= (others => '0');
      elsif its(s_idata_vld) then
        if s_dec_cnt = 0 then
          s_dec_cnt <= props_in.R-1;
        else
          s_dec_cnt <= s_dec_cnt-1;
        end if;
      end if;
    end if;
  end process;

  s_odata_vld <= to_bool(s_dec_cnt = 0) and s_idata_vld;

  -- Give to output port when output of primitive is valid
  out_out.valid <= s_odata_vld;
  out_out.data  <= in_in.data;

end rtl;
