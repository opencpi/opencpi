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
-- Switch Debounce Circuit
-------------------------------------------------------------------------------
--
-- File: debounce.vhd
--
-- Description:
-- Mechanical switches are often wired to FPGA input pins. When the switch is
-- actuated the contacts often rebound, or bounce, off one another before
-- settling into a stable state. Several methods exist to deal with this
-- phenomena, but this circuit implements a debounce circuit using a counter.
--
-- The XOR gate and N-bit counter accomplish the timing. If the button’s level
-- changes, the values of button_q1 and button_q2 differ for a clock cycle,
-- clearing the N-bit counter via the XOR gate. If the button’s level is
-- unchanging (i.e. if button_q1 and button_q2 are the same logic level), then
-- the XOR gate releases the counter’s synchronous clear, and the counter begins
-- to count. The counter continues to increment in this manner until it reaches
-- the specified time and enables the output register, or is interrupted and
-- cleared by the XOR gate because the button’s logic level is not yet stable.
--
-- The counter’s size determines the time required to validate the button’s
-- stability. When the counter increments to the point that its carry out bit is
-- asserted, it disables itself from incrementing further and enables the output
-- register RESULT. The circuit remains in this state until a different button
-- value is clocked into RESULT, clearing the counter via the XOR gate.
--
-- The size of the counter and the clock frequency together determine the time
-- period that validates the button's stability. That relationship is given by:
--
-- P = (2^N + 2)/f ~ 2^N/f
--
-- Most switches reach a stable logic level within 10ms of the actuation.
-- Supposing we have a 100MHz clock, we need to count 0.01*100,000,000 = 1,000,000
-- clock cycles to reach 10ms. A 20-bit counter fulfills this requirement.
-- Using the counter’s carry output eliminates the requirement of evaluating the
-- entire output bus of the counter. With this method, the actual time implemented is
-- (2^20+2) / 100,000,000 = 10.49ms.
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity debounce is
generic (
  COUNTER_WIDTH : positive);

port (
  CLK    : in  std_logic;
  RST    : in  std_logic;
  BUTTON : in  std_logic;
  RESULT : out std_logic);

end debounce;

architecture behavior of debounce is
  signal button_q1 : std_logic;
  signal button_q2 : std_logic;
  signal count     : unsigned(COUNTER_WIDTH downto 0) := (others => '0');

begin

  -- implement the registers that address metastability and output
  process(CLK)
  begin
    if rising_edge(CLK) then
      if RST = '1' then
        button_q1 <= '0';
        button_q2 <= '0';
        RESULT    <= '0';
      else
        button_q1 <= BUTTON;
        button_q2 <= button_q1;
        if count(COUNTER_WIDTH) = '1' then
          RESULT  <= button_q2;
        end if;
      end if;
    end if;
  end process;

  -- implement the counter
  process(CLK)
  begin
    if rising_edge(CLK) then
      if RST = '1' then
        count <= (others => '0');
      elsif (button_q1 xor button_q2) = '1' then
        count <= (others => '0');
      elsif count(COUNTER_WIDTH) = '0' then
        count <= count + 1;
      end if;
    end if;
  end process;

end behavior;
