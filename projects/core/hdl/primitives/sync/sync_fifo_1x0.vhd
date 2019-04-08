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
---------------------------------------------------------------------------------
--
-- Single Bit, 1 deep FIFO with Common Clock
--
-- Description:
--  Implements a 1 deep buffer for a single bit, in a single clock domain.
--  When ENQ is active, a 'value' is loaded into the FIFO and the status flags
--  are set active FULL_N and EMPTY_N flags to indicate that a 'value' is
--  available to be DEQ'ed.
--  The CLR signal is provided to synchronously clear the the FIFO and status flags.
--
--  For simulation purposes, enq/deq error flags are provided to indicate when
--  a ENQ or DEQ was active, while the FIFO was FULL_N or EMPTY_N, respectively.
--
-- Generics:
--  N/A
--
-- Latency:
--  1 CLK
--
-- Background:
--  VHDL replacement for core/hdl/primitives/bsv/imports/FIFO10.v
--
---------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;

entity sync_fifo_1x0 is
  port (
    CLK     : in  std_logic;
    RST     : in  std_logic;
    CLR     : in  std_logic;    
    ENQ     : in  std_logic;
    FULL_N  : out std_logic;
    DEQ     : in  std_logic;
    EMPTY_N : out std_logic);
end entity sync_fifo_1x0;

architecture rtl of sync_fifo_1x0 is

  signal s_empty     : std_logic;
  signal s_enq_error : std_logic;
  signal s_deq_error : std_logic;

begin

  FULL_N  <= not s_empty;
  EMPTY_N <= s_empty;

  process(CLK)
  begin
    if (rising_edge(CLK)) then
      if (RST = '1') then
        s_empty <= '0';
      else
        if (CLR = '1') then
          s_empty <= '0';
        elsif (ENQ = '1') then
          s_empty <= '1';
        elsif (DEQ = '1') then
          s_empty <= '0';
        end if;
      end if;
    end if;
  end process;

  error_checks : process(CLK)
    variable v_name : integer;
  begin
    if (rising_edge(CLK)) then
      if (RST = '1') then
        s_enq_error <= '0';
        s_deq_error <= '0';
      elsif (s_empty = '0' and DEQ = '1') then
        s_enq_error <= '0';
        s_deq_error <= '1';
        report "sync_fifo_1x0:error_checks -- Dequeuing from an empty FIFO." severity warning;
      elsif (s_empty = '1' and ENQ = '1' and DEQ = '0') then
        s_enq_error <= '1';
        s_deq_error <= '0';
        report "sync_fifo_1x0:error_checks -- Enqueuing to a full FIFO." severity warning;
      else
        s_enq_error <= '0';
        s_deq_error <= '0';
      end if;
    end if;
  end process;

end architecture rtl;
