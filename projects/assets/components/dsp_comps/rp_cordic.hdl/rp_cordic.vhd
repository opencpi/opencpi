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
-- Rectangular-to-Polar CORDIC
-------------------------------------------------------------------------------
--
-- Description:
--
-- The Rectangular-to-Polar CORDIC worker inputs complex signed samples and
-- converts them to real signed phase output samples. The magnitude is output
-- as a self-clearing readable property. The build-time parameter STAGES_c is
-- used to control the precision of the output at the expense of more logic
-- resources.
--
-- Assuming that the input data valid is held asserted, the CORDIC circuit will
-- not output (valid) samples until STAGES_c + 6 clocks (input valids) later. The
-- output and internal state of the CORDIC is immediately held when the input
-- data valid is deasserted. Two additional clock cycle delays are implemented
-- by the demod process, making the net delay through the worker = STAGES + 8.
--
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
  --  cordic_rp.vhd : 5
  --  cordic_rp.vhd/cordic.vhd/cordic_stage.vhd : c_stages (build property parameter)
  --  cordic_rp.vhd/round_conv.vhd : 1
  constant c_primitive_delays : positive := c_stages+6;

  signal s_transient_cnt   : unsigned(integer(ceil(log2(real(c_primitive_delays))))-1 downto 0);
  signal s_transient_done  : std_logic;
  signal s1_transient_done : std_logic;
  signal s2_transient_done : std_logic;
  signal s_data_vld_i      : std_logic;
  signal s_data_vld_o      : std_logic;
  signal s_data_o          : std_logic_vector(c_data_width-1 downto 0);
  signal s_magnitude_o     : std_logic_vector(c_data_width-1 downto 0);
  signal s1_magnitude_o    : std_logic_vector(c_data_width-1 downto 0);
  signal s_phase_o         : std_logic_vector(c_data_width-1 downto 0);
  signal s1_phase_o        : std_logic_vector(c_data_width-1 downto 0);
  --
  signal s_peak_rst_i      : std_logic;
  signal s_peak_a_i        : std_logic_vector(15 downto 0);
  signal s_peak_o          : std_logic_vector(15 downto 0);

begin

  -- WCI Properties
  props_out.magnitude <= short_t(resize(signed(s1_magnitude_o), 16));

  -- WSI Interface
  in_out.take <= s_data_vld_i;

  out_out.data <= std_logic_vector(resize(signed(s_data_o), c_data_width));

  out_out.valid <= s_data_vld_o;

  -- Internal
  s_data_vld_i <= in_in.valid and out_in.ready;

  s_data_vld_o <= s_data_vld_i and s2_transient_done;

  s_peak_rst_i <= ctl_in.reset or std_logic(props_in.peak_read);
  s_peak_a_i   <= std_logic_vector(resize(signed(s_data_o), 16));

  -----------------------------------------------------------------------------
  -- Flag to indicate when Start-Up Transient data has been pushed output of
  -- the CORIDC primitive and that the first 'valid' data is available on the
  -- output of the CORIDC primitive.
  -----------------------------------------------------------------------------
  Flag_StartUpTransientDone : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
        s_transient_cnt   <= (others => '0');
        s_transient_done  <= '0';
        s1_transient_done <= '0';
        s2_transient_done <= '0';
      elsif (s_data_vld_i = '1') then
        s1_transient_done <= s_transient_done;  -- align with FM Discriminator
        s2_transient_done <= s1_transient_done;
        if (s_transient_cnt = c_primitive_delays-1) then
          s_transient_done <= '1';
        else
          s_transient_cnt <= s_transient_cnt + 1;
        end if;
      end if;
    end if;
  end process Flag_StartUpTransientDone;

  -----------------------------------------------------------------------------
  -- Rectangular-to-Polar CORDIC
  -----------------------------------------------------------------------------

  cord : dsp_prims.dsp_prims.cordic_rp
    generic map (
      DATA_WIDTH => c_data_width,
      DATA_EXT   => c_data_ext,
      STAGES     => c_stages
      )
    port map (
      CLK     => ctl_in.clk,
      RST     => ctl_in.reset,
      I       => in_in.data(c_data_width-1 downto 0),
      Q       => in_in.data(c_data_width-1+16 downto 16),
      VLD_IN  => s_data_vld_i,
      MAG     => s_magnitude_o,
      PHASE   => s_phase_o,
      VLD_OUT => open
      );

  demod : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      s1_magnitude_o <= s_magnitude_o;

      if (ctl_in.reset = '1') then
        s_data_o <= (others => '0');
      elsif (s_data_vld_i = '1' and s_transient_done = '1') then
        s_data_o <= std_logic_vector(signed(s_phase_o) - signed(s1_phase_o));
      end if;

      if (s_data_vld_i = '1') then
        s1_phase_o <= s_phase_o;
      end if;
    end if;
  end process demod;

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
        B_IN     => (others => '0'),
        PEAK_OUT => s_peak_o);

    props_out.peak <= signed(s_peak_o);
  end generate pm_gen;

  no_pm_gen : if its(not PEAK_MONITOR) generate
    props_out.peak <= (others => '0');
  end generate no_pm_gen;
end rtl;
