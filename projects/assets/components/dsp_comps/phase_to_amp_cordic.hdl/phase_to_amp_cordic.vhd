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
-- Phase to Amplitude CORDIC
-------------------------------------------------------------------------------
--
-- Description:
--
-- The Phase to Amplitude CORDIC worker inputs a signed magnitude and
-- a phase increment value.  The phase increment is accumulated then passed to
-- a CORDIC to produce a complex sine/cosine waveform with the configured magnitude.
-- The frequency of the complex waveform can be calculated as follows:
--
-- output_freq = input_data/2^DATA_WIDTH
--
--      input_data = constant value will produce constant frequency
--      DATA_WIDTH = data width of the input to the CORDIC
-------------------------------------------------------------------------------

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions

architecture rtl of worker is

  constant c_data_width       : positive := to_integer(DATA_WIDTH);
  constant c_data_ext         : positive := to_integer(DATA_EXT);
  constant c_stages           : positive := to_integer(STAGES);
  --
  -- Breakdown of CORDIC primitive(s) delays:
  --  cordic_pr.vhd : 1
  --  cordic_pr.vhd/cordic.vhd/cordic_stage.vhd : c_stages (build property parameter)
  --  cordic_pr.vhd/round_conv.vhd : 1
  constant c_primitive_delays : positive := c_stages+2;

  signal s_transient_cnt   : unsigned(integer(ceil(log2(real(c_primitive_delays))))-1 downto 0);
  signal s_transient_done  : std_logic;
  signal s_data_vld_i      : std_logic;
  signal s_data_phase      : std_logic_vector(c_data_width-1 downto 0);
  signal s_data_vld_o      : std_logic;
  signal s_i_o             : std_logic_vector(c_data_width-1 downto 0);
  signal s_q_o             : std_logic_vector(c_data_width-1 downto 0);
  --
  signal s_peak_rst_i      : std_logic;
  signal s_peak_a_i        : std_logic_vector(15 downto 0);
  signal s_peak_b_i        : std_logic_vector(15 downto 0);
  signal s_peak_o          : std_logic_vector(15 downto 0);

begin

  -- WSI Interface
  in_out.take <= s_data_vld_i;

  out_out.data <= std_logic_vector(resize(signed(s_q_o), c_data_width)) & -- default output
                  std_logic_vector(resize(signed(s_i_o), c_data_width))
                   when (props_in.enable = '1') else
                  std_logic_vector(props_in.magnitude(c_data_width-1 downto 0)) & in_in.data;  --BYPASS (ENABLE=0)

  out_out.valid <= s_data_vld_o;

  -- Internal
  s_data_vld_i <= in_in.valid and out_in.ready;

  s_data_vld_o <= s_data_vld_i and s_transient_done;

  s_peak_rst_i <= ctl_in.reset or std_logic(props_in.peak_read);
  s_peak_a_i   <= std_logic_vector(resize(signed(s_i_o), 16));
  s_peak_b_i   <= std_logic_vector(resize(signed(s_q_o), 16));

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
  -- Magnitude is set by property, but Phase Increment is accumulated
  -----------------------------------------------------------------------------

  proc_InputPhaseAccum : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
        s_data_phase <= (others => '0');
      elsif (s_data_vld_i = '1') then
        s_data_phase <= std_logic_vector(signed(s_data_phase) + signed(in_in.data));
      end if;
    end if;
  end process;

  -----------------------------------------------------------------------------
  -- Polar-to-Rectangular CORDIC
  -----------------------------------------------------------------------------

  inst_cord : dsp_prims.dsp_prims.cordic_pr
    generic map (
      DATA_WIDTH => c_data_width,
      DATA_EXT   => c_data_ext,
      STAGES     => c_stages
      )
    port map (
      CLK     => ctl_in.clk,
      RST     => ctl_in.reset,
      MAG     => std_logic_vector(props_in.magnitude(c_data_width-1 downto 0)),
      PHASE   => s_data_phase,
      VLD_IN  => s_data_vld_i,
      I       => s_i_o,
      Q       => s_q_o,
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
