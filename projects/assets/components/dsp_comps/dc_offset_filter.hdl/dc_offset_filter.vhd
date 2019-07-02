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
-- DC Offset Cancellation Filter
-------------------------------------------------------------------------------
--
-- Description:
--
-- The DC Offset Cancellation Filter worker inputs complex signed samples and
-- removes the DC bias from both I and Q input rails using separate 1st-order
-- IIR filters. The response time of the filter is programmable, as is the
-- ability to bypass the filter and to update/hold the calculated DC value to
-- be removed. A generic controls insertion of a peak detection circuit.
--
-- The time constant input TC = 128 * α, which is limited to a signed eight-bit
-- number, helps to control the bit growth of the multiplier. Rearranging the
-- equation, α = TC/128, where a maximum value of +127 is allowed due to signed
-- multiplication. Larger values of TC give both a faster filter response and a
-- narrower frequency magnitude notch at zero Hertz. A typical value of TC = 121
-- (α = 0.95) is used.
--
-- The input should be attenuated by as much as one bit to avoid overflow on
-- the output; i.e. the input should not be driven more than half-scale to
-- avoid overflow.
--
-- The circuit may be bypassed by asserting the BYPASS input. The circuit has a
-- latency of one DIN_VLD clock cycle.
-------------------------------------------------------------------------------

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions

architecture rtl of worker is

  constant c_data_width : integer := to_integer(DATA_WIDTH_p);

  signal s_data_vld_i : std_logic;
  signal s_data_vld_o : std_logic;
  signal s_i_o        : signed(c_data_width-1 downto 0);
  signal s_q_o        : signed(c_data_width-1 downto 0);
  --
  signal s_peak_rst_i : std_logic;
  signal s_peak_a_i   : std_logic_vector(15 downto 0);
  signal s_peak_b_i   : std_logic_vector(15 downto 0);
  signal s_peak_o     : std_logic_vector(15 downto 0);

begin

  -- WSI Interface
  in_out.take <= s_data_vld_i;

  out_out.data <= std_logic_vector(resize(s_q_o, c_data_width)) &
                  std_logic_vector(resize(s_i_o, c_data_width));

  out_out.valid <= s_data_vld_o;

  -- Internal
  s_data_vld_i <= in_in.valid and out_in.ready;

  s_peak_rst_i <= ctl_in.reset or std_logic(props_in.peak_read);
  s_peak_a_i   <= std_logic_vector(resize(s_i_o, 16));
  s_peak_b_i   <= std_logic_vector(resize(s_q_o, 16));

  -----------------------------------------------------------------------------
  -- DC offset cancellation filter
  -----------------------------------------------------------------------------

  i_dc_filter : dsp_prims.dsp_prims.dc_offset_cancellation
    generic map (
      DATA_WIDTH => c_data_width)
    port map (
      CLK      => ctl_in.clk,
      RST      => ctl_in.reset,
      BYPASS   => std_logic(props_in.bypass),
      UPDATE   => std_logic(props_in.update),
      TC       => signed(props_in.tc),
      DIN      => signed(in_in.data(c_data_width-1 downto 0)),
      DIN_VLD  => s_data_vld_i,
      DOUT_RDY => out_in.ready,
      DOUT     => s_i_o,
      DOUT_VLD => s_data_vld_o);

  q_dc_filter : dsp_prims.dsp_prims.dc_offset_cancellation
    generic map (
      DATA_WIDTH => c_data_width)
    port map (
      CLK      => ctl_in.clk,
      RST      => ctl_in.reset,
      BYPASS   => std_logic(props_in.bypass),
      UPDATE   => std_logic(props_in.update),
      TC       => signed(props_in.tc),
      DIN      => signed(in_in.data(c_data_width-1+16 downto 16)),
      DIN_VLD  => s_data_vld_i,
      DOUT_RDY => out_in.ready,
      DOUT     => s_q_o,
      DOUT_VLD => open);

  -----------------------------------------------------------------------------
  -- Peak Detection primitive. Value is cleared when read
  -----------------------------------------------------------------------------
  pm_gen : if its(PEAK_MONITOR_p) generate
    pd : util_prims.util_prims.peakDetect
      port map (
        CLK_IN   => ctl_in.clk,
        RST_IN   => s_peak_rst_i,
        EN_IN    => s_data_vld_o,
        A_IN     => s_peak_a_i,
        B_IN     => s_peak_b_i,
        PEAK_OUT => s_peak_o);

    props_out.peak <= signed(s_peak_o);
  end generate pm_gen;

  no_pm_gen : if its(not PEAK_MONITOR_p) generate
    props_out.peak <= (others => '0');
  end generate no_pm_gen;
end rtl;
