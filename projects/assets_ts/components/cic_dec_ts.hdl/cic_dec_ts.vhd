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

-- Cascaded Integrator-Comb (CIC) Decimator
library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.types.all; use ocpi.util.all;-- remove this to avoid all ocpi name collisions

architecture rtl of worker is

  constant c_n                      : positive := to_integer(N);
  constant c_m                      : positive := to_integer(M);
  constant c_r                      : positive := to_integer(R);
  constant c_group_delay            : positive := to_integer(GROUP_DELAY);
  constant c_count_ones_in_r        : natural  := count_ones(from_ushort(R));
  constant c_data_width             : positive := to_integer(DATA_WIDTH);
  constant c_acc_width              : positive := integer(ceil(real(c_n)*log2(real(c_r*c_m))))+c_data_width;
      
  signal s_prim_rst                 : std_logic;
  signal s_idata_vld, s_idata_rdy   : std_logic;
  signal s_odata_vld, s_odata_rdy   : std_logic;
  signal s_i_in, s_q_in             : std_logic_vector(c_data_width-1 downto 0);
  signal s_i_out, s_q_out           : std_logic_vector(c_data_width-1 downto 0);
  signal s_samples_opcode           : std_logic;
  signal s_samples_eom              : std_logic;
  signal s_interval_opcode          : std_logic;
  signal s_flush_opcode             : std_logic;
  signal s_sync_opcode              : std_logic;
  signal s_take_flush               : std_logic;
  signal s_samples_msg_in_progress  : std_logic;
  signal s_non_samples_msg_on_input : std_logic;
  signal s_interval_data            : std_logic_vector(ocpi_port_in_data_width-1 downto 0);
  signal s_interval_vld             : std_logic;
  signal s_interval_eom             : std_logic;
  signal s_group_delay_max          : std_logic;
  signal s_group_delay_zero         : std_logic;
  signal s_group_delay_counter      : unsigned(width_for_max(c_group_delay) downto 0);
  -- Peak detector
  signal s_peak_rst_in              : std_logic;
  signal s_peak_a_in                : std_logic_vector(c_data_width-1 downto 0);
  signal s_peak_b_in                : std_logic_vector(c_data_width-1 downto 0);
  signal s_peak_out                 : std_logic_vector(c_data_width-1 downto 0);
begin
  
  s_samples_opcode  <= in_in.valid and to_bool(in_in.opcode=ComplexShortWithMetadata_samples_op_e);
  s_interval_opcode <= in_in.valid and to_bool(in_in.opcode=ComplexShortWithMetadata_interval_op_e);
  s_flush_opcode    <= in_in.ready and to_bool(in_in.opcode=ComplexShortWithMetadata_flush_op_e);
  s_sync_opcode     <= in_in.ready and to_bool(in_in.opcode=ComplexShortWithMetadata_sync_op_e);
  
  -- Enable input to primitive
  s_idata_vld <= s_samples_opcode or (s_flush_opcode and not s_group_delay_zero);

  -- For samples, take when data is present and primitive is ready.
  -- For interval, take when completing interval message.
  -- For flush, when group delay counter is zero and flush message is complete
  -- and output is allowed
  -- Otherwise, whenever messages are available on input and output is allowed
  -- and previous samples message is complete
  s_take_flush <= s_flush_opcode and s_group_delay_zero and not s_samples_msg_in_progress
                  and out_in.ready; 
  in_out.take  <= s_idata_rdy    when its(s_samples_opcode) else
                  s_interval_eom when its(s_interval_opcode) and its(in_in.eom) else
                  s_take_flush   when its(s_flush_opcode) else
                  in_in.ready and out_in.ready and not s_samples_msg_in_progress;

  -- Input data to primitive (zeros when flushing)
  s_i_in <= (others => '0') when its(s_flush_opcode) else in_in.data(c_data_width-1 downto 0);
  s_q_in <= (others => '0') when its(s_flush_opcode) else in_in.data(2*c_data_width-1 downto c_data_width);

  -- Reset to primitive
  -- Reset when control plane reset, sync, or taking flush
  s_prim_rst <= ctl_in.reset or s_sync_opcode or s_take_flush;
  
  -- CIC Decimation primitives (I & Q)
  Dec_I : dsp_prims.dsp_prims.cic_dec_gen
    generic map(
      g_n          => c_n,
      g_m          => c_m,
      g_r          => c_r,
      g_din_width  => c_data_width,
      g_acc_width  => c_acc_width,
      g_dout_width => c_data_width
      )
    port map(
      i_clk      => ctl_in.clk,
      i_rst      => s_prim_rst,
      i_din      => s_i_in,
      i_din_vld  => s_idata_vld,
      o_din_rdy  => s_idata_rdy,
      o_dout     => s_i_out,
      o_dout_vld => s_odata_vld,
      i_dout_rdy => s_odata_rdy
      );
  Dec_Q : dsp_prims.dsp_prims.cic_dec_gen
    generic map(
      g_n          => c_n,
      g_m          => c_m,
      g_r          => c_r,
      g_din_width  => c_data_width,
      g_acc_width  => c_acc_width,
      g_dout_width => c_data_width
      )
    port map(
      i_clk      => ctl_in.clk,
      i_rst      => s_prim_rst,
      i_din      => s_q_in,
      i_din_vld  => s_idata_vld,
      o_din_rdy  => open,
      o_dout     => s_q_out,
      o_dout_vld => open,
      i_dout_rdy => s_odata_rdy
      );

  -- Message types which do not produce samples are present on input
  s_non_samples_msg_on_input <= in_in.ready and not s_idata_vld;
  
  -- Enable primitive output when output is allowed and sample producing
  -- messages present on input
  s_odata_rdy <= out_in.ready and s_idata_vld;
  
  -- For interval, give when output is allowed and computation is valid.
  -- For flush, give when output is allowed, primitive is valid or flush is complete.
  -- When primitive output is allowed, give when primitive is valid and group
  -- delay has passed
  -- Otherwise, give when input is present and output is allowed 
  out_out.give <= out_in.ready and (s_interval_vld or s_interval_eom)  when its(s_interval_opcode) else
                  out_in.ready and (s_odata_vld or s_group_delay_zero) when its(s_flush_opcode) else
                  s_odata_vld and s_group_delay_max                    when its(s_odata_rdy) else
                  out_in.ready and in_in.ready;
  
  -- For interval, valid when output is allowed and computation is valid.
  -- For flush, valid when output is allowed and primitive is valid
  -- When primitive output is allowed, give when primitive is valid and group
  -- delay has passed
  -- Otherwise, give when input is valid and there is not a samples message in
  -- progress
  out_out.valid <= out_in.ready and (s_interval_vld or s_interval_eom) when its(s_interval_opcode) else
                   out_in.ready and s_odata_vld                        when its(s_flush_opcode) else
                   s_odata_vld and s_group_delay_max                   when its(s_odata_rdy) else
                   in_in.valid and not s_samples_msg_in_progress;
  
  -- When primitive output is enabled or samples message in progress
  -- Otherwise: pass along along opcode
  out_out.opcode <= ComplexShortWithMetadata_samples_op_e when s_odata_rdy or s_samples_msg_in_progress
                    else in_in.opcode;
  
  -- When samples messages are in progress: EOM when samples message ending
  -- When flushing: EOM when samples message ending
  -- When interval messages are in progress: EOM when interval processing complete
  -- Otherwise: pass along EOM
  out_out.eom <= s_samples_eom  when its(s_samples_msg_in_progress) else
                 s_samples_eom  when its(s_flush_opcode) and not its(s_group_delay_zero) else
                 s_interval_eom when its(s_interval_opcode) else
                 in_in.eom;
  
  -- For powers of 2, division is a shift
  r_is_power_of_2 : if c_count_ones_in_R = 1 generate
    s_interval_data <= std_logic_vector(unsigned(in_in.data) srl integer(ceil(log2(real(c_R)))));
    s_interval_eom  <= in_in.eom;
    s_interval_vld  <= in_in.valid;
  end generate;

  -- Non-powers of 2, perform interval division
  r_is_not_power_of_2 : if c_count_ones_in_R /= 1 generate
    constant c_interval_op_width : integer := ulonglong_t'length;
    signal s_din, s_dout         : unsigned(c_interval_op_width-1 downto 0);  
    signal s_interval_in_msb     : unsigned(ocpi_port_in_data_width-1 downto 0);  
    signal s_din_vld, s_din_rdy  : std_logic;  
  begin
    interval_in_r : process (ctl_in.clk)
    begin
      if rising_edge(ctl_in.clk) then
        if its(ctl_in.reset) then
          s_interval_in_msb <= (others => '0');
        elsif its(s_interval_opcode) and its(in_in.som) then
          s_interval_in_msb(ocpi_port_in_data_width-1 downto 0) <= unsigned(in_in.data);
        end if;
      end if;
    end process;

    s_din     <= s_interval_in_msb & unsigned(in_in.data);
    s_din_vld <= s_din_rdy and s_interval_opcode and in_in.eom;
      
    dbc: util_prims.util_prims.divide_by_constant
      generic map(
        g_const      => c_r,
        g_num_bits   => r'length,
        g_data_width => c_interval_op_width
        )
      port map(
        i_clk      => ctl_in.clk,
        i_rst      => ctl_in.reset,
        i_din      => s_din,
        i_din_vld  => s_din_vld,
        o_din_rdy  => s_din_rdy,
        o_dout     => s_dout,
        o_dout_vld => s_interval_vld,
        i_dout_rdy => out_in.ready
      );
    
    interval_eom_r : process (ctl_in.clk)
    begin
      if rising_edge(ctl_in.clk) then
        if its(ctl_in.reset) then
          s_interval_eom <= '0';
        elsif its(out_in.ready) then
          s_interval_eom <= s_interval_vld;
        end if;
      end if;
    end process;
    
    s_interval_data <= std_logic_vector(s_dout(ocpi_port_in_data_width-1 downto 0)) when its(s_interval_eom) else
                       std_logic_vector(s_dout(c_interval_op_width-1 downto c_interval_op_width-ocpi_port_in_data_width));
  end generate;

  out_out.data   <= std_logic_vector(resize(signed(s_q_out), 16)) &
                    std_logic_vector(resize(signed(s_i_out), 16))   when (its(s_odata_vld) and its(out_in.ready)) or its(s_samples_msg_in_progress) else
                    s_interval_data                                 when its(s_interval_opcode) else
                    in_in.data;

  -- To properly assert EOMs for samples opcodes, keep track of messages in
  -- progress, which resets when non-samples messages appear on input
  msg_in_progress : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) or its(s_samples_eom) then
        s_samples_msg_in_progress <= '0';
      elsif its(s_odata_vld) and its(out_in.ready) then
        s_samples_msg_in_progress <= '1';
      end if;
    end if;
  end process;

  s_samples_eom <= out_in.ready and s_non_samples_msg_on_input;

  groupDelayCounterProc : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        s_group_delay_counter <= (others => '0');
      elsif its(s_idata_vld) and its(s_idata_rdy) then
        --Decrement group delay counter when flushing
        if its(s_flush_opcode) and s_group_delay_counter > 0 then
          s_group_delay_counter <= s_group_delay_counter - 1;
        --Increment group delay counter when taking samples data
        elsif its(s_samples_opcode) and s_group_delay_counter <= c_group_delay then
          s_group_delay_counter <= s_group_delay_counter + 1;
        end if;
      end if;
    end if;
  end process;
  s_group_delay_max  <= to_bool(s_group_delay_counter > c_group_delay);
  s_group_delay_zero <= to_bool(s_group_delay_counter = 0);

  -- Peak Detection primitive. Value is cleared when read
  s_peak_rst_in <= ctl_in.reset or std_logic(props_in.peak_read);
  s_peak_a_in   <= std_logic_vector(resize(signed(s_i_out),16));
  s_peak_b_in   <= std_logic_vector(resize(signed(s_q_out),16));

  pm_gen : if its(PEAK_MONITOR) generate
    pd : util_prims.util_prims.peakDetect
      port map (
        CLK_IN   => ctl_in.clk,
        RST_IN   => s_peak_rst_in,
        EN_IN    => s_odata_vld,
        A_IN     => s_peak_a_in,
        B_IN     => s_peak_b_in,
        PEAK_OUT => s_peak_out);

    props_out.peak <= signed(s_peak_out);
  end generate pm_gen;

  no_pm_gen : if its(not PEAK_MONITOR) generate 
    props_out.peak <= (others => '0'); 
  end generate no_pm_gen;

end rtl;
