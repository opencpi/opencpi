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
-- Polar-to-Rectangular CORDIC
-------------------------------------------------------------------------------
--
-- Description:
--
-- The Polar-to-Rectangular CORDIC worker inputs signed magnitude and phase
-- increment values. The phase increment is accumulated external to this worker
-- (such as within the input test data file). The CORDIC is used to produce a
-- complex sine/cosine waveform with the configured magnitude. The build-time
-- parameter STAGES_c is used to control the precision of the output at the
-- expense of more logic resources. The frequency of the complex waveform can be
-- calculated as follows:
--
-- output_freq = input_data/2^DATA_WIDTH
--
--      input_data = constant value will produce constant frequency
--      DATA_WIDTH = data width of the input to the CORDIC
--
-- Assuming that the input data valid is held asserted, the circuit will not
-- output (valid) samples until STAGES_c + 1 clocks (input valids) later. The
-- output and internal state of the cordic is immediately held when the input
-- data valid is deasserted.
-------------------------------------------------------------------------------

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions

architecture rtl of worker is

  -- Normalize Property Parameters
  constant c_data_width       : positive := to_integer(DATA_WIDTH);
  constant c_data_ext         : positive := to_integer(DATA_EXT);
  constant c_stages           : positive := to_integer(STAGES);
  --
  -- Breakdown of CORDIC primitive(s) delays:
  --  cordic_pr.vhd : 1
  --  cordic_pr.vhd/cordic.vhd/cordic_stage.vhd : c_stages (build property parameter)
  --  cordic_pr.vhd/round_conv.vhd : 1
  constant c_primitive_delays : positive := c_stages+2;

  signal s_transient_cnt  : unsigned(integer(ceil(log2(real(c_primitive_delays))))-1 downto 0);
  signal s_transient_done : std_logic;
  signal s_data_vld_i     : std_logic;
  signal s_data_vld_o     : std_logic;
  signal s_magnitude      : std_logic_vector(c_data_width-1 downto 0);
  signal s_phase          : std_logic_vector(c_data_width-1 downto 0);
  signal s_real_o         : std_logic_vector(c_data_width-1 downto 0);
  signal s_imag_o         : std_logic_vector(c_data_width-1 downto 0);
  --
  signal s_peak_rst_i     : std_logic;
  signal s_peak_a_i       : std_logic_vector(15 downto 0);
  signal s_peak_b_i       : std_logic_vector(15 downto 0);
  signal s_peak_o         : std_logic_vector(15 downto 0);

begin

  -- WSI Interface
  in_out.take <= s_data_vld_i;

  out_out.data(31 downto 16) <= std_logic_vector(resize(signed(s_imag_o), 16));
  out_out.data(15 downto 0)  <= std_logic_vector(resize(signed(s_real_o), 16));

  out_out.valid <= s_data_vld_o;

  -- Internal
  s_data_vld_i <= in_in.valid and out_in.ready;

  s_data_vld_o <= s_data_vld_i and s_transient_done;

  s_magnitude <= in_in.data(c_data_width-1+16 downto 16);
  s_phase     <= in_in.data(c_data_width-1 downto 0);

  s_peak_rst_i <= ctl_in.reset or std_logic(props_in.peak_read);
  s_peak_a_i   <= std_logic_vector(resize(signed(s_real_o), 16));
  s_peak_b_i   <= std_logic_vector(resize(signed(s_imag_o), 16));

  -----------------------------------------------------------------------------
  -- Flag to indicate when Start-Up Transient data has been pushed output of
  -- the CORIDC primitive and that the first 'valid' data is available on the
  -- output of the CORIDC primitive.
  -----------------------------------------------------------------------------
  Flag_StartUpTransientDone : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
        s_transient_cnt  <= (others => '0');
        s_transient_done <= '0';
      elsif (s_data_vld_i = '1') then
        if (s_transient_cnt = c_primitive_delays-1) then
          s_transient_done <= '1';
        else
          s_transient_cnt <= s_transient_cnt + 1;
        end if;
      end if;
    end if;
  end process Flag_StartUpTransientDone;

  -----------------------------------------------------------------------------
  -- Polar-to-Rectangular CORDIC
  -----------------------------------------------------------------------------
  cord : dsp_prims.dsp_prims.cordic_pr
    generic map (
      DATA_WIDTH => c_data_width,
      DATA_EXT   => c_data_ext,
      STAGES     => c_stages
      )
    port map (
      CLK     => ctl_in.clk,
      RST     => ctl_in.reset,
      MAG     => s_magnitude,
      PHASE   => s_phase,
      VLD_IN  => s_data_vld_i,
      I       => s_real_o,
      Q       => s_imag_o,
      VLD_OUT => open
      );

  -----------------------------------------------------------------------------
  -- Peak Detection primitive. Value is cleared when read
  -----------------------------------------------------------------------------

  pm_gen : if its(PEAK_MONITOR) generate
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

  no_pm_gen : if its(not PEAK_MONITOR) generate
    props_out.peak <= (others => '0');
  end generate no_pm_gen;
end rtl;
