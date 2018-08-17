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

architecture rtl of phase_to_amp_cordic_worker is

  constant DATA_WIDTH_c : positive := to_integer(unsigned(DATA_WIDTH));
  constant DATA_EXT_c   : positive := to_integer(unsigned(DATA_EXT));
  constant STAGES_c     : positive := to_integer(unsigned(STAGES));
  constant MAX_MESSAGE_VALUES_c : integer := 4096;  -- from iqstream_protocol

  -- WSI Interface
  signal take_s, give_s      : std_logic;
  signal som_s, eom_s, vld_s : std_logic;
  signal msg_cnt             : unsigned(integer(ceil(log2(real(MAX_MESSAGE_VALUES_c))))-1 downto 0);
  signal max_sample_cnt      : unsigned(integer(ceil(log2(real(MAX_MESSAGE_VALUES_c))))-1 downto 0);

  -- Zero Length Messages
  type state_zlm_t is (INIT_s, WAIT_s, SEND_s, TERM_CURR_MSG_s);
  signal zlm_current_state : state_zlm_t;
  signal zlm_take          : std_logic;
  signal zlm_force_som     : std_logic;
  signal zlm_force_eom     : std_logic;
  signal zlm_force_eom_l   : std_logic;
  signal zlm_force_vld     : std_logic;

  -- Phase to Amplitude CORDIC control/data
  signal enable      : std_logic;
  signal scnt        : integer;
  signal hold_off    : std_logic;
  signal idata_mag   : std_logic_vector(DATA_WIDTH_c-1 downto 0);
  signal idata_phase : std_logic_vector(DATA_WIDTH_c-1 downto 0);
  signal odata_vld   : std_logic;
  signal odata_i     : std_logic_vector(DATA_WIDTH_c-1 downto 0);
  signal odata_q     : std_logic_vector(DATA_WIDTH_c-1 downto 0);

begin

  -----------------------------------------------------------------------------
  -- WSI Port Assignments
  -----------------------------------------------------------------------------

  in_out.take   <= take_s;
  out_out.give  <= give_s;
  out_out.som   <= som_s;
  out_out.eom   <= eom_s;
  out_out.valid <= vld_s;
  out_out.data  <= std_logic_vector(resize(signed(odata_q), 16)) &              -- default output
                   std_logic_vector(resize(signed(odata_i), 16))
                   when (props_in.enable = '1') else (idata_mag & in_in.data);  --BYPASS (ENABLE=0)
  -- Since ZeroLengthMessages=true for the output WSI, this signal must be controlled
  out_out.byte_enable <= (others => '1');

  -----------------------------------------------------------------------------
  --  WSI temporary signals
  -----------------------------------------------------------------------------

  take_s <= '1' when (out_in.ready = '1' and in_in.ready = '1' and ctl_in.is_operating = '1'
                      and zlm_take = '1') else '0';

  give_s <= '1' when (out_in.ready = '1' and hold_off = '1' and ctl_in.is_operating = '1'
                      and (enable = '1' or zlm_force_eom = '1')) else '0';

  som_s <= '1' when (out_in.ready = '1' and hold_off = '1'
                     and (msg_cnt = 1 or zlm_force_som = '1')) else '0';

  eom_s <= '1' when (out_in.ready = '1' and hold_off = '1'
                     and (msg_cnt = max_sample_cnt or zlm_force_eom = '1')) else '0';

  vld_s <= '1' when (out_in.ready = '1' and hold_off = '1'
                     and enable = '1') else '0';

  -----------------------------------------------------------------------------
  -- WSI Message Counter
  -----------------------------------------------------------------------------

  max_sample_cnt <= resize(props_in.messageSize srl 2, max_sample_cnt'length);

  proc_MessageWordCount : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
        msg_cnt   <= (0 => '1', others => '0');
      elsif (give_s = '1' and zlm_force_eom_l = '0') then
        if (msg_cnt = max_sample_cnt) then
          msg_cnt <= (0 => '1', others => '0');
        else
          msg_cnt <= msg_cnt + 1;
        end if;
      end if;
    end if;
  end process;

  -----------------------------------------------------------------------------
  -- Zero-Length Message FSM
  -- the zlm_fsm is being depreciated, instead see dc_offset_filter.vhd
  -- for recommended mechanism for dealing with primitive latency
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
        zlm_force_vld     <= '0';
      else
        -- defaults
        zlm_current_state <= zlm_current_state;
        zlm_force_som     <= '0';
        zlm_force_eom     <= '0';

        case zlm_current_state is
          when INIT_s =>
            zlm_take <= '1';
            -- 'Full' ZLM present, send a ZLM
            if (in_in.som = '1' and in_in.eom = '1' and in_in.valid = '0' and zlm_force_eom_l = '0') then
              zlm_current_state <= SEND_s;
            -- 'Partial' ZLM present, wait for remaining portion of ZLM
            elsif (in_in.som = '1' and in_in.valid = '0') then
              zlm_current_state <= WAIT_s;
            end if;
          when WAIT_s =>
            zlm_take <= '1';
            -- Valid message from upstream, return to ZLM detection
            if (in_in.valid = '1') then
              zlm_current_state <= INIT_s;
            -- Remainder of 'partial' ZLM present, send a ZLM
            elsif (in_in.eom = '1') then
              zlm_current_state <= SEND_s;
            end if;
          when SEND_s =>
            if (out_in.ready = '1') then
              zlm_force_eom_l <= '1';
              -- Determine if in the middle of a message
              if (msg_cnt /= 1 and zlm_force_eom_l = '0')then
                zlm_current_state <= TERM_CURR_MSG_s;
                zlm_take          <= '1';
                zlm_force_eom     <= '1';
                zlm_force_vld     <= '1';
              -- Send a ZLM
              else
                zlm_take          <= '0';
                zlm_current_state <= INIT_s;
                zlm_force_som     <= '1';
                zlm_force_eom     <= '1';
              end if;
            end if;
          when TERM_CURR_MSG_s =>
            if (out_in.ready = '1') then
              zlm_current_state <= SEND_s;
              zlm_force_eom     <= '0';
              zlm_force_vld     <= '0';
            end if;
        end case;
      end if;
    end if;
  end process zlm_fsm;

  -----------------------------------------------------------------------------
  -- 'enable' (when up/downstream Workers ready, input data valid)
  -----------------------------------------------------------------------------
  enable <= '1' when (out_in.ready = '1' and in_in.ready = '1' and in_in.valid = '1' and ctl_in.is_operating = '1') else '0';

  -----------------------------------------------------------------------------
  -- Propagation Delay Count - counter to disable output for STAGES_c+1 input samples
  -----------------------------------------------------------------------------
  proc_PropagationDelayCnt : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if(ctl_in.reset = '1') then
        scnt     <= 0;
        hold_off <= '0';
      elsif (scnt = STAGES_c+1) then
        hold_off <= '1';
      elsif (enable = '1') then
        scnt <= scnt + 1;
      end if;
    end if;
  end process;

  -----------------------------------------------------------------------------
  -- Magnitude is set by property, but Phase Increment is accumulated
  -----------------------------------------------------------------------------

  idata_mag <= std_logic_vector(props_in.magnitude(DATA_WIDTH_c-1 downto 0));

  proc_InputPhaseAccum : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
        idata_phase <= (others => '0');
      elsif (enable = '1') then
        idata_phase <= std_logic_vector(signed(idata_phase) + signed(in_in.data));
      end if;
    end if;
  end process;

  -----------------------------------------------------------------------------
  -- Polar-to-Rectangular CORDIC
  -----------------------------------------------------------------------------

  inst_cord : dsp_prims.dsp_prims.cordic_pr
    generic map (
      DATA_WIDTH => DATA_WIDTH_c,
      DATA_EXT   => DATA_EXT_c,
      STAGES     => STAGES_c
      )
    port map (
      CLK     => ctl_in.clk,
      RST     => ctl_in.reset,
      MAG     => idata_mag,
      PHASE   => idata_phase,
      VLD_IN  => enable,
      I       => odata_i,
      Q       => odata_q,
      VLD_OUT => odata_vld
      );

end rtl;
