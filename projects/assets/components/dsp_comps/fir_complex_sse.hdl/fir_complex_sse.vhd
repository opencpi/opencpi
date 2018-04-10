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
-- FIR Complex SSE Filter
-------------------------------------------------------------------------------
--
-- Description:
--
-- The FIR Complex SSE (Systolic Symmetric Even) worker inputs complex signed
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

architecture rtl of fir_complex_sse_worker is

  constant NUM_TAPS_c           : integer := to_integer(unsigned(NUM_TAPS_p));
  constant DATA_WIDTH_c         : integer := to_integer(unsigned(DATA_WIDTH_p));
  constant COEFF_WIDTH_c        : integer := to_integer(unsigned(COEFF_WIDTH_p));
  constant MAX_MESSAGE_VALUES_c : integer := 4096;  -- from iqstream_protocol

  signal fir_wdata         : std_logic_vector(COEFF_WIDTH_c-1 downto 0);
  signal fir_rdata         : std_logic_vector(COEFF_WIDTH_c-1 downto 0);
  signal i_odata           : std_logic_vector(DATA_WIDTH_c-1 downto 0);
  signal q_odata           : std_logic_vector(DATA_WIDTH_c-1 downto 0);
  signal odata_vld         : std_logic;
  signal missed_odata_vld  : std_logic := '0';
  signal peak_out          : std_logic_vector(15 downto 0);
  signal msg_cnt           : unsigned(integer(ceil(log2(real(MAX_MESSAGE_VALUES_c))))-1 downto 0);
  signal max_sample_cnt    : unsigned(integer(ceil(log2(real(MAX_MESSAGE_VALUES_c))))-1 downto 0);
  signal enable            : std_logic;
  signal take              : std_logic;
  signal force_som         : std_logic;
  signal force_eom         : std_logic;
  type state_t is (INIT_s, WAIT_s, SEND_s);
  signal current_state     : state_t;
  -- temp signals for old VHDL
  signal raw_byte_enable   : std_logic_vector(3 downto 0);
  signal raw_addr          : std_logic_vector(15 downto 0);
  signal peak_rst_in       : std_logic;
  signal peak_a_in         : std_logic_vector(15 downto 0);
  signal peak_b_in         : std_logic_vector(15 downto 0);

begin

  raw_byte_enable     <= props_in.raw.byte_enable;
  raw_addr            <= '0' & std_logic_vector(props_in.raw.address(15 downto 1));
  peak_rst_in         <= ctl_in.reset or std_logic(props_in.peak_read);
  peak_a_in           <= std_logic_vector(resize(signed(i_odata),16));
  peak_b_in           <= std_logic_vector(resize(signed(q_odata),16));
  props_out.raw.done  <= '1';
  props_out.raw.error <= '0';

  -----------------------------------------------------------------------------
  -- 'enable' fir (when up/downstream Workers ready, input data valid)
  -----------------------------------------------------------------------------

  enable <= '1' when (out_in.ready = '1' and in_in.ready = '1' and in_in.valid = '1'
                and ctl_in.is_operating = '1') else '0';

  -----------------------------------------------------------------------------
  -- Take (when up/downstream Workers ready and input data valid)
  -----------------------------------------------------------------------------

  in_out.take <= '1' when (out_in.ready = '1' and in_in.ready = '1' and take = '1'
                     and ctl_in.is_operating = '1') else '0';

  -----------------------------------------------------------------------------
  -- Give (when downstream Worker ready & fir_complex_sse_worker.vhd output valid)
  -----------------------------------------------------------------------------

  out_out.give <= '1' when (out_in.ready = '1' and ctl_in.is_operating = '1'
                      and (odata_vld = '1' or missed_odata_vld = '1' or force_eom = '1')) else '0';

  -----------------------------------------------------------------------------
  -- Valid (when "Give")
  -----------------------------------------------------------------------------

  out_out.valid <= '1' when (out_in.ready = '1' and (odata_vld = '1' or missed_odata_vld = '1')) else '0';

  -----------------------------------------------------------------------------
  -- Zero-Length Message FSM
  -- the zlm_fsm is being depreciated, instead see dc_offset_filter.vhd
  -- for recommended mechanism for dealing with primitive latency
  -----------------------------------------------------------------------------  

  zlm_fsm : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if(ctl_in.reset = '1') then
        current_state <= INIT_s;
        take          <= '1';
        force_som     <= '0';
        force_eom     <= '0';
      else
        -- defaults
        current_state <= current_state;
        take          <= '1';
        force_som     <= '0';
        force_eom     <= '0';

        case current_state is
          when INIT_s =>
            if (in_in.som = '1' and in_in.eom = '1' and in_in.valid = '0') then
              current_state <= SEND_s;
            elsif (in_in.som = '1' and in_in.valid = '0') then
              current_state <= WAIT_s;
            end if;
          when WAIT_s =>
            if (in_in.valid = '1') then
              current_state <= INIT_s;
            elsif (in_in.eom = '1') then
              current_state <= SEND_s;
            end if;
          when SEND_s =>
            take <= '0';
            if (msg_cnt /= 1 and out_in.ready = '1') then
              force_eom     <= '1';
            elsif (out_in.ready = '1') then
              current_state <= INIT_s;
              force_som     <= '1';
              force_eom     <= '1';
            end if;
        end case;

      end if;
    end if;
  end process zlm_fsm;

  -----------------------------------------------------------------------------
  -- SOM/EOM - counter set to message size, increment while giving
  -----------------------------------------------------------------------------

  max_sample_cnt <= resize(props_in.messageSize srl 2, max_sample_cnt'length);

  messageSize_count : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if(ctl_in.reset = '1' or force_eom = '1') then
        msg_cnt   <= (0 => '1', others => '0');
      elsif (odata_vld = '1') then
        if(msg_cnt = max_sample_cnt) then
          msg_cnt <= (0 => '1', others => '0');
        else
          msg_cnt <= msg_cnt + 1;
        end if;
      end if;
    end if;
  end process messageSize_count;

  out_out.som <= '1' when ((out_in.ready = '1' and odata_vld = '1' and
                           msg_cnt = 1) or force_som = '1') else '0';
  out_out.eom <= '1' when ((out_in.ready = '1' and odata_vld = '1' and
                           msg_cnt = max_sample_cnt) or
                           force_eom = '1') else '0';

  -----------------------------------------------------------------------------
  -- handle coefficient resizing and byte enables
  -----------------------------------------------------------------------------
  be_32_gen : if COEFF_WIDTH_c <= 32 and COEFF_WIDTH_c > 16 generate
    --Input byte enable decode
    be_input : process (raw_byte_enable, props_in.raw.data)
    begin
      case raw_byte_enable is
        when "1111" => fir_wdata <= std_logic_vector(props_in.raw.data(COEFF_WIDTH_c-1 downto  0));
        when others => fir_wdata <= (others => '0');
      end case;
    end process be_input;

    --Output byte enable encoding
    be_output : process (raw_byte_enable, fir_rdata)
    begin
      case raw_byte_enable is
         when "1111" => props_out.raw.data <= std_logic_vector(resize(signed(fir_rdata),32));
         when others => props_out.raw.data <= (others => '0');
      end case;
    end process be_output;
  end generate be_32_gen;

  be_16_gen : if COEFF_WIDTH_c <= 16 and COEFF_WIDTH_c > 8 generate
    --Input byte enable decode
    be_input : process (raw_byte_enable, props_in.raw.data)
    begin
      case raw_byte_enable is
        when "0011" => fir_wdata <= std_logic_vector(props_in.raw.data(COEFF_WIDTH_c-1    downto  0));
        when "0110" => fir_wdata <= std_logic_vector(props_in.raw.data(COEFF_WIDTH_c-1+8  downto  8));
        when "1100" => fir_wdata <= std_logic_vector(props_in.raw.data(COEFF_WIDTH_c-1+16 downto 16));
        when others => fir_wdata <= (others => '0');
      end case;
    end process be_input;

    be_output : process (raw_byte_enable, fir_rdata)
    begin
      case raw_byte_enable is
        when "0011" => props_out.raw.data <= x"0000" & std_logic_vector(resize(signed(fir_rdata),16));
        when "0110" => props_out.raw.data <= x"00" & std_logic_vector(resize(signed(fir_rdata),16)) & x"00";
        when "1100" => props_out.raw.data <= std_logic_vector(resize(signed(fir_rdata),16)) & x"0000";
        when others => props_out.raw.data <= (others => '0');
      end case;
    end process be_output;
  end generate be_16_gen;

  be_8_gen : if COEFF_WIDTH_c <= 8 generate
    --Input byte enable decode
    be_input : process (raw_byte_enable, props_in.raw.data)
    begin
      case raw_byte_enable is
        when "0001" => fir_wdata <= std_logic_vector(props_in.raw.data(COEFF_WIDTH_c-1    downto  0));
        when "0010" => fir_wdata <= std_logic_vector(props_in.raw.data(COEFF_WIDTH_c-1+8  downto  8));
        when "0100" => fir_wdata <= std_logic_vector(props_in.raw.data(COEFF_WIDTH_c-1+16 downto 16));
        when "1000" => fir_wdata <= std_logic_vector(props_in.raw.data(COEFF_WIDTH_c-1+24 downto 24));
        when others => fir_wdata <= (others => '0');
      end case;
    end process be_input;

    --Output byte enable encoding
    be_output : process (raw_byte_enable, fir_rdata)
    begin
      case raw_byte_enable is
        when "0001" => props_out.raw.data <= x"000000" & std_logic_vector(resize(signed(fir_rdata),8));
        when "0010" => props_out.raw.data <= x"0000" & std_logic_vector(resize(signed(fir_rdata),8)) & x"00";
        when "0100" => props_out.raw.data <= x"00" & std_logic_vector(resize(signed(fir_rdata),8)) & x"0000";
        when "1000" => props_out.raw.data <= std_logic_vector(resize(signed(fir_rdata),8)) & x"000000";
        when others => props_out.raw.data <= (others => '0');
      end case;
    end process be_output;
  end generate be_8_gen;

  -----------------------------------------------------------------------------
  -- FIR Systolic Symmetrical Even primitive
  -----------------------------------------------------------------------------

  i_fir : dsp_prims.dsp_prims.fir_systolic_sym_even
    generic map (
      NUM_SECTIONS  => NUM_TAPS_c,
      DATA_WIDTH    => DATA_WIDTH_c,
      COEFF_WIDTH   => COEFF_WIDTH_c,
      ACC_PREC      => DATA_WIDTH_c + COEFF_WIDTH_c + 1 + integer(ceil(log2(real(NUM_TAPS_c)))),
      DATA_ADDR     => (others => '0'),
      PTR_ADDR      => (others => '0'),
      USE_COEFF_PTR => false)
    port map (
      CLK      => ctl_in.clk,
      RST      => ctl_in.reset,
      DIN      => in_in.data(DATA_WIDTH_c-1+16 downto 16),
      DIN_VLD  => enable,
      DOUT     => i_odata,
      DOUT_VLD => odata_vld,
      ADDR     => raw_addr,
      RDEN     => props_in.raw.is_read,
      WREN     => props_in.raw.is_write,
      RDATA    => fir_rdata,
      WDATA    => fir_wdata);

  q_fir : dsp_prims.dsp_prims.fir_systolic_sym_even
    generic map (
      NUM_SECTIONS  => NUM_TAPS_c,
      DATA_WIDTH    => DATA_WIDTH_c,
      COEFF_WIDTH   => COEFF_WIDTH_c,
      ACC_PREC      => DATA_WIDTH_c + COEFF_WIDTH_c + 1 + integer(ceil(log2(real(NUM_TAPS_c)))),
      DATA_ADDR     => (others => '0'),
      PTR_ADDR      => (others => '0'),
      USE_COEFF_PTR => false)
    port map (
      CLK      => ctl_in.clk,
      RST      => ctl_in.reset,
      DIN      => in_in.data(DATA_WIDTH_c-1 downto 0),
      DIN_VLD  => enable,
      DOUT     => q_odata,
      DOUT_VLD => open,
      ADDR     => raw_addr,
      RDEN     => '0',
      WREN     => props_in.raw.is_write,
      RDATA    => open,
      WDATA    => fir_wdata);

  backPressure : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if(ctl_in.reset = '1' or out_in.ready = '1') then
        missed_odata_vld <= '0';
      elsif (out_in.ready = '0' and odata_vld = '1') then
        missed_odata_vld <= '1';
      end if;
    end if;
  end process backPressure;

  out_out.data        <= std_logic_vector(resize(signed(i_odata),16)) &
                         std_logic_vector(resize(signed(q_odata),16));
  out_out.byte_enable <= (others => '1');

  -----------------------------------------------------------------------------
  -- Peak Detection primitive. Value is cleared when read
  -----------------------------------------------------------------------------

  pd : util_prims.util_prims.peakDetect
    port map (
      CLK_IN   => ctl_in.clk,
      RST_IN   => peak_rst_in,
      EN_IN    => odata_vld,
      A_IN     => peak_a_in,
      B_IN     => peak_b_in,
      PEAK_OUT => peak_out);

  props_out.peak <= signed(peak_out);

end rtl;
