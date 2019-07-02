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
-- FIR Real SSE Filter
-------------------------------------------------------------------------------
--
-- Description:
--
-- The FIR Real SSE (Systolic Symmetric Even) worker inputs real signed
-- samples and filters them based upon a programmable number of coefficient tap
-- values. The underlying FIR Filter implementation makes use of a symmetric
-- systolic structure to construct a filter with an even number of taps and
-- symmetry about its midpoint. Thus the NUM_TAPS_p parameter defines N/2
-- coefficient values. Care should be taken to make sure that the COEFF_WIDTH_p
-- parameter is <= the type (size) of the taps property.
--
-- This implementation uses NUM_TAPS_p multipliers to process input data at the
-- clock rate - i.e. this worker can handle a new input value every clock cycle.
-- It is unnecessary to round the output data from this filter.
--
-- This filter will produce valid outputs one clock after each valid
-- input, but care must be exercised when attempting to align outputs
-- according to the filter's actual group delay and propagation delay.
--
-- For an FIR filter with symmetric impulse response we are guaranteed
-- to have linear phase response and thus constant group delay vs
-- frequency. In general the group delay will be equal to (N-1)/2 where
-- N is the number of filter taps.
--
-- The filter topology itself will add some propagation delay to the
-- response. For this design the total delay from an impulse input to the
-- beginning of the impulse response will be NUM_TAPS_p + 4 samples
-------------------------------------------------------------------------------

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions

architecture rtl of worker is

  constant c_NUM_TAPS         : integer := to_integer(NUM_TAPS_p);
  constant c_DATA_WIDTH       : integer := to_integer(DATA_WIDTH_p);
  constant c_COEFF_WIDTH      : integer := to_integer(COEFF_WIDTH_p);

  signal s_fir_wdata          : std_logic_vector(c_COEFF_WIDTH-1 downto 0);
  signal s_o                  : std_logic_vector(c_DATA_WIDTH-1 downto 0);
  signal s_data_vld_o         : std_logic;
  signal s_missed_data_vld_o  : std_logic := '0';
  signal s_peak_o             : std_logic_vector(15 downto 0);
  signal s_data_vld_i         : std_logic;
  -- temp signals for old VHDL
  signal s_raw_byte_enable    : std_logic_vector(3 downto 0);
  signal s_raw_addr           : std_logic_vector(15 downto 0);
  signal s_peak_rst_i         : std_logic;
  signal s_peak_a_i           : std_logic_vector(15 downto 0);
begin

  s_raw_byte_enable     <= props_in.raw.byte_enable;
  s_raw_addr            <= '0' & std_logic_vector(props_in.raw.address(15 downto 1));
  s_peak_rst_i         <= ctl_in.reset or std_logic(props_in.peak_read);
  s_peak_a_i           <= std_logic_vector(resize(signed(s_o),16));
  props_out.raw.done  <= '1';
  props_out.raw.error <= '0';


  s_data_vld_i <= out_in.ready and in_in.valid;

  in_out.take <= s_data_vld_i;

  out_out.valid <= out_in.ready and (s_data_vld_o or s_missed_data_vld_o);

  -----------------------------------------------------------------------------
  -- handle coefficient resizing and byte enables
  -----------------------------------------------------------------------------
  be_16_gen : if c_COEFF_WIDTH <= 16 generate
    --Input byte enable decode
    s_fir_wdata <= props_in.raw.data(c_COEFF_WIDTH-1 downto 0) when (s_raw_byte_enable = "0011") else
                   props_in.raw.data(c_COEFF_WIDTH-1+16 downto 16) when (s_raw_byte_enable = "1100") else (others => '0');
  end generate be_16_gen;

  -----------------------------------------------------------------------------
  -- FIR Systolic Symmetrical Even primitive
  -----------------------------------------------------------------------------

  fir : dsp_prims.dsp_prims.fir_systolic_sym_even
    generic map (
      NUM_SECTIONS  => c_NUM_TAPS,
      DATA_WIDTH    => c_DATA_WIDTH,
      COEFF_WIDTH   => c_COEFF_WIDTH,
      ACC_PREC      => c_DATA_WIDTH + c_COEFF_WIDTH + 1 + integer(ceil(log2(real(c_NUM_TAPS)))),
      DATA_ADDR     => (others => '0'),
      PTR_ADDR      => (others => '0'),
      USE_COEFF_PTR => false)
    port map (
      CLK      => ctl_in.clk,
      RST      => ctl_in.reset,
      DIN      => in_in.data(c_DATA_WIDTH-1 downto 0),
      DIN_VLD  => s_data_vld_i,
      DOUT     => s_o,
      DOUT_VLD => s_data_vld_o,
      ADDR     => s_raw_addr,
      RDEN     => props_in.raw.is_read,
      WREN     => props_in.raw.is_write,
      RDATA    => open,
      WDATA    => s_fir_wdata);

  backPressure : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if(ctl_in.reset = '1' or out_in.ready = '1') then
        s_missed_data_vld_o <= '0';
      elsif (out_in.ready = '0' and s_data_vld_o = '1') then
        s_missed_data_vld_o <= '1';
      end if;
    end if;
  end process backPressure;

  out_out.data <= std_logic_vector(resize(signed(s_o),16));

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
