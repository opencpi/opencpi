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
-- IQ Imbalance Fixer
-------------------------------------------------------------------------------
--
-- Description:
--
-- The IQ Imbalance Fixer worker inputs complex signed samples and corrects for
-- power differences in I and Q input rails and also provides perfect 90 degree
-- quadrature phase alignment between input rails. The response time of the
-- module is programmable, as is the loop gain, and the ability to bypass the
-- module and to update/hold the calculated gain and phase offsets to be applied
-- to the Q rail. A generic controls insertion of a peak detection circuit.
--
-- Input values are presented as corrected output values following four
-- 'enable' cycles. This circuit operates at the full clock rate - that is,
-- 'enable' may be held asserted every clock cycle.
-------------------------------------------------------------------------------

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions

architecture rtl of worker is

  constant c_data_width : integer := to_integer(DATA_WIDTH_p);
  constant c_acc_prec   : integer := to_integer(ACC_PREC_p);

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
  -- IQ Imbalance Correction component
  -----------------------------------------------------------------------------

  iq_imbalance : dsp_prims.dsp_prims.iq_imbalance_corrector
    generic map (
      DATA_WIDTH => c_data_width,
      ACC_PREC   => c_acc_prec)
    port map (
      CLK             => ctl_in.clk,
      RST             => ctl_in.reset,
      ENABLE          => std_logic(props_in.enable),
      UPDATE          => std_logic(props_in.update),
      LOG2_AVG_LEN    => unsigned(props_in.log2_averaging_length(4 downto 0)),
      NLOG2_LOOP_GAIN => unsigned(props_in.neg_log2_loop_gain(4 downto 0)),
      DIN_I           => signed(in_in.data(c_data_width-1 downto 0)),
      DIN_Q           => signed(in_in.data(c_data_width-1+16 downto 16)),
      DIN_VLD         => s_data_vld_i,
      DOUT_I          => s_i_o,
      DOUT_Q          => s_q_o,
      DOUT_VLD        => s_data_vld_o,
      C_CORR          => open,
      D_CORR          => open);

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
