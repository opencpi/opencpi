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

-- THIS FILE WAS ORIGINALLY GENERATED ON Wed Mar 29 10:45:49 2017 EDT
-- BASED ON THE FILE: ad9361_dac.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: ad9361_dac

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
use ocpi.util.all; -- needed for slv()
library util; use util.util.all;
library misc_prims; use misc_prims.misc_prims.all;
architecture rtl of worker is
  constant dac_width : positive := 12;
  -- FIFO parameters
  constant fifo_width : positive := (dac_width*2); -- the fifo is just wide enough to feed AD9361 DAC

  -- mandatory input port logic
  signal event_in_out_take            : std_logic := '0';
  signal ready_for_event_in_port_data : std_logic := '0';

  -- CTL/WSI Clock domain signals
  signal in_I       : std_logic_vector(15 downto 0);
  signal in_Q       : std_logic_vector(15 downto 0);
  signal wsi_data_I : std_logic_vector(dac_width- 1 downto 0);
  signal wsi_data_Q : std_logic_vector(dac_width- 1 downto 0);
  signal wsi_data   : std_logic_vector(fifo_width - 1 downto 0);
  signal wsi_event_in_opcode_on_off : std_logic := '0';
  signal event_in_clk               : std_logic := '0';
  signal event_in_reset             : std_logic := '0';
  -- AD9361 TX clock domain signals
  signal dac_data : std_logic_vector(fifo_width - 1 downto 0);
  signal dac_ready : std_logic := '0';
  signal dac_data_I : std_logic_vector(dac_width - 1 downto 0);
  signal dac_data_Q : std_logic_vector(dac_width - 1 downto 0);
begin
  --TODO: Replace ctl clock with port clock once AV-65 is resolved
  event_in_clk <= ctl_in.clk;
  event_in_reset <= ctl_in.reset;

  idata_width_32 : if IDATA_WIDTH_p = 32 generate

    -- iqstream w/ DataWidth=32 formats Q in most significant bits, I in least
    -- significant (see OpenCPI_HDL_Development section on Message Payloads vs.
    -- Physical Data Width on Data Interfaces)
    in_Q <= in_in.data(31 downto 16);
    in_I <= in_in.data(15 downto 0);

  end generate idata_width_32;

  -- transform signed Q0.15 to signed Q0.11, taking most significant 12 bits
  -- (and using the 13th bit to round)
  trunc_round_Q : entity work.trunc_round_16_to_12_signed
    port map(
      DIN  => in_Q,
      DOUT => wsi_data_Q);
  trunc_round_I : entity work.trunc_round_16_to_12_signed
    port map(
      DIN  => in_I,
      DOUT => wsi_data_I);

  wsi_data <= wsi_data_Q & wsi_data_I;

  fifo : util.util.dac_fifo
    generic map(width     => fifo_width,
                depth     => to_integer(fifo_depth))
    port map   (clk       => ctl_in.clk,
                reset     => ctl_in.reset,
                operating => ctl_in.is_operating,
                wsi_ready => in_in.ready,
                wsi_valid => in_in.valid,
                wsi_data  => wsi_data,
                clear     => props_in.underrun_written,
                wsi_take  => in_out.take,
                underrun  => props_out.underrun,
                dac_clk   => dev_dac_in.dac_clk,
                dac_reset => open,
                dac_take  => dev_dac_in.dac_take,
                dac_ready => dac_ready,
                dac_data  => dac_data);

  dev_dac_out.dac_ready <= dac_ready;

  dac_data_Q <= dac_data((dac_width*2)-1 downto dac_width);
  dac_data_I <= dac_data(dac_width-1     downto 0        );

  -- TX zeros when starved for input data
  dev_dac_out.dac_data_Q <= dac_data_Q when dac_ready='1' else (others => '0');
  dev_dac_out.dac_data_I <= dac_data_I when dac_ready='1' else (others => '0');

  dev_dac_out.present <= '1';

  -- mandatory input port logic, (note that
  -- ready_for_event_in_port_data MUST have the same clock latency as the latency
  -- between in port's data and any associated output port's data)
  event_in_out.take <= event_in_out_take;
  event_in_out_take <= ctl_in.is_operating and event_in_in.ready and
                       ready_for_event_in_port_data; -- this applies backpressure

  -- this is necessary because we have to synchronize tx events from control
  -- plane clock domain to AD9361 FB_CLK domain (was decided to use two
  -- flip-flop synchronizer, which necessitates ensuring tx events only occur at
  -- most at 1.5X the DAC (AD9361 FB_CLK) clock rate
  counter_to_space_out_tx_events : process(ctl_in.clk)
    -- props.in-min_num_cp_clks_per_txen_event is UShort_t
    constant init_val : UShort_t := to_ushort(1);
    variable count : UShort_t := to_ushort(1);
  begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
        count := init_val;
        ready_for_event_in_port_data <= '1';
      elsif count < props_in.min_num_cp_clks_per_txen_event then
        if count = init_val then
          if event_in_in.ready = '1' then
            count := count + 1;

            -- applies backpressure to event_in
            ready_for_event_in_port_data <= '0';

          end if;
        else -- count > 1
          count := count + 1;
        end if;
      else -- count >= props_in.min_num_cp_clks_per_txen_event
        -- note this will be always be reached when
        -- props_in.min_num_cp_clks_per_txen_event is 0 or 1
        count := init_val;
        ready_for_event_in_port_data <= '1';
      end if;
    end if;
  end process counter_to_space_out_tx_events;

  wsi_event_in_opcode_on_off <= '1' when (event_in_in.opcode = tx_event_txOn_op_e) else '0';

  event_in_to_txen : misc_prims.misc_prims.event_in_to_txen
    port map (event_in_clk           => event_in_clk,
              event_in_reset         => event_in_reset,
              ctl_in_is_operating    => ctl_in.is_operating,
              event_in_in_reset      => event_in_in.reset,
              event_in_in_som        => '1', -- event_in_in.som,
              event_in_in_valid      => '0', -- event_in_in.valid,
              event_in_in_eom        => '1', -- event_in_in.eom,
              event_in_in_ready      => event_in_in.ready,
              event_in_out_take      => event_in_out_take,
              event_in_opcode_on_off => wsi_event_in_opcode_on_off,
              txon_pulse             => dev_tx_event_out.txon_pulse,
              txoff_pulse            => dev_tx_event_out.txoff_pulse,
              txen                   => open,
              event_in_connected     => dev_tx_event_out.event_in_connected,
              is_operating           => dev_tx_event_out.is_operating);

end rtl;
