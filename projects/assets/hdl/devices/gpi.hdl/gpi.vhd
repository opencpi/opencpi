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
-- GPI
-------------------------------------------------------------------------------
--
-- Description:
--
-- The GPI device worker reads GPIO pins and stores the values in the property
-- mask_data and send the values to its (optional) output port.
--
-- It's (optional) output port uses a protocol that has one opcode and contains
-- data and a mask. It sends data regardless of whether or not the worker
-- downstream is ready, so it is something that cannot be piped directly to
-- software. See gpio_protocol.xml in ocpi.core/specs/gpio_protocol.xml for
-- further details on the protocol.
--
-- For the mask_data property and the output port data, the MSW is a mask and
-- the LSW are the values read from the GPIO pins.
--
-- The mask allows knowledge of which GPIO pins changed since the previous read
-- cycle. The mask is the current data XOR the previous data.
--
-- There are parameter properties that enable/disable build-time inclusion of 3
-- different circuits.
--
-- The USE_DEBOUNCE property enables/disables build-time inclusion of a debounce
-- circuit which debounces input from mechanical switches. There is a COUNTER_WIDTH
-- property that sets width of the of the counter used by the debounce circuit.
-- The counter width is calculated by using the formula: ceil(log2(clock rate * debounce time))
-- It takes 2^COUNTER_WIDTH + 3 clock cycles until the input is ready at the
-- debounce circuit's output. If the debounce circuit is not enabled, metastability
-- flip flops are used instead. These metastability flip flops have a latency of
-- two clock cycles.
--
-- The EDGE_MODE property enables/disables build-time inclusion of an edge detector
-- circuit. The edge detector detects rising and falling edge of an input
-- on the rising edge of the clock. There is a RISING property that is used to
-- select between using the rising edge output of the edge detector or the falling
-- edge. If RISING is true the rising edge is used otherwise the falling edge is
-- used. The outputs of the edge detector have a latency of one clock cycle. The
-- output of either the debounce circuit or metastability flip flops is fed to
-- the edge detector. If the edge detector is not used, the output of debounce
-- circuit or metastability flip flops is used instead.
--
-- The USE_TOGGLE property enables/disables build-time inclusion of a toggle
-- circuit. The toggle circuit has a register that stores the result of XOR'ing
-- the output of the edge detector, debounce circuit, or metastability flip flops
-- with the current value of the register. If toggle circuit is not used the output
-- of the edge detector, debounce circuit, or metastability flops is used instead.
--
-- The output of these chain of circuits is fed to device worker's output port.
--
-- There is a parameter property called EVENT_MODE. If EVENT_MODE is set to true
-- the output port's valid signal is driven high only when there is a change in
-- the data to be sent to the output port. If it is set to false, the output port
-- valid signal is driven high and the data is valid every clock cycle.

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all; use ieee.math_real.all; use ieee.std_logic_misc.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions

architecture rtl of worker is
  -----------------------------------------------------------------------------
  -- Constants
  -----------------------------------------------------------------------------
  constant c_NUM_INPUTS    : integer := to_integer(NUM_INPUTS);
  -- The c_COUNTER_WIDTH constant below should be uncommented and be used to
  -- set the debounce circuit's COUNTER_WIDTH generic instead when AV-5635 is resolved
  --constant c_COUNTER_WIDTH : positive := positive(ceil(log2(from_double(CLK_RATE_HZ)*(from_double(DEBOUNCE_TIME_PSEC)/from_double(to_double(1000000000000.0))))));;

  signal s_gpi_q1        : std_logic_vector(c_NUM_INPUTS-1 downto 0);
  signal s_gpi_q2        : std_logic_vector(c_NUM_INPUTS-1 downto 0);
  signal s_gpi_sync      : std_logic_vector(c_NUM_INPUTS-1 downto 0);
  signal s_gpi_red       : std_logic_vector(c_NUM_INPUTS-1 downto 0);
  signal s_gpi_fed       : std_logic_vector(c_NUM_INPUTS-1 downto 0);
  signal s_gpi_ed        : std_logic_vector(c_NUM_INPUTS-1 downto 0);
  signal s_gpi_pulse     : std_logic_vector(c_NUM_INPUTS-1 downto 0);
  signal s_gpi_output    : std_logic_vector(c_NUM_INPUTS-1 downto 0);
  signal s_gpi_output_r  : std_logic_vector(c_NUM_INPUTS-1 downto 0);

begin

  input_condition_i : for ii in 0 to c_NUM_INPUTS-1 generate

    -- metastability FFs built into debounce circuit
    debounce_gen : if its(USE_DEBOUNCE) generate
      debouncer : misc_prims.misc_prims.debounce
        generic map (
          COUNTER_WIDTH => to_integer(unsigned(COUNTER_WIDTH)))
        port map (
          CLK    => ctl_in.clk,
          RST    => ctl_in.reset,
          BUTTON => gpi_pin(ii),
          RESULT => s_gpi_sync(ii));
    end generate debounce_gen;

    -- insert metastability FFs
    no_debounce_gen : if not(its(USE_DEBOUNCE)) generate
      process (ctl_in.clk)
      begin
        if rising_edge(ctl_in.clk) then
          if ctl_in.reset = '1' then
            s_gpi_q1 (ii) <= '0';
            s_gpi_q2(ii) <= '0';
          else
            s_gpi_q1 (ii) <= gpi_pin(ii);
            s_gpi_q2(ii) <= s_gpi_q1 (ii);
          end if;
        end if;
      end process;
    s_gpi_sync(ii) <= s_gpi_q2(ii);
    end generate no_debounce_gen;

    -- edge detect the input signal
    edge_detect_gen : if its(EDGE_MODE) generate
      ed : misc_prims.misc_prims.edge_detector
        port map (
          clk           => ctl_in.clk,
          reset         => ctl_in.reset,
          din           => s_gpi_sync(ii),
          rising_pulse  => s_gpi_red(ii),
          falling_pulse => s_gpi_fed(ii));
      s_gpi_ed(ii) <= s_gpi_red(ii) when its(RISING) else s_gpi_fed(ii);
    end generate edge_detect_gen;

    -- level mode
    no_edge_detect_gen : if not(its(EDGE_MODE)) generate
      s_gpi_ed(ii) <= s_gpi_sync(ii);
    end generate no_edge_detect_gen;

    -- toggle mode
    toggle_gen : if its(USE_TOGGLE) generate
      process (ctl_in.clk)
      begin
        if rising_edge(ctl_in.clk) then
          if ctl_in.reset = '1' then
            s_gpi_pulse(ii) <= '0';
          else
            s_gpi_pulse(ii) <= s_gpi_pulse(ii) xor s_gpi_ed(ii);
          end if;
        end if;
      end process;
      s_gpi_output(ii) <= s_gpi_pulse(ii);
    end generate toggle_gen;

    -- no toggle needed
    no_toggle_gen : if not(its(USE_TOGGLE)) generate
      s_gpi_output(ii) <= s_gpi_ed(ii);
    end generate no_toggle_gen;

    process (ctl_in.clk)
    begin
      if rising_edge(ctl_in.clk) then
        s_gpi_output_r(ii) <= s_gpi_output(ii);
      end if;
    end process;

  end generate input_condition_i;

  output_condition_i : for ii in 0 to c_NUM_INPUTS-1 generate
    out_out.data(ii)    <= s_gpi_output(ii);
    -- The mask is the MSW for the property mask_data and output port data and
    -- it is previous data XOR the current data
    out_out.data(ii+16) <= s_gpi_output_r(ii) xor s_gpi_output(ii);

    props_out.mask_data(ii) <= s_gpi_output(ii);
    props_out.mask_data(ii+16) <= s_gpi_output_r(ii) xor s_gpi_output(ii);

  end generate output_condition_i;

  add_zeros_gen : if c_NUM_INPUTS<32 generate
    out_out.data(31 downto c_NUM_INPUTS+16) <= (others=>'0');
    out_out.data(15 downto c_NUM_INPUTS) <= (others=>'0');

    props_out.mask_data(31 downto c_NUM_INPUTS+16) <= (others=>'0');
    props_out.mask_data(15 downto c_NUM_INPUTS) <= (others=>'0');
  end generate add_zeros_gen;

  -- Event mode
  -- Drive out_out.valid only when there has been change in the detected signal
  event_mode_gen : if its(EVENT_MODE) generate
    out_out.valid <= or_reduce(s_gpi_output xor s_gpi_output_r);
  end generate event_mode_gen;

  -- Drive out_out.valid high to send output data every clock cycle
  no_event_mode_gen : if not(its(EVENT_MODE)) generate
    out_out.valid <= '1';
  end generate no_event_mode_gen;


end rtl;
