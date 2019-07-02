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
  -- mandatory output port logic
  signal data_ready_for_out_port : std_logic := '0';
  signal out_meta_is_reserved    : std_logic := '0';
  signal out_som                 : std_logic := '0';
  signal out_eom                 : std_logic := '0';
  signal out_valid               : std_logic := '0';

  signal som_data   : std_logic := '0';
  signal eom_data   : std_logic := '0';
  signal valid_data : std_logic := '0';

  signal enable        : std_logic := '0';
  signal data_count    : std_logic_vector(WIDTH-1 downto 0) := (others => '0');
  signal data_count_32 : std_logic_vector(31 downto 0)      := (others => '0');
  signal data_walking      : std_logic_vector(WIDTH-1 downto 0) := (others => '0');
  signal data_lfsr         : std_logic_vector(WIDTH-1 downto 0) := (others => '0');
  signal data_lfsr_ordered : std_logic_vector(WIDTH-1 downto 0) := (others => '0');
  signal data_lfsr_rev     : std_logic_vector(WIDTH-1 downto 0) := (others => '0');
  signal data              : std_logic_vector(WIDTH-1 downto 0) := (others => '0');

  signal data_Q : std_logic_vector(15 downto 0) := (others => '0');
  signal data_I : std_logic_vector(15 downto 0) := (others => '0');
  signal out_Q  : std_logic_vector(15 downto 0) := (others => '0');
  signal out_I  : std_logic_vector(15 downto 0) := (others => '0');

  signal message_size_samples : ulong_t := (others => '0');
  signal msg_samp_ctr         : ulong_t := (0 => '1', others => '0');

  signal num_samples_valid : std_logic := '0';

  signal zlm_req                 : std_logic := '0'; -- ZLM request
  signal zlm_req_once            : std_logic := '0';
  signal zlm_req_once_r          : std_logic := '0';
  signal zlm_req_once_sent       : std_logic := '0';
  signal zlm_req_once_clr_sticky : std_logic := '0';
begin
  ------------------------------------------------------------------------------
  -- out port
  ------------------------------------------------------------------------------

  -- mandatory output port logic, (note that
  -- data_ready_for_out_port MUST be clock-aligned with out_out.data)
  out_out.give <= ctl_in.is_operating and out_in.ready and
                  (not out_meta_is_reserved) and data_ready_for_out_port;
  out_meta_is_reserved <= (not out_som) and (not out_valid) and (not out_eom);
  out_out.som   <= out_som;
  out_out.eom   <= out_eom;
  out_out.valid <= out_valid;

  data_ready_for_out_port <= enable or zlm_req_once_sent;

  odata_width_32 : if ODATA_WIDTH_p = 32 generate

    -- iqstream w/ DataWidth=32 formats Q in most significant bits, I in least
    -- significant (see OpenCPI_HDL_Development section on Message Payloads vs.
    -- Physical Data Width on Data Interfaces)
    out_out.data <= out_Q & out_I;

  end generate odata_width_32;

  out_port_inject_zlm_i : entity work.out_port_inject_zlm
    port map (
      clk          => ctl_in.clk,
      reset        => ctl_in.reset,
      zlm_req      => zlm_req_once,
      som_i        => som_data,
      eom_i        => eom_data,
      valid_i      => valid_data,
      out_in_ready => out_in.ready, -- enforces backpressure
      zlm_req_sent => zlm_req_once_sent,
      som_o        => out_som,    -- does obey backpressure (for injected ZLMs)
      eom_o        => out_eom,    -- does obey backpressure (for injected ZLMs)
      valid_o      => out_valid); -- does obey backpressure (for injected ZLMs)

  out_out.byte_enable <= (others => '1');

  zlm_req_once_regs : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
        zlm_req_once_r <= '0';
        zlm_req_once_clr_sticky <= '0';
      else
        zlm_req_once_r <= zlm_req_once;
        zlm_req_once_clr_sticky <= zlm_req_once_sent or zlm_req_once_clr_sticky;
      end if;
    end if;
  end process;

  -- will send only one ZLM
  zlm_req_once <= (zlm_req or zlm_req_once_r) and (not zlm_req_once_clr_sticky);

  som_data   <= to_bool(msg_samp_ctr = 1);
  eom_data   <= to_bool(msg_samp_ctr = message_size_samples) or
                (num_samples_valid and
                to_bool(msg_samp_ctr = unsigned(props_in.num_samples)));
  valid_data <= ctl_in.is_operating and out_in.ready and
                data_ready_for_out_port;

  num_samples_valid_gen : util_prims.util_prims.set_clr
    port map(
      clk => ctl_in.clk,
      rst => ctl_in.reset,
      set => props_in.num_samples_written,
      clr => '0',
      q   => num_samples_valid,
      q_r => open);

  -- from the component data sheet: "Data_Src selects one DATA_BIT_WIDTH_p
  -- bits-wide data bus from multiple data generation sources, packs the
  -- DATA_BIT_WIDTH_p bits in bit-forward order in the least significant bits of
  -- the I data bus and bit-reverse order in the most significant bits of the Q
  -- data bus"
  data_I(15 downto 15-WIDTH+1) <= data;
  data_I(15-WIDTH downto 0) <= (others => '0');
  data_rev : for idx in 0 to WIDTH-1 generate
    data_Q(15-idx) <= data(idx);
  end generate;
  data_Q(15-WIDTH downto 0) <= (others => '0');

  enable <= ctl_in.is_operating and out_in.ready and props_in.enable and
            (num_samples_valid and
             (to_bool(props_in.num_samples = -1) or
              to_bool(long_t(data_count_32) < props_in.num_samples)));

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
  message_size_samples <=
    props_in.messageSize_bytes srl 2 when props_in.messageSize_bytes /= 0 else
    resize(props_in.ocpi_buffer_size_out srl 2, message_size_samples'length);
  msg_counter : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
        msg_samp_ctr <= (0 => '1', others => '0');
      elsif enable = '1' then
        if eom_data = '1' then
          msg_samp_ctr <= (0 => '1', others => '0');
        else
          msg_samp_ctr <= msg_samp_ctr + 1;
        end if;
      end if;
    end if;
  end process msg_counter;

  ------------------------------------------------------------------------------
  -- mask_I, mask_Q properties
  ------------------------------------------------------------------------------

  out_Q <= data_Q and std_logic_vector(props_in.mask_Q);
  out_I <= data_I and std_logic_vector(props_in.mask_I);

  ------------------------------------------------------------------------------
  -- ZLM_WHEN_NUM_SAMPLES_REACHED_p parameter property
  ------------------------------------------------------------------------------

  -- does not obey backpressure, will send multiple ZLMs if used directly
  zlm_req <= num_samples_valid and
             to_bool(props_in.num_samples /= -1) and
             to_bool(long_t(data_count_32) >= (props_in.num_samples))
             when (ZLM_WHEN_NUM_SAMPLES_REACHED_p = btrue) else '0';

  ------------------------------------------------------------------------------
  -- mode property
  ------------------------------------------------------------------------------

  with props_in.mode select
    data <= data_count           when count_e,
            data_walking         when walking_e,
            data_lfsr_ordered    when lfsr_e,
            std_logic_vector(props_in.fixed_value) when others;

  ------------------------------------------------------------------------------
  -- LFSR_bit_reverse property
  ------------------------------------------------------------------------------

  data_lfsr_ordered <= data_lfsr_rev when (props_in.LFSR_bit_reverse = btrue)
                       else data_lfsr;

end rtl;
