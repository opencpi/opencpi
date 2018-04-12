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

-- THIS FILE WAS ORIGINALLY GENERATED ON Fri May  5 14:27:15 2017 EDT
-- BASED ON THE FILE: fifo.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: fifo

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library bsv; use bsv.bsv.all;
architecture rtl of fifo_worker is

  -- mandatory input port logic, don't touch this
  signal ready_for_in_port_data : std_logic := '0';

  -- mandatory output port logic, don't touch this
  signal data_ready_for_out_port : std_logic := '0';

  signal oneshot_and_done_enq_d : std_logic := '0';
  signal oneshot_and_done_deq_d : std_logic := '0';
  signal fifo_reset_n   : std_logic := '0';
  signal fifo_enq       : std_logic := '0';
  signal fifo_deq       : std_logic := '0';
  signal fifo_d_in      : std_logic_vector(35 downto 0) := (others => '0');
  signal fifo_d_out     : std_logic_vector(35 downto 0) := (others => '0');
  signal fifo_full_n    : std_logic := '0';
  signal fifo_empty_n   : std_logic := '0';

  -- ZLM needed for simulation to stop
  signal force_zlm          : std_logic := '0';
  signal nsamps_enq_count_d : unsigned(FIFO_DEPTH_p'length-1 downto 0)
                            := (others => '0');
  signal nsamps_deq_count_d : unsigned(FIFO_DEPTH_p'length-1 downto 0)
                            := (others => '0');

  signal ctl_in_reset_n : std_logic := '0';
begin
  -- mandatory input port logic, don't touch this, (note that
  -- ready_for_in_port_data MUST have the same clock latency as the latency
  -- between in port's data and any associated output port's data)
  in_out.take <= ctl_in.is_operating and in_in.ready and
                 ready_for_in_port_data; -- this applies backpressure

  -- mandatory output port logic, don't touch this, (note that
  -- data_ready_for_out_port MUST be clock-aligned with out_out.data)
  out_out.give <= ctl_in.is_operating and out_in.ready and
                  data_ready_for_out_port;

  oneshot_and_done_enq_d <= '1' when (props_in.oneshot = btrue) and
                            (nsamps_enq_count_d = FIFO_DEPTH_p) else '0';
  oneshot_and_done_deq_d <= '1' when (props_in.oneshot = btrue) and
                            (nsamps_deq_count_d = FIFO_DEPTH_p) else '0';
  fifo_enq <= ctl_in.is_operating and in_in.ready and fifo_full_n;
  fifo_deq <= ctl_in.is_operating and out_in.ready and
              fifo_empty_n and (not oneshot_and_done_deq_d);
  fifo_d_in <= in_in.data & in_in.som & in_in.eom &
               in_in.valid & in_in.byte_enable;

  ready_for_in_port_data <= fifo_full_n or oneshot_and_done_enq_d;

  data_ready_for_out_port <= fifo_deq or force_zlm;

  nsamps_enq_counter : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
        nsamps_enq_count_d <= (others => '0');
      elsif ctl_in.is_operating = '1' then
        if (fifo_enq = '1') and (nsamps_enq_count_d < FIFO_DEPTH_p) then
          nsamps_enq_count_d <= nsamps_enq_count_d + 1;
        end if;
      end if;
    end if;
  end process;

  nsamps_deq_counter : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
        nsamps_deq_count_d <= (others => '0');
      elsif ctl_in.is_operating = '1' then
        if (fifo_deq = '1') and (nsamps_deq_count_d < FIFO_DEPTH_p) then
          nsamps_deq_count_d <= nsamps_deq_count_d + 1;
        end if;
      end if;
    end if;
  end process;

  ctl_in_reset_n <= not ctl_in.reset;
  fifo_reset_n <= ctl_in_reset_n and (not oneshot_and_done_deq_d);

  rx_data_fifo : bsv.bsv.SizedFIFO
    generic map (
      p1width      => 36, -- FIFO data bus width
      p2depth      => to_integer(FIFO_DEPTH_p),
      p3cntr_width => integer(ceil(log2(real(to_integer(FIFO_DEPTH_p)-1)))))
    port map (
      CLK          => ctl_in.clk,
      RST          => fifo_reset_n, -- RST is active low apparently
      D_IN         => fifo_d_in,
      ENQ          => fifo_enq,
      FULL_N       => fifo_full_n,
      D_OUT        => fifo_d_out,
      DEQ          => fifo_deq,
      EMPTY_N      => fifo_empty_n,
      CLR          => ctl_in.reset);

  -- ZLM only needed for simulation to stop
  force_zlm <= oneshot_and_done_deq_d when (ZLM_WHEN_ONESHOT_DONE_p = btrue)
               else '0';

  out_out.data            <= fifo_d_out(35 downto 4);
  out_out.som             <= fifo_d_out(3) or force_zlm;
  out_out.eom             <= fifo_d_out(2) or oneshot_and_done_enq_d
                                           or force_zlm;
  out_out.valid           <= fifo_d_out(1) and (not force_zlm);
  out_out.byte_enable     <= (others => fifo_d_out(0));

end rtl;
