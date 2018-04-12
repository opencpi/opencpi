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
-- Cascaded Integrator-Comb (CIC) Interpolator
-------------------------------------------------------------------------------
--
-- Description:
--
-- The CIC Interpolation worker instantiates two real primitives to implement a
-- complex version and handles the output message signaling. The worker provides
-- parameters for configuring the primitive's generic ports.
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

architecture rtl of cic_int_worker is

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
  type state_zlm_t is (INIT_s, WAIT_s, SEND_s, TERM_CURR_MSG_s);
  signal zlm_current_state  : state_zlm_t;
  signal zlm_detected_l     : std_logic;
  signal zlm_take           : std_logic;
  signal zlm_force_som      : std_logic;
  signal zlm_force_eom      : std_logic;
  signal zlm_force_eom_l    : std_logic;
  --
  signal int_cnt            : unsigned(15 downto 0);
  signal idata_vld          : std_logic;
  signal enable_out         : std_logic;
  signal odata_vld          : std_logic;
  signal i_out, q_out       : std_logic_vector(DOUT_WIDTH_c-1 downto 0);

begin

  -----------------------------------------------------------------------------
  -- idata_vld (when in operating state, upstream worker is ready and has 'valid' data
  -- and downstream worker ready and interpolation counter at 'begin')
  -----------------------------------------------------------------------------

  idata_vld <= '1' when (ctl_in.is_operating = '1' and out_in.ready = '1' and in_in.ready = '1'
                         and in_in.valid = '1' and int_cnt = 0) else '0';

  -----------------------------------------------------------------------------
  -- enable_out (when idata_vld OR up/downstream workers ready and
  -- interpolation cycle has 'begun')
  -----------------------------------------------------------------------------

  enable_out <= '1' when (idata_vld = '1'
                          or (out_in.ready = '1' and in_in.ready = '1' and int_cnt /= 0)) else '0';

  -----------------------------------------------------------------------------
  -- Take (when in operating state, up/downstream workers ready,
  -- interpolation counter at 'begin' and not sending a ZLM)
  -----------------------------------------------------------------------------

  in_out.take <= '1' when (ctl_in.is_operating = '1' and out_in.ready = '1' and in_in.ready = '1'
                           and int_cnt = 0 and zlm_take = '1') else '0';

  -----------------------------------------------------------------------------
  -- Give (when in operating state, downstream worker ready & primitive has valid output
  -- OR the ZLM is detected and the current message must be terminated early)
  -----------------------------------------------------------------------------

  out_out.give <= '1' when ((ctl_in.is_operating = '1' and out_in.ready = '1' and zlm_force_eom = '1')
                            or (enable_out = '1' and odata_vld = '1')) else '0';

  -----------------------------------------------------------------------------
  -- Valid (when in operating state, upstream worker is ready and has 'valid' data
  -- and downstream worker ready and interpolation count is at 'beginning'
  -- and primitive has valid output data, OR up/downstream workers ready and
  -- interpolation cycle has begun and primitive has valid output data)
  -----------------------------------------------------------------------------

  out_out.valid <= '1' when (enable_out = '1' and odata_vld = '1') else '0';

  out_out.data  <= std_logic_vector(resize(signed(i_out), 16)) & std_logic_vector(resize(signed(q_out), 16));

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
        zlm_detected_l    <= '0';
        zlm_take          <= '1';
        zlm_force_som     <= '0';
        zlm_force_eom     <= '0';
        zlm_force_eom_l   <= '0';
      else
        -- defaults
        zlm_current_state <= zlm_current_state;
        zlm_take          <= '1';
        zlm_force_som     <= '0';
        zlm_force_eom     <= '0';

        case zlm_current_state is
          when INIT_s =>
            -- 'Full' ZLM present, send ZLM
            if (in_in.som = '1' and in_in.eom = '1' and in_in.valid = '0') then
              zlm_current_state <= SEND_s;
              zlm_detected_l    <= '1';
              -- 'Partial' ZLM present, wait for remaininng portion of ZLM
            elsif (in_in.som = '1' and in_in.valid = '0') then
              zlm_current_state <= WAIT_s;
            end if;
          when WAIT_s =>
            -- Valid message from upstream, return to ZLM detection
            if (in_in.valid = '1') then
              zlm_current_state <= INIT_s;
              -- Remainder of 'partial' ZLM present, send ZLM
            elsif (in_in.eom = '1') then
              zlm_current_state <= SEND_s;
            end if;
          when SEND_s =>
            if (out_in.ready = '1') then
              -- Determine if interpolation cycle is (near) complete, but message is not
              if (int_cnt = unsigned(R)-2 and msg_cnt /= 1 and zlm_force_eom_l = '0') then
                zlm_current_state <= TERM_CURR_MSG_s;
                zlm_force_eom     <= '1';
                zlm_force_eom_l   <= '1';
                -- Send ZLM
              elsif (int_cnt = 0) then
                zlm_current_state <= INIT_s;
                zlm_take          <= '0';
                zlm_force_som     <= '1';
                zlm_force_eom     <= '1';
                zlm_force_eom_l   <= '1';
              end if;
            end if;
          when TERM_CURR_MSG_s =>
            -- Hold zlm_force_eom until next out_in.ready = '1'.
            -- This case is meant to deal with backpressure,
            -- where out_in.ready is toggling, which could
            -- result in the zlm_force_eom <= '1' being missed.
            zlm_force_eom <= '1';
            if (out_in.ready = '1') then
              -- Ensure EOM is active for only one out_in.ready clock cycle!!!
              -- Otherwise, the OCP will 'imply' a SOM, and may cause
              -- a new message if 'give' is not handled correctly.
              zlm_force_eom <= '0';
              -- Now that current message is completed, send ZLM to downstream Worker
              if (int_cnt = 0) then
                zlm_current_state <= SEND_s;
              end if;
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
      elsif (enable_out = '1' and odata_vld = '1') then
        if (msg_cnt = max_sample_cnt) then
          msg_cnt <= (0 => '1', others => '0');
        else
          msg_cnt <= msg_cnt + 1;
        end if;
      end if;
    end if;
  end process;

  out_out.som <= '1' when ((enable_out = '1' and odata_vld = '1'
                            and msg_cnt = 1)
                           or zlm_force_som = '1') else '0';

  out_out.eom <= '1' when ((enable_out = '1' and odata_vld = '1'
                            and msg_cnt = max_sample_cnt)
                           or zlm_force_eom = '1') else '0';

  -----------------------------------------------------------------------------
  -- Interpolation count tracker
  -----------------------------------------------------------------------------

  proc_InterpolationCounter : process (ctl_in.clk)
  begin
    if (rising_edge(ctl_in.clk)) then
      if (ctl_in.reset = '1') then
        int_cnt <= (others => '0');
      elsif (out_in.ready = '1' and in_in.ready = '1') then
        -- begin (do not if input ZLM detected)
        if (in_in.valid = '1' and int_cnt = 0 and zlm_detected_l = '0') then
          int_cnt <= to_unsigned(1, 16);
          -- end
        elsif (int_cnt = unsigned(R)-1) then
          int_cnt <= (others => '0');
          -- continue
        elsif (zlm_force_eom_l = '0') then
          int_cnt <= int_cnt + 1;
        end if;
      end if;
    end if;
  end process;

  -----------------------------------------------------------------------------
  -- CIC Interpolation primitives (I & Q)
  -----------------------------------------------------------------------------

  Int_I : dsp_prims.dsp_prims.cic_int_gen
    generic map(
      N          => N_c,
      M          => M_c,
      R          => R_c,
      DIN_WIDTH  => DIN_WIDTH_c,
      ACC_WIDTH  => ACC_WIDTH_c,
      DOUT_WIDTH => DOUT_WIDTH_c
      )
    port map(
      CLK      => ctl_in.clk,
      RST      => ctl_in.reset,
      DIN_VLD  => idata_vld,
      DIN      => in_in.data(DIN_WIDTH_c-1+16 downto 16),
      DOUT_EN  => enable_out,
      DOUT_VLD => odata_vld,
      DOUT     => i_out
      );
  Int_Q : dsp_prims.dsp_prims.cic_int_gen
    generic map(
      N          => N_c,
      M          => M_c,
      R          => R_c,
      DIN_WIDTH  => DIN_WIDTH_c,
      ACC_WIDTH  => ACC_WIDTH_c,
      DOUT_WIDTH => DOUT_WIDTH_c
      )
    port map(
      CLK      => ctl_in.clk,
      RST      => ctl_in.reset,
      DIN_VLD  => idata_vld,
      DIN      => in_in.data(DIN_WIDTH_c-1 downto 0),
      DOUT_EN  => enable_out,
      DOUT_VLD => open,
      DOUT     => q_out
      );

  -----------------------------------------------------------------------------
  -- DEBUG: CHIPSCOPE
  -----------------------------------------------------------------------------
  gen_DEBUG : if its(VIVADO_ILA_p) or its(CHIPSCOPE_p) generate
    signal DATA     : std_logic_vector(111 downto 0);
    signal TRIG0    : std_logic_vector(47 downto 0);
  begin

    gen_VILA: if its(VIVADO_ILA_p) generate
      component vivado_ila
        port (
          clk     : in    std_logic;
          probe0  : in    std_logic_vector(111 downto 0);
          probe1  : in    std_logic_vector(47 downto 0));
      end component;
  
    begin
      inst_ILA : vivado_ila
        port map (
          clk     => ctl_in.clk,
          probe0  => DATA,
          probe1  => TRIG0
          );
  
    end generate;
  
    gen_CS : if its(CHIPSCOPE_p) generate

      component chipscope_icon
        port (
          CONTROL0 : inout std_logic_vector(35 downto 0));
      end component;
  
      component chipscope_ila_trig_n_data
        port (
          CONTROL : inout std_logic_vector(35 downto 0);
          CLK     : in    std_logic;
          DATA    : in    std_logic_vector(111 downto 0);
          TRIG0   : in    std_logic_vector(47 downto 0));
      end component;
  
      signal CONTROL0 : std_logic_vector(35 downto 0);
  
    begin
  
      inst_ICON : chipscope_icon
        port map (
          CONTROL0 => CONTROL0
          );
      inst_ILA : chipscope_ila_trig_n_data
        port map (
          CONTROL => CONTROL0,
          CLK     => ctl_in.clk,
          DATA    => DATA,
          TRIG0   => TRIG0
          );

    end generate;
  
    DATA(111 downto 104) <= std_logic_vector(int_cnt(15 downto 8));
    DATA(103 downto 96)  <= std_logic_vector(int_cnt(7 downto 0));
    DATA(95 downto 92)   <= '0' & '0' & zlm_detected_l & zlm_force_eom_l;
    DATA(91 downto 88)   <= '0' & odata_vld & enable_out & idata_vld;
    DATA(87 downto 84)   <= zlm_take & zlm_force_som & zlm_force_eom & out_in.ready;
    DATA(83 downto 80)   <= in_in.ready & in_in.valid & in_in.som & in_in.eom;
    DATA(79 downto 64)   <= std_logic_vector(resize(msg_cnt,16));
    DATA(63 downto 48)   <= in_in.data(DIN_WIDTH_c-1+16 downto 16);
    DATA(47 downto 32)   <= in_in.data(DIN_WIDTH_c-1 downto 0);
    DATA(31 downto 16)   <= i_out;
    DATA(15 downto 0)    <= q_out;

    TRIG0(47 downto 40) <= std_logic_vector(int_cnt(15 downto 8));
    TRIG0(39 downto 32) <= std_logic_vector(int_cnt(7 downto 0));
    TRIG0(31 downto 28) <= '0' & '0' & zlm_detected_l & zlm_force_eom_l;
    TRIG0(27 downto 24) <= '0' & odata_vld & enable_out & idata_vld;
    TRIG0(23 downto 20) <= zlm_take & zlm_force_som & zlm_force_eom & out_in.ready;
    TRIG0(19 downto 16) <= in_in.ready & in_in.valid & in_in.som & in_in.eom;
    TRIG0(15 downto 0)  <= std_logic_vector(resize(msg_cnt,16));

  end generate;

end rtl;
