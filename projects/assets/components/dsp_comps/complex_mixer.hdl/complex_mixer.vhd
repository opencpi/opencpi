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
-- Complex Mixer
-------------------------------------------------------------------------------
--
-- Description:
--
-- The Complex Mixer worker inputs complex signed samples and performs a
-- complex multiply with a digital sine wave produced by an numerically
-- controlled oscillator (NCO). The resulting output data is a frequency
-- shifted version of the input data. The frequency and amplitude of the sine
-- wave are programmable as well as the ability to bypass the module. Generics
-- can be used to control the width of the data, phase accumulator, and phase
-- increment of the NCO as well as control insertion of a peak detection
-- circuit.
-------------------------------------------------------------------------------

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.types.all, ocpi.util.all; -- remove this to avoid all ocpi name collisions
architecture rtl of complex_mixer_worker is

  constant NCO_DATA_WIDTH_c   : integer := to_integer(unsigned(NCO_DATA_WIDTH_p));
  constant INPUT_DATA_WIDTH_c : integer := to_integer(unsigned(INPUT_DATA_WIDTH_p));
  constant CORDIC_STAGES_c    : integer := to_integer(unsigned(CORDIC_STAGES_p));
  constant RND_DATA_WIDTH_c   : integer := 16;
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

  -- NCO signals
  signal nco_enable  : std_logic;
  signal scnt        : integer;
  signal hold_off    : std_logic;
  signal hold_off_r  : std_logic;
  signal hold_off_r2 : std_logic;
  signal nco_out_i   : std_logic_vector(NCO_DATA_WIDTH_c-1 downto 0);
  signal nco_out_q   : std_logic_vector(NCO_DATA_WIDTH_c-1 downto 0);

  -- Complex Multiplier
  signal mult_enable   : std_logic;
  signal mix_i         : std_logic_vector(RND_DATA_WIDTH_c-1 downto 0);
  signal mix_q         : std_logic_vector(RND_DATA_WIDTH_c-1 downto 0);

  -- Peak detector
  signal peak_rst_in : std_logic;
  signal peak_a_in   : std_logic_vector(15 downto 0);
  signal peak_b_in   : std_logic_vector(15 downto 0);
  signal peak_out    : std_logic_vector(15 downto 0);

begin

  -----------------------------------------------------------------------------
  -- WSI Port Assignments
  -----------------------------------------------------------------------------
  in_out.take   <= take_s;
  out_out.give  <= give_s;
  out_out.som   <= som_s;
  out_out.eom   <= eom_s;
  out_out.valid <= vld_s;
  out_out.data  <= std_logic_vector(resize(signed(mix_q), 16))  -- default output
                   & std_logic_vector(resize(signed(mix_i), 16))
                   when (props_in.enable = '1') else in_in.data -- BYPASS: input(ENABLE=0)
                   when (props_in.data_select = '0')            -- BYPASS: NCO output(ENABLE=0, DATA_SELECT=1)
                   else std_logic_vector(resize(signed(nco_out_q), 16)) & std_logic_vector(resize(signed(nco_out_i), 16));

  -- Since ZeroLengthMessages=true for the output WSI, this signal must be controlled
  -- (revisit once AV-4200 is resolved)
  out_out.byte_enable <= (others => '1');

  -----------------------------------------------------------------------------
  --  WSI temporary signals
  -----------------------------------------------------------------------------

  take_s <= '1' when (out_in.ready = '1' and in_in.ready = '1' and ctl_in.is_operating = '1'
                      and hold_off_r2 = '1' and zlm_take = '1') else '0';

  give_s <= '1' when (out_in.ready = '1' and hold_off_r2 = '1' and ctl_in.is_operating = '1'
                      and (mult_enable = '1' or zlm_force_eom = '1')) else '0';

  som_s <= '1' when (out_in.ready = '1' and hold_off_r2 = '1'
                       and (msg_cnt = 1 or zlm_force_som = '1')) else '0';

  eom_s <= '1' when (out_in.ready = '1' and hold_off_r2 = '1'
                       and (msg_cnt = max_sample_cnt or zlm_force_eom = '1')) else '0';

  vld_s <= '1' when (out_in.ready = '1' and hold_off_r2 = '1')
           and (mult_enable = '1') else '0';

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
            zlm_take          <= '1';
            -- 'Full' ZLM present, send a ZLM
            if (in_in.ready = '1' and in_in.som = '1' and in_in.eom = '1' and in_in.valid = '0' and zlm_force_eom_l = '0') then
              zlm_current_state <= SEND_s;
           -- 'Partial' ZLM present, wait for remaining portion of ZLM
            elsif (in_in.ready = '1' and in_in.som = '1' and in_in.valid = '0') then
              zlm_current_state <= WAIT_s;
            end if;
          when WAIT_s =>
            zlm_take          <= '1';
            -- Valid message from upstream, return to ZLM detection
            if (in_in.ready = '1' and in_in.valid = '1') then
              zlm_current_state <= INIT_s;
            -- Remainder of 'partial' ZLM present, send a ZLM
            elsif (in_in.ready = '1' and in_in.eom = '1') then
              zlm_current_state <= SEND_s;
            end if;
          when SEND_s =>
            if (out_in.ready = '1') then
              zlm_force_eom_l <= '1';
              -- Determine if in the middle of a message
              if (msg_cnt /= 1 and zlm_force_eom_l = '0')then
                zlm_current_state <= TERM_CURR_MSG_s;
                zlm_take <= '1';
                zlm_force_eom <= '1';
                zlm_force_vld <= '1';
              -- Send a ZLM
              else
                zlm_take <= '0';
                zlm_current_state <= INIT_s;
                zlm_force_som <= '1';
                zlm_force_eom <= '1';
              end if;
            end if;
          when TERM_CURR_MSG_s =>
            if (out_in.ready = '1') then
              zlm_current_state <= SEND_s;
              zlm_force_eom <= '0';
              zlm_force_vld <= '0';
            end if;
        end case;
      end if;
    end if;
  end process zlm_fsm;

  -----------------------------------------------------------------------------
  -- 'enable' NCO (when up/downstream Workers ready, input data valid)
  -----------------------------------------------------------------------------

  nco_enable <= '1' when out_in.ready = '1' and in_in.ready = '1' and in_in.valid = '1' and ctl_in.is_operating = '1' else '0';

  -----------------------------------------------------------------------------
  -- Output Clamp Counter - cntr to disable output for STAGES_c+1 input samples
  -----------------------------------------------------------------------------

  proc_NcoInitHoldOff : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1' or props_in.phs_inc_written = '1' or props_in.mag_written = '1') then
        scnt        <= 0;
        hold_off    <= '0';
        hold_off_r  <= '0';
        hold_off_r2 <= '0';
      elsif (nco_enable = '1') then
        hold_off_r  <= hold_off;
        hold_off_r2 <= hold_off_r;
        if (scnt = CORDIC_STAGES_c+1) then
          hold_off <= '1';
        else
          scnt <= scnt + 1;
        end if;
      end if;
    end if;
  end process;

  -----------------------------------------------------------------------------
  -- 'enable' multiplier (when up/downstream Workers ready, input valid and NCO output valid)
  -----------------------------------------------------------------------------

  mult_enable <= '1' when out_in.ready = '1' and in_in.ready = '1' and in_in.valid = '1'
                          and ctl_in.is_operating = '1' and hold_off = '1' else '0';

  -----------------------------------------------------------------------------
  -- NCO Primitive
  -----------------------------------------------------------------------------
  nco_inst : dsp_prims.dsp_prims.nco
    generic map (
      DATA_WIDTH    => NCO_DATA_WIDTH_c,
      PHS_INC_WIDTH => props_in.phs_inc'length,
      PHS_ACC_WIDTH => props_in.phs_init'length,
      CORDIC_STAGES => CORDIC_STAGES_c)
    port map (
      CLK      => ctl_in.clk,
      RST      => ctl_in.reset,
      EN       => nco_enable,
      MAG      => std_logic_vector(props_in.mag(NCO_DATA_WIDTH_c-1 downto 0)),
      PHS_INC  => std_logic_vector(props_in.phs_inc),
      PHS_INIT => std_logic_vector(props_in.phs_init),
      I_OUT    => nco_out_i,
      Q_OUT    => nco_out_q,
      VLD_OUT  => open);        -- This is wired to EN

  -----------------------------------------------------------------------------
  -- Complex Multiply
  -----------------------------------------------------------------------------
  mult_inst : util_prims.util_prims.complex_mult
    generic map (
      A_WIDTH    => NCO_DATA_WIDTH_c,
      B_WIDTH    => INPUT_DATA_WIDTH_c,
      PROD_WIDTH => RND_DATA_WIDTH_c
      )
    port map (
      CLK      => ctl_in.clk,
      RST      => ctl_in.reset,
      DIN_A_RE => nco_out_i,
      DIN_A_IM => nco_out_q,
      DIN_B_RE => in_in.data(INPUT_DATA_WIDTH_c-1 downto 0),
      DIN_B_IM => in_in.data(INPUT_DATA_WIDTH_c+15 downto 16),
      DIN_VLD  => mult_enable,
      DOUT_RE  => mix_i,
      DOUT_IM  => mix_q,
      DOUT_VLD => open);        -- This is wired to DIN_VLD

  -----------------------------------------------------------------------------
  -- Peak Detection primitive. Value is cleared when read
  -----------------------------------------------------------------------------
  peak_rst_in <= ctl_in.reset or std_logic(props_in.peak_read);
  peak_a_in  <= std_logic_vector(resize(signed(mix_i), 16))  -- default output
                when (props_in.enable = '1') else in_in.data(15 downto 0);  --BYPASS (ENABLE=0)
  peak_b_in  <= std_logic_vector(resize(signed(mix_q), 16))  -- default output
                when (props_in.enable = '1') else in_in.data(31 downto 16);  --BYPASS (ENABLE=0)


  pm_gen : if its(PEAK_MONITOR_p) generate

    pd : util_prims.util_prims.peakDetect
      port map (
        CLK_IN   => ctl_in.clk,
        RST_IN   => peak_rst_in,
        EN_IN    => mult_enable,
        A_IN     => peak_a_in,
        B_IN     => peak_b_in,
        PEAK_OUT => peak_out);

    props_out.peak <= signed(peak_out);

  end generate pm_gen;

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
  
    gen_CS:   if its(CHIPSCOPE_p)  generate
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

    DATA(111 downto 104) <= std_logic_vector(to_unsigned(scnt, 8));
    DATA(103 downto 96)  <= x"0" & vld_s & som_s & eom_s & mult_enable;
    DATA(95 downto 92)   <= hold_off_r2 & hold_off_r & hold_off & nco_enable;
    DATA(91 downto 88)   <= give_s & take_s & zlm_force_vld & zlm_force_eom_l;
    DATA(87 downto 84)   <= zlm_take & zlm_force_som & zlm_force_eom & out_in.ready;
    DATA(83 downto 80)   <= in_in.ready & in_in.valid & in_in.som & in_in.eom;
    DATA(79 downto 64)   <= std_logic_vector(resize(msg_cnt,16));
    DATA(63 downto 48)   <= std_logic_vector(resize(signed(mix_i), 16));
    DATA(47 downto 32)   <= std_logic_vector(resize(signed(mix_q), 16));
    DATA(31 downto 16)   <= std_logic_vector(resize(signed(nco_out_i), 16));
    DATA(15 downto 0)    <= std_logic_vector(resize(signed(nco_out_q), 16));
  
    TRIG0(47 downto 40) <= std_logic_vector(to_unsigned(scnt, 8));
    TRIG0(39 downto 32) <= x"0" & vld_s & som_s & eom_s & mult_enable;
    TRIG0(31 downto 28) <= hold_off_r2 & hold_off_r & hold_off & nco_enable;
    TRIG0(27 downto 24) <= give_s & take_s & zlm_force_vld & zlm_force_eom_l;
    TRIG0(23 downto 20) <= zlm_take & zlm_force_som & zlm_force_eom & out_in.ready;
    TRIG0(19 downto 16) <= in_in.ready & in_in.valid & in_in.som & in_in.eom;
    TRIG0(15 downto 0)  <= std_logic_vector(resize(msg_cnt,16));
  end generate;

end rtl;
