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

  constant DATA_WIDTH_c   : integer := to_integer(unsigned(DATA_WIDTH_p));

  signal idata_vld        : std_logic;
  signal i_odata          : signed(DATA_WIDTH_c-1 downto 0);
  signal q_odata          : signed(DATA_WIDTH_c-1 downto 0);
  signal odata_vld        : std_logic;
  signal peak_out         : std_logic_vector(15 downto 0);
  -- Temp signals to make older VHDL happy
  signal peak_rst_in       : std_logic;
  signal peak_a_in         : std_logic_vector(15 downto 0);
  signal peak_b_in         : std_logic_vector(15 downto 0);

begin

  peak_rst_in <= ctl_in.reset or std_logic(props_in.peak_read);
  peak_a_in   <= std_logic_vector(resize(i_odata,16));
  peak_b_in   <= std_logic_vector(resize(q_odata,16));

  -----------------------------------------------------------------------------
  -- 'enable' circuit (when up/downstream Workers ready and operating)
  -----------------------------------------------------------------------------

  idata_vld <= in_in.valid and out_in.ready;

  -----------------------------------------------------------------------------
  -- Take (when up/downstream Workers ready and operating)
  -----------------------------------------------------------------------------

  in_out.take <= idata_vld;

  -----------------------------------------------------------------------------
  -- Valid is always true since we only give valid data.
  -----------------------------------------------------------------------------

  out_out.valid <= odata_vld;

  -----------------------------------------------------------------------------
  -- DC offset cancellation filter
  -----------------------------------------------------------------------------

  i_dc_filter : dsp_prims.dsp_prims.dc_offset_cancellation
    generic map (
      DATA_WIDTH => DATA_WIDTH_c)
    port map (
      CLK       => ctl_in.clk,
      RST       => ctl_in.reset,
      BYPASS    => std_logic(props_in.bypass),
      UPDATE    => std_logic(props_in.update),
      TC        => signed(props_in.tc),
      DIN       => signed(in_in.data(DATA_WIDTH_c-1 downto 0)),
      DIN_VLD   => idata_vld,
      DOUT_RDY  => out_in.ready,
      DOUT      => i_odata,
      DOUT_VLD  => odata_vld);

  q_dc_filter : dsp_prims.dsp_prims.dc_offset_cancellation
    generic map (
      DATA_WIDTH => DATA_WIDTH_c)
    port map (
      CLK       => ctl_in.clk,
      RST       => ctl_in.reset,
      BYPASS    => std_logic(props_in.bypass),
      UPDATE    => std_logic(props_in.update),
      TC        => signed(props_in.tc),
      DIN       => signed(in_in.data(DATA_WIDTH_c-1+16 downto 16)),
      DIN_VLD   => idata_vld,
      DOUT_RDY  => out_in.ready,
      DOUT      => q_odata,
      DOUT_VLD  => open);

    out_out.data        <= std_logic_vector(resize(q_odata,16)) &
                           std_logic_vector(resize(i_odata,16));

  -----------------------------------------------------------------------------
  -- Peak Detection primitive. Value is cleared when read
  -----------------------------------------------------------------------------
  pm_gen : if its(PEAK_MONITOR_p) generate
    pd : util_prims.util_prims.peakDetect
      port map (
        CLK_IN   => ctl_in.clk,
        RST_IN   => peak_rst_in,
        EN_IN    => odata_vld,
        A_IN     => peak_a_in,
        B_IN     => peak_b_in,
        PEAK_OUT => peak_out);

    props_out.peak <= signed(peak_out);
  end generate pm_gen;

end rtl;
