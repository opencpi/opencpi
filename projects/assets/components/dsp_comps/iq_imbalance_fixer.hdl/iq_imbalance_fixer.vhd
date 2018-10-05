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

architecture rtl of iq_imbalance_fixer_worker is

  constant DATA_WIDTH_c         : integer := to_integer(unsigned(DATA_WIDTH_p));
  constant ACC_PREC_c           : integer := to_integer(unsigned(ACC_PREC_p));
  constant MAX_MESSAGE_VALUES_c : integer := 4096;  -- from iqstream_protocol

  signal i_odata           : signed(DATA_WIDTH_c-1 downto 0);
  signal q_odata           : signed(DATA_WIDTH_c-1 downto 0);
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
  -- Temp signals to make older VHDL happy
  signal peak_rst_in       : std_logic;
  signal peak_a_in         : std_logic_vector(16-1 downto 0);
  signal peak_b_in         : std_logic_vector(16-1 downto 0);

begin

  peak_rst_in <= ctl_in.reset or std_logic(props_in.peak_read);
  peak_a_in   <= std_logic_vector(resize(i_odata,16));
  peak_b_in   <= std_logic_vector(resize(q_odata,16));

  -----------------------------------------------------------------------------
  -- 'enable' worker (when up/downstream Workers ready, input data valid)
  -----------------------------------------------------------------------------

  enable <= '1' when (out_in.ready = '1' and in_in.ready = '1' and in_in.valid = '1'
                and ctl_in.is_operating = '1') else '0';

  -----------------------------------------------------------------------------
  -- Take (when up/downstream Workers ready and not sending a ZLM)
  -----------------------------------------------------------------------------

  in_out.take <= '1' when (out_in.ready = '1' and in_in.ready = '1' and take = '1'
                     and ctl_in.is_operating = '1') else '0';

  -----------------------------------------------------------------------------
  -- Give (when downstream Worker ready & primitive has valid output OR the
  -- primitive was disabled and there is one valid sample on the primitive
  -- output OR we detected a ZLM and need to end the current message early)
  -----------------------------------------------------------------------------

  out_out.give <= '1' when (out_in.ready = '1' and ctl_in.is_operating = '1'
                      and (odata_vld = '1' or missed_odata_vld = '1' or force_eom = '1')) else '0';

  -----------------------------------------------------------------------------
  -- Valid (when downstream Worker ready & primitive has valid output OR the
  -- primitive was disabled and there is one valid sample on the primitive
  -- output)
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
  -- IQ Imbalance Correction component
  -----------------------------------------------------------------------------

  iq_imbalance : dsp_prims.dsp_prims.iq_imbalance_corrector
    generic map (
      DATA_WIDTH => DATA_WIDTH_c,
      ACC_PREC   => ACC_PREC_c)
    port map (
      CLK             => ctl_in.clk,
      RST             => ctl_in.reset,
      ENABLE          => std_logic(props_in.enable),
      UPDATE          => std_logic(props_in.update),
      LOG2_AVG_LEN    => unsigned(props_in.log2_averaging_length(4 downto 0)),
      NLOG2_LOOP_GAIN => unsigned(props_in.neg_log2_loop_gain(4 downto 0)),
      DIN_I           => signed(in_in.data(DATA_WIDTH_c-1    downto  0)),
      DIN_Q           => signed(in_in.data(DATA_WIDTH_c-1+16 downto 16)),
      DIN_VLD         => enable,
      DOUT_I          => i_odata,
      DOUT_Q          => q_odata,
      DOUT_VLD        => odata_vld,
      C_CORR          => open,
      D_CORR          => open);

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

  out_out.data        <= std_logic_vector(resize(q_odata,16)) &
                         std_logic_vector(resize(i_odata,16));
  out_out.byte_enable <= (others => '1');

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
