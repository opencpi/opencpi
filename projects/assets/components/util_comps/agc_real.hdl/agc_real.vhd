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

architecture rtl of agc_real_worker is

  constant DATA_WIDTH_c         : integer := to_integer(unsigned(DATA_WIDTH_p));
  constant AVG_WINDOW_c         : integer := to_integer(unsigned(AVG_WINDOW_p));
  constant MAX_MESSAGE_VALUES_c : integer := 4096;  -- from iqstream_protocol

  signal odata            : std_logic_vector(DATA_WIDTH_c-1 downto 0);
  signal odata_vld        : std_logic;
  signal missed_odata_vld : std_logic := '0';
  signal reg_wr_en        : std_logic;
  signal msg_cnt          : unsigned(integer(ceil(log2(real(MAX_MESSAGE_VALUES_c))))-1 downto 0);
  signal max_sample_cnt   : unsigned(integer(ceil(log2(real(MAX_MESSAGE_VALUES_c))))-1 downto 0);
  signal enable           : std_logic;
  signal take             : std_logic;
  signal force_som        : std_logic;
  signal force_eom        : std_logic;
  type state_t is (INIT_s, WAIT_s, SEND_s);
  signal current_state    : state_t;

begin

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

  out_out.valid <= '1' when (out_in.ready = '1' and (odata_vld = '1'
                             or missed_odata_vld = '1')) else '0';

  -----------------------------------------------------------------------------
  -- Zero-Length Message FSM
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

  backPressure : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if(ctl_in.reset = '1' or out_in.ready = '1') then
        missed_odata_vld <= '0';
      elsif (out_in.ready = '0' and odata_vld = '1') then
        missed_odata_vld <= '1';
      end if;
      if(ctl_in.reset = '1') then
        odata_vld <= '0';
      else
        odata_vld <= enable;
      end if;
    end if;
  end process backPressure;

  out_out.data        <= std_logic_vector(resize(signed(odata),16));
  out_out.byte_enable <= (others => '1');

end rtl;
