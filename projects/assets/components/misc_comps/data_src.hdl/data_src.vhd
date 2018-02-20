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

-- THIS FILE WAS ORIGINALLY GENERATED ON Fri May  5 16:02:04 2017 EDT
-- BASED ON THE FILE: data_src.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: data_src

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of data_src_worker is
  constant WIDTH      : integer := to_integer(DATA_BIT_WIDTH_p);
  -- mandatory output port logic, don't touch this
  signal data_ready_for_out_port : std_logic := '0';

  signal enable        : std_logic := '0';
  signal data_count    : std_logic_vector(WIDTH-1 downto 0) := (others => '0');
  signal data_count_32 : std_logic_vector(31 downto 0)      := (others => '0');
  signal data_walking  : std_logic_vector(WIDTH-1 downto 0) := (others => '0');
  signal data_lfsr     : std_logic_vector(WIDTH-1 downto 0) := (others => '0');
  signal data_lfsr_ordered : std_logic_vector(WIDTH-1 downto 0) := (others => '0');
  signal data_lfsr_rev : std_logic_vector(WIDTH-1 downto 0) := (others => '0');
  signal data          : std_logic_vector(WIDTH-1 downto 0) := (others => '0');
  signal data_I        : std_logic_vector(15 downto 0)      := (others => '0');
  signal data_Q        : std_logic_vector(15 downto 0)      := (others => '0');

  signal message_size_samples  : ulong_t := (others => '0');
  signal msg_samp_ctr : ulong_t := (0 => '1', others => '0');
  signal eom          : std_logic := '0';

  signal force_zlm    : std_logic := '0';
  signal force_zlm_sticky_r : std_logic := '0';
begin
  -- mandatory output port logic, don't touch this, (note that
  -- data_ready_for_out_port MUST be clock-aligned with out_out.data)
  out_out.give <= ctl_in.is_operating and out_in.ready and
                  data_ready_for_out_port;

  data_ready_for_out_port <= enable or force_zlm;

  force_zlm <= to_bool(props_in.num_samples /= -1) and
               to_bool(long_t(data_count_32) >= (props_in.num_samples)) and
               to_bool(force_zlm_sticky_r = '0') when
               (ZLM_WHEN_NUM_SAMPLES_REACHED_p = btrue) else '0';

  force_zlm_sticky_reg : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
        force_zlm_sticky_r <= '0';
      elsif ctl_in.is_operating = '1' then
        if force_zlm = '1' then
          force_zlm_sticky_r <= '1';
        end if;
      end if;
    end if;
  end process;

  out_out.valid           <= ctl_in.is_operating and out_in.ready and
                             data_ready_for_out_port and (not force_zlm);
  out_out.byte_enable     <= (others => '1');
  out_out.som <= to_bool(msg_samp_ctr = 1) or force_zlm;
  out_out.eom <= eom or force_zlm;
  eom <= to_bool(msg_samp_ctr = message_size_samples) or
         to_bool(msg_samp_ctr = unsigned(props_in.num_samples));

  with props_in.mode select
    data <= data_count           when count_e,
            data_walking         when walking_e,
            data_lfsr_ordered    when lfsr_e,
            std_logic_vector(props_in.fixed_value) when others;

  data_lfsr_ordered <= data_lfsr_rev when (props_in.LFSR_bit_reverse = btrue) else data_lfsr;

  data_I(15 downto 15-WIDTH+1) <= data;
  data_I(15-WIDTH downto 0) <= (others => '0');

  data_rev : for idx in 0 to WIDTH-1 generate
    data_Q(15-idx) <= data(idx);
  end generate;
  data_Q(15-WIDTH downto 0) <= (others => '0');

  out_out.data <= data_Q & data_I;

  enable <= ctl_in.is_operating and out_in.ready and props_in.enable and
            (to_bool(props_in.num_samples = -1) or
             to_bool(long_t(data_count_32) < props_in.num_samples));

  lfsr : misc_prims.misc_prims.lfsr
    generic map (
      POLYNOMIAL => std_logic_vector(LFSR_POLYNOMIAL_p),
      SEED       => std_logic_vector(LFSR_SEED_p))
    port map (
      CLK        => ctl_in.clk,
      RST        => ctl_in.reset,
      EN         => enable,
      REG        => data_lfsr);

  lfsr_rev : for idx in 0 to WIDTH-1 generate
    data_lfsr_rev(WIDTH-1-idx) <= data_lfsr(idx);
  end generate;

  data_regs : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
        data_count_32 <= (others => '0');
        data_count <= (others => '0');
        data_walking(data_walking'left) <= '1';
        data_walking(data_walking'left-1 downto 0) <= (others => '0');
      elsif ctl_in.is_operating = '1' then
        if enable = '1' then
          data_count_32 <= std_logic_vector(signed(data_count_32) + 1);
          data_count <= std_logic_vector(unsigned(data_count) + 1);
          data_walking <= data_walking(0) &
                          data_walking(data_walking'length-1 downto 1);
        end if;
      end if;
    end if;
  end process data_regs;

  -- message size in samples = message size in bytes divided by (4 bytes in a
  -- 32-bit output bus)
  message_size_samples <= (props_in.messageSize_bytes srl 2);

  msg_counter : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset or eom then
        msg_samp_ctr <= (0 => '1', others => '0');
      elsif enable = '1' then
        msg_samp_ctr <= msg_samp_ctr + 1;
      end if;
    end if;
  end process msg_counter;

end rtl;
