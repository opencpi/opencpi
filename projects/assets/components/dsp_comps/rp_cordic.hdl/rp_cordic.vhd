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
-- Assuming that the input data valid is held asserted, the circuit will not
-- output (valid) samples until STAGES_c + 6 clocks (input valids) later. The
-- output and internal state of the cordic is immediately held when the input
-- data valid is deasserted.
-------------------------------------------------------------------------------

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions

architecture rtl of rp_cordic_worker is

  constant DATA_WIDTH_c         : positive := to_integer(unsigned(DATA_WIDTH));
  constant DATA_EXT_c           : positive := to_integer(unsigned(DATA_EXT));
  constant STAGES_c             : positive := to_integer(unsigned(STAGES));
  constant MAX_MESSAGE_VALUES_c : integer  := 4096;  -- from iqstream_protocol

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

  --  control/data
  signal enable      : std_logic;
  signal scnt        : integer;
  signal hold_off    : std_logic;
  signal hold_off_r  : std_logic;
  signal odata_vld   : std_logic;
  signal odata       : std_logic_vector(DATA_WIDTH_c-1 downto 0);

  signal magnitude   : std_logic_vector(DATA_WIDTH_c-1 downto 0);
  signal magnitude_r : std_logic_vector(DATA_WIDTH_c-1 downto 0);
  signal phase       : std_logic_vector(DATA_WIDTH_c-1 downto 0);
  signal phase_r     : std_logic_vector(DATA_WIDTH_c-1 downto 0);

begin

  -----------------------------------------------------------------------------
  -- WSI Port Assignments
  -----------------------------------------------------------------------------

  in_out.take   <= take_s;
  out_out.give  <= give_s;
  out_out.som   <= som_s;
  out_out.eom   <= eom_s;
  out_out.valid <= vld_s;
  out_out.data  <= std_logic_vector(resize(signed(odata), 16))  -- default output
                   when (props_in.enable = '1') else (in_in.data(15 downto 0));  --BYPASS (ENABLE=0)
  -- Since ZeroLengthMessages=true for the output WSI, this signal must be controlled
  out_out.byte_enable <= (others => '1');

  -----------------------------------------------------------------------------
  --  WSI temporary signals
  -----------------------------------------------------------------------------

  take_s <= '1' when (out_in.ready = '1' and in_in.ready = '1' and ctl_in.is_operating = '1'
                      and zlm_take = '1') else '0';

  give_s <= '1' when (out_in.ready = '1' and hold_off_r = '1' and ctl_in.is_operating = '1'
                      and (enable = '1' or zlm_force_eom = '1')) else '0';

  som_s <= '1' when (out_in.ready = '1' and hold_off_r = '1'
                     and (msg_cnt = 1 or zlm_force_som = '1')) else '0';

  eom_s <= '1' when (out_in.ready = '1' and hold_off_r = '1'
                     and (msg_cnt = max_sample_cnt or zlm_force_eom = '1')) else '0';

  vld_s <= '1' when (out_in.ready = '1' and hold_off_r = '1'
                     and (enable = '1')) else '0';

  -----------------------------------------------------------------------------
  -- WSI Message Counter
  -----------------------------------------------------------------------------

  max_sample_cnt <= resize(props_in.messageSize srl 1, max_sample_cnt'length);

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
  end process proc_MessageWordCount;

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
  -- Propagation Delay Count - counter to disable output for STAGES_c+6 input samples
  -----------------------------------------------------------------------------
  proc_PropagationDelayCnt : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
        scnt       <= 0;
        hold_off   <= '0';
        hold_off_r <= '0';
      elsif (enable = '1') then
        hold_off_r <= hold_off;
        if (scnt = STAGES_c+6) then
          hold_off <= '1';
        else
          scnt     <= scnt + 1;
        end if;
      end if;
    end if;
  end process proc_PropagationDelayCnt;

  -----------------------------------------------------------------------------
  -- Rectangular-to-Polar CORDIC
  -----------------------------------------------------------------------------

  cord : dsp_prims.dsp_prims.cordic_rp
    generic map (
      DATA_WIDTH => DATA_WIDTH_c,
      DATA_EXT   => DATA_EXT_c,
      STAGES     => STAGES_c
      )
    port map (
      CLK     => ctl_in.clk,
      RST     => ctl_in.reset,
      I       => in_in.data(DATA_WIDTH_c-1+16 downto 16),
      Q       => in_in.data(DATA_WIDTH_c-1 downto 0),
      VLD_IN  => enable,
      MAG     => magnitude,
      PHASE   => phase,
      VLD_OUT => open
      );

  demod : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      odata_vld   <= enable;
      magnitude_r <= magnitude;
      if (ctl_in.reset = '1') then
        odata     <= (others => '0');
      elsif(enable = '1' and hold_off = '1') then
        odata     <= std_logic_vector(signed(phase) - signed(phase_r));
      end if;
      if (enable = '1') then
        phase_r <= phase;
      end if;
    end if;
  end process demod;

  props_out.magnitude <= short_t(resize(signed(magnitude_r),16));

end rtl;
