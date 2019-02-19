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

-------------------------------------------------------------------------------
-- Automatic Gain Control Real
-------------------------------------------------------------------------------
--
-- Description:
--
-- The Automatic Gain Control (AGC) Real worker inputs real signed samples and
-- applies an AGC circuit to the input data in order to smooth amplitude
-- transitions. The response time of the AGC is programmable, as is the ability
-- to update/hold the current gain setting.
--
-- The REF property defines the desired output amplitude, while the MU property
-- defines the fixed feedback coefficient that is multiplied by the difference
-- in the feedback voltage and thus controls the response time of the circuit.
--
-- The circuit gain may be held constant by asserting the HOLD input. Build-time
-- parameters include the width of the data and the length of the averaging
-- window. The circuit has a latency of three DIN_VLD clock cycles.
-------------------------------------------------------------------------------

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions

architecture rtl of worker is

  constant DATA_WIDTH_c         : integer := to_integer(unsigned(DATA_WIDTH_p));
  constant AVG_WINDOW_c         : integer := to_integer(unsigned(AVG_WINDOW_p));

  signal odata            : std_logic_vector(DATA_WIDTH_c-1 downto 0);
  signal odata_vld        : std_logic;
  signal reg_wr_en        : std_logic;
  signal enable           : std_logic;

begin

  -----------------------------------------------------------------------------
  -- Note the primitive has no output valid.  If it did we could suppress
  -- output on startup, and drain the output properly on EOF.
  -- To maintain compatibility with legacy, we simply suppress the first output
  -----------------------------------------------------------------------------
  ovalid : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if(ctl_in.reset = '1') then
        odata_vld <= '0';
      elsif its(out_in.ready) then
        odata_vld <= in_in.valid;
      end if;
    end if;
  end process;
  -----------------------------------------------------------------------------
  -- 'enable' everything when there is valid data and output is ready
  -----------------------------------------------------------------------------

  enable <= out_in.ready and in_in.valid;

  -----------------------------------------------------------------------------
  -- Take (when up/downstream Workers ready and not sending a ZLM)
  -----------------------------------------------------------------------------

  in_out.take <= enable;

  -----------------------------------------------------------------------------
  -- Give (when output port is ready & primitive has valid output
  -----------------------------------------------------------------------------

  out_out.valid <= odata_vld and out_in.ready;

  -----------------------------------------------------------------------------
  -- AGC Primitive Instantation
  -----------------------------------------------------------------------------

  reg_wr_en <= props_in.ref_written or props_in.mu_written;

  prim_agc : util_prims.util_prims.agc
    generic map (
      DATA_WIDTH => DATA_WIDTH_c,
      NAVG       => AVG_WINDOW_c)
    port map (
      CLK     => ctl_in.clk,
      RST     => ctl_in.reset,
      REG_WR  => reg_wr_en,
      REF     => std_logic_vector(props_in.ref),
      MU      => std_logic_vector(props_in.mu),
      DIN_VLD => enable,
      HOLD    => props_in.hold,
      DIN     => in_in.data(DATA_WIDTH_c-1 downto 0),
      DOUT    => odata);

  out_out.data        <= std_logic_vector(resize(signed(odata),16));

end rtl;
