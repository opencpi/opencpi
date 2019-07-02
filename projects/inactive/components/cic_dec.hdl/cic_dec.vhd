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
-- Cascaded Integrator-Comb (CIC) Decimator
-------------------------------------------------------------------------------
--
-- Description:
--
-- The CIC Decimation worker instantiates two real primitives to implement a
-- complex version and handles the output message signaling.
--
-- The generic parameters for this design are as follows:
--
--      N = Number of Stages
--      M = Differential Delay
--      R = Interpolation Factor
--      DIN_WIDTH = Input data width
--      ACC_WIDTH = Accumulator width ( >= CEIL(N*log2(R*M))+DIN_WIDTH )
--      DOUT_WIDTH = Output data width
-------------------------------------------------------------------------------

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions

architecture rtl of cic_dec_worker is

  constant N_c              : integer := to_integer(unsigned(N));
  constant M_c              : integer := to_integer(unsigned(M));
  constant R_c              : integer := to_integer(unsigned(R));
  constant DIN_WIDTH_c      : integer := to_integer(unsigned(DIN_WIDTH));
  constant ACC_WIDTH_c      : integer := to_integer(unsigned(ACC_WIDTH));
  constant DOUT_WIDTH_c     : integer := to_integer(unsigned(DOUT_WIDTH));
  constant MAX_MESSAGE_VALUES_c : integer := 4096;  -- from iqstream_protocol

  -- WSI Interface
  signal msg_cnt            : unsigned(integer(ceil(log2(real(MAX_MESSAGE_VALUES_c))))-1 downto 0);
  signal max_sample_cnt     : unsigned(integer(ceil(log2(real(MAX_MESSAGE_VALUES_c))))-1 downto 0);
  -- Zero Length Messages
  type state_zlm_t is (INIT_s, WAIT_s, SEND_s);
  signal zlm_current_state  : state_zlm_t;
  signal zlm_take           : std_logic;
  signal zlm_force_som      : std_logic;
  signal zlm_force_eom      : std_logic;
  signal zlm_force_eom_l    : std_logic;
  --
  signal idata_vld          : std_logic;
  signal idata_rdy          : std_logic;
  signal odata_vld          : std_logic;
  signal i_out, q_out       : std_logic_vector(DOUT_WIDTH_c-1 downto 0);

begin

  -----------------------------------------------------------------------------
  -- idata_vld (when in operating state, upstream worker is ready and has 'valid' data
  -- and downstream workers ready)
  -----------------------------------------------------------------------------

  idata_vld <= '1' when ctl_in.is_operating = '1' and out_in.ready = '1' and in_in.ready = '1'
               and idata_rdy = '1' and in_in.valid = '1' else '0';

  -----------------------------------------------------------------------------
  -- Take (when in operating state, up/downstream workers ready and not sending a ZLM)
  -----------------------------------------------------------------------------

  in_out.take <= '1' when (ctl_in.is_operating = '1' and out_in.ready = '1' and in_in.ready = '1'
                           and zlm_take = '1') else '0';

  -----------------------------------------------------------------------------
  -- Give (when in operating state, downstream worker ready & primitive has valid output
  -- OR the ZLM is detected and the current message must be terminated early)
  -----------------------------------------------------------------------------

  out_out.give <= '1' when (ctl_in.is_operating = '1' and out_in.ready = '1'
                            and (odata_vld = '1' or zlm_force_eom = '1')) else '0';

  -----------------------------------------------------------------------------
  -- Valid (when downstream worker ready & primitive has valid output)
  -----------------------------------------------------------------------------

  out_out.valid <= '1' when out_in.ready = '1' and odata_vld = '1' else '0';

  out_out.data  <= std_logic_vector(resize(signed(q_out), 16)) & std_logic_vector(resize(signed(i_out), 16));

  -- Since ZeroLengthMessages=true for the output WSI, this signal must be controlled
  out_out.byte_enable <= (others => '1');

  -----------------------------------------------------------------------------
  -- Zero-Length Message FSM
  -----------------------------------------------------------------------------

  zlm_fsm : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if(ctl_in.reset = '1') then
        zlm_current_state <= INIT_s;
        zlm_take          <= '1';
        zlm_force_som     <= '0';
        zlm_force_eom     <= '0';
        zlm_force_eom_l   <= '0';
      else
        -- defaults
        zlm_current_state <= zlm_current_state;
        zlm_force_som     <= '0';
        zlm_take          <= '1';
        zlm_force_eom     <= '0';

        case zlm_current_state is
          when INIT_s =>
            -- 'Full' ZLM present, send ZLM
            if (in_in.ready = '1' and in_in.som = '1' and in_in.eom = '1' and in_in.valid = '0' and zlm_force_eom_l = '0') then
              zlm_current_state <= SEND_s;
           -- 'Partial' ZLM present, wait for remaining portion of ZLM
            elsif (in_in.ready = '1' and in_in.som = '1' and in_in.valid = '0') then
              zlm_current_state <= WAIT_s;
            -- Observe backpressure
            elsif (zlm_force_eom = '1' and zlm_force_som = '1' and out_in.ready = '0') then
              zlm_current_state <= INIT_s;
              zlm_force_som     <= '1';
              zlm_force_eom     <= '1';
              zlm_take          <= '0';
            end if;
          when WAIT_s =>
            zlm_take          <= '1';
            -- Valid message from upstream, return to ZLM detection
            if (in_in.ready = '1' and in_in.valid = '1') then
              zlm_current_state <= INIT_s;
            -- Remainder of 'partial' ZLM present, send ZLM
            elsif (in_in.ready = '1' and in_in.eom = '1') then
              zlm_current_state <= SEND_s;
            end if;
          when SEND_s =>
            zlm_take <= '0';
            -- Determine if in the middle of a message
            if (msg_cnt /= 1 and out_in.ready = '1' and zlm_force_eom_l = '0') then
              zlm_force_eom <= '1';
              zlm_force_eom_l <= '1';
            -- Send ZLM
            elsif (out_in.ready = '1') then
              zlm_force_eom_l <= '0';
              zlm_current_state <= INIT_s;
              zlm_force_som <= '1';
              zlm_force_eom <= '1';
            -- Observe backpressure
            else
              zlm_current_state <= zlm_current_state;
              zlm_force_som <= zlm_force_som;
              zlm_force_eom <= zlm_force_eom;
            end if;
        end case;

      end if;
    end if;
  end process zlm_fsm;

  -----------------------------------------------------------------------------
  -- WSI Message Counter
  -----------------------------------------------------------------------------

  max_sample_cnt <= resize(props_in.messageSize srl 2, max_sample_cnt'length);

  proc_MessageWordCount : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
        msg_cnt   <= (0 => '1', others => '0');
      elsif (odata_vld = '1' and zlm_force_eom_l = '0') then
        if (msg_cnt = max_sample_cnt) then
          msg_cnt <= (0 => '1', others => '0');
        else
          msg_cnt <= msg_cnt + 1;
        end if;
      end if;
    end if;
  end process;

  out_out.som <= '1' when (out_in.ready = '1'
                           and (msg_cnt = 1 or zlm_force_som = '1')) else '0';

  out_out.eom <= '1' when (out_in.ready = '1'
                           and (msg_cnt = max_sample_cnt or zlm_force_eom = '1')) else '0';

  -----------------------------------------------------------------------------
  -- CIC Decimation primitives (I & Q)
  -----------------------------------------------------------------------------

  Dec_I : dsp_prims.dsp_prims.cic_dec_gen
    generic map(
      g_n          => N_c,
      g_m          => M_c,
      g_r          => R_c,
      g_din_width  => DIN_WIDTH_c,
      g_acc_width  => ACC_WIDTH_c,
      g_dout_width => DOUT_WIDTH_c
      )
    port map(
      i_clk      => ctl_in.clk,
      i_rst      => ctl_in.reset,
      i_din      => in_in.data(DIN_WIDTH_c-1 downto 0),
      i_din_vld  => idata_vld,
      o_din_rdy  => idata_rdy,
      o_dout     => i_out,
      o_dout_vld => odata_vld,
      i_dout_rdy => out_in.ready
      );
  
  Dec_Q : dsp_prims.dsp_prims.cic_dec_gen
    generic map(
      g_n          => N_c,
      g_m          => M_c,
      g_r          => R_c,
      g_din_width  => DIN_WIDTH_c,
      g_acc_width  => ACC_WIDTH_c,
      g_dout_width => DOUT_WIDTH_c
      )
    port map(
      i_clk      => ctl_in.clk,
      i_rst      => ctl_in.reset,
      i_din      => in_in.data(2*DIN_WIDTH_c-1 downto DIN_WIDTH_c),
      i_din_vld  => idata_vld,
      o_din_rdy  => open,
      o_dout     => q_out,
      o_dout_vld => open,
      i_dout_rdy => out_in.ready
      );

end rtl;
