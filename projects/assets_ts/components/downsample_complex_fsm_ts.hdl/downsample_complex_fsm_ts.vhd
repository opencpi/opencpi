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

--Decreases the sample rate of x by keeping the first sample and then every Rth sample after the first.
library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions

architecture rtl of worker is

  constant c_r                     : integer  := to_integer(R);
  constant c_data_width            : positive := to_integer(DATA_WIDTH);

  signal s_do_work                 : std_logic;
  signal s_dec_cnt                 : ushort_t;
  signal s_odata_vld               : std_logic;
  signal s_samples_opcode          : std_logic;
  signal s_samples_msg_in_progress : std_logic;
  signal s_interval_opcode         : std_logic;
  signal s_counter_en              : bool_t;
  -- Peak detector
  signal s_peak_rst_in             : std_logic;
  signal s_peak_a_in               : std_logic_vector(c_data_width-1 downto 0);
  signal s_peak_b_in               : std_logic_vector(c_data_width-1 downto 0);
  signal s_peak_out                : std_logic_vector(c_data_width-1 downto 0);
  
  type state_type is (idle_s, samples_s, interval_s, bypass_s);

  signal current_state : state_type;

begin
  
  s_samples_opcode  <= to_bool(in_in.opcode=ComplexShortWithMetadata_samples_op_e);
  s_interval_opcode <= to_bool(in_in.opcode=ComplexShortWithMetadata_interval_op_e);

  s_do_work   <= in_in.ready and out_in.ready;

  seq: process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        s_dec_cnt <= (others => '0');
        s_samples_msg_in_progress <= bfalse;
      elsif its(s_do_work) then
        --To avoid producing ZLMs on the samples opcode, keep track of messages in
        --progress, which resets on EOM
        if its(in_in.eom) and its(s_samples_opcode) then
          s_samples_msg_in_progress <= bfalse;
        elsif its(s_odata_vld) then
          s_samples_msg_in_progress <= btrue;
        end if;
        --Counter to track decimation
        if its(s_counter_en) then
          if s_dec_cnt = 0 then
            s_dec_cnt <= to_unsigned(c_r-1,s_dec_cnt'length);
          else
            s_dec_cnt <= s_dec_cnt-1;
          end if;
        end if;
      end if;
    end if;
  end process;

  --State can be computed combinatorially
  current_state <= idle_s     when its(not s_do_work) else
                   samples_s  when its(s_samples_opcode) else
                   interval_s when its(s_interval_opcode) else
                   bypass_s;

  s_odata_vld <= to_bool(s_dec_cnt = 0) and s_counter_en;

  fsm_comb : process(current_state, in_in.valid, s_do_work, in_in.eom,
                     s_samples_msg_in_progress, s_odata_vld, in_in.data)
  begin
    -- FSM Output Defaults
    in_out.take    <= s_do_work;
    out_out.give   <= s_do_work;
    out_out.valid  <= in_in.valid;
    out_out.data   <= in_in.data;
    out_out.eom    <= in_in.eom;
    out_out.opcode <= in_in.opcode;
    s_counter_en   <= bfalse;
    
    -- Output Assertions
    case (current_state) is
      
      when idle_s =>
        in_out.take   <= bfalse;
        out_out.give  <= bfalse;
        out_out.valid <= bfalse;
        
      when samples_s =>
        s_counter_en  <= in_in.valid;
        out_out.give  <= s_do_work and ((in_in.eom and s_samples_msg_in_progress) or s_odata_vld);
        out_out.valid <= s_odata_vld;
          
      when interval_s =>
        out_out.data <= std_logic_vector(unsigned(in_in.data) srl integer(log2(real(c_r))));

      when bypass_s =>
        -- Outputs are default
        
    end case;
  end process;

  -- Peak Detection primitive. Value is cleared when read
  s_peak_rst_in <= ctl_in.reset or std_logic(props_in.peak_read);
  s_peak_a_in   <= in_in.data(c_data_width-1 downto 0);
  s_peak_b_in   <= in_in.data(c_data_width-1+16 downto 16);

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
