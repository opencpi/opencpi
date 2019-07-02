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
  signal s_idata_vld               : std_logic;
  signal s_odata_vld               : std_logic;
  signal s_samples_opcode          : std_logic;
  signal s_interval_opcode         : std_logic;
  signal s_samples_msg_in_progress : std_logic;
  -- Peak detector
  signal s_peak_rst_in             : std_logic;
  signal s_peak_a_in               : std_logic_vector(c_data_width-1 downto 0);
  signal s_peak_b_in               : std_logic_vector(c_data_width-1 downto 0);
  signal s_peak_out                : std_logic_vector(c_data_width-1 downto 0);
  
begin
  
  s_samples_opcode  <= to_bool(in_in.opcode=ComplexShortWithMetadata_samples_op_e);
  s_interval_opcode <= to_bool(in_in.opcode=ComplexShortWithMetadata_interval_op_e);
  
  -- Enable input to primitive
  s_idata_vld <= in_in.valid and out_in.ready and s_samples_opcode;

  -- Take from input (in_out.take) when data is present and output is allowed
  s_do_work   <= in_in.ready and out_in.ready;
  in_out.take <= s_do_work;

  proc_dec_cnt : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        s_dec_cnt <= (others => '0');
      elsif its(s_idata_vld) then
        if s_dec_cnt = 0 then
          s_dec_cnt <= to_unsigned(c_r-1,s_dec_cnt'length);
        else
          s_dec_cnt <= s_dec_cnt-1;
        end if;
      end if;
    end if;
  end process;

  s_odata_vld <= to_bool(s_dec_cnt = 0) and s_idata_vld;

  -- Give to output port when data is present and output is allowed and when output of primitive is
  -- valid or SOM/EOM (for samples)
  out_out.give   <= s_do_work and (s_odata_vld or (in_in.eom and s_samples_msg_in_progress)) when its(s_samples_opcode)
                    else s_do_work;
  out_out.valid  <= s_odata_vld when its(s_samples_opcode) else in_in.valid;
  --Non-powers of 2 currently not supported. Interval data shifted
  out_out.data   <= std_logic_vector(unsigned(in_in.data) srl integer(log2(real(c_r)))) when its(s_interval_opcode) else
                    in_in.data;
  out_out.opcode <= in_in.opcode;
  out_out.eom    <= in_in.eom and s_samples_msg_in_progress when its(s_samples_opcode) else in_in.eom;

  --To avoid producing ZLMs on the samples opcode, keep track of messages in
  --progress, which resets on EOM
  msg_in_progress : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        s_samples_msg_in_progress <= '0';
      elsif its(in_in.eom) and its(s_samples_opcode) then
        s_samples_msg_in_progress <= '0';
      elsif its(s_odata_vld) then
        s_samples_msg_in_progress <= '1';
      end if;
    end if;
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
