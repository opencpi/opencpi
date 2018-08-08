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
-- Linear Feedback Shift Register
-------------------------------------------------------------------------------
--
-- File: lfsr.vhd
--
-- Description:
-- This component implements a Linear Feedback Shift Register (LFSR) with a
-- generic polynomial and seed value.
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity lfsr is
generic (
   POLYNOMIAL : std_logic_vector := x"ffff";
   SEED       : std_logic_vector := x"0000"); -- must never be all zeros and must
                                              -- be same width as POLYNOM
port (
   CLK : in std_logic; -- active high clock
   RST : in std_logic; -- synchronous reset, active high
   EN  : in std_logic; -- active high enable
   REG : out std_logic_vector(POLYNOMIAL'length-1 downto 0)); -- REG is output
  -- with 0 clock delay, REG value always valid, EN just tells it to advance)
end lfsr;
architecture behavior of lfsr is
  signal lfsr_r   : std_logic_vector(POLYNOMIAL'length-1 downto 0) := (others => '0');
  signal feedback : std_logic_vector(POLYNOMIAL'length-1 downto 0) := (others => '0');
begin
  REG <= lfsr_r;

  feedback_taps : process(lfsr_r)
    variable feed      : std_logic := '0';
    variable feed_last : std_logic := '0';
  begin
    for tap_idx in POLYNOMIAL'length-1 downto 0 loop
      if tap_idx = POLYNOMIAL'length-1 then
        feed      := '0';
        feed_last := '0';
      end if;
      -- note POLYNOMIAL default to (... to ...) direction
      if POLYNOMIAL(POLYNOMIAL'length-1-tap_idx) = '1' then
        feed := lfsr_r(POLYNOMIAL'length-1-tap_idx) xor feed_last;
      end if;
      feedback(tap_idx) <= feed;
      feed_last := feed;
    end loop;
  end process;

  lfsr : process(CLK)
  begin
    if rising_edge(CLK) then
      if RST = '1' then
        lfsr_r <= SEED;
      elsif EN = '1' then
        lfsr_r <= feedback(0) &
                  lfsr_r(lfsr_r'length-1 downto 1);
      end if;
    end if;
  end process lfsr;
end;

