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

  constant R_c                   : integer  := to_integer(unsigned(R));
  constant DATA_WIDTH_c          : positive := to_integer(DATA_WIDTH);

  signal do_work                 : std_logic;
  signal dec_cnt                 : ushort_t;
  signal idata_vld               : std_logic;
  signal odata_vld               : std_logic;
  signal samples_opcode          : std_logic;
  signal interval_opcode         : std_logic;
  signal samples_msg_in_progress : std_logic;
  -- Peak detector
  signal peak_rst_in : std_logic;
  signal peak_a_in   : std_logic_vector(15 downto 0);
  signal peak_b_in   : std_logic_vector(15 downto 0);
  signal peak_out    : std_logic_vector(15 downto 0);
  
begin
  
  samples_opcode <= to_bool(in_in.opcode=ComplexShortWithMetadata_samples_op_e);
  interval_opcode <= to_bool(in_in.opcode=ComplexShortWithMetadata_interval_op_e);
  
  -- Enable input to primitive (idata_vld)
  idata_vld   <= in_in.valid and out_in.ready and samples_opcode;

  -- Take from input (in_out.take) when data is present and output is allowed
  do_work     <= in_in.ready and out_in.ready;
  in_out.take <= do_work;

  proc_dec_cnt : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) or its(not ctl_in.is_operating) then
        dec_cnt <= to_unsigned(R_c-1,ushort_t'length); --Initialized to R-1 to give first sample
      elsif its(idata_vld) then
        if dec_cnt = R_c-1 then
          dec_cnt <= (others => '0');
        else
          dec_cnt <= dec_cnt+1;
        end if;
      end if;
    end if;
  end process;

  odata_vld <= to_bool(dec_cnt = R_c-1) and idata_vld;

  -- Give to output port when data is present and output is allowed and when output of primitive is
  -- valid or SOM/EOM (for samples)
  out_out.give   <= do_work and (odata_vld or (in_in.eom and samples_msg_in_progress)) when its(samples_opcode) else do_work;
  out_out.valid  <= odata_vld when its(samples_opcode) else in_in.valid;
  --Non-powers of 2 currently not supported. Interval data shifted
  out_out.data   <= std_logic_vector(unsigned(in_in.data) srl integer(log2(real(R_c)))) when its(interval_opcode) else
                    in_in.data;
  out_out.opcode <= in_in.opcode;
  out_out.eom    <= in_in.eom and samples_msg_in_progress when its(samples_opcode) else in_in.eom;

  --To avoid producing ZLMs on the samples opcode, keep track of messages in
  --progress, which resets on EOM
  msg_in_progress : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        samples_msg_in_progress <= '0';
      elsif its(in_in.eom) and its(samples_opcode) then
        samples_msg_in_progress <= '0';
      elsif its(odata_vld) then
        samples_msg_in_progress <= '1';
      end if;
    end if;
  end process;

    -- Peak Detection primitive. Value is cleared when read
  peak_rst_in <= ctl_in.reset or std_logic(props_in.peak_read);
  peak_a_in   <= in_in.data(DATA_WIDTH_c-1 downto 0);
  peak_b_in   <= in_in.data(DATA_WIDTH_c-1+16 downto 16);

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
