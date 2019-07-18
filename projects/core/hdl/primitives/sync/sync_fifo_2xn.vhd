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
-- Multi-Bit, 2 deep FIFO with Common Clock
--
-- Description:
--  Implements a 2 deep buffer for multi-bit bus, in a single clock domain.
--  When ENQ is active for a single clock cycle, D_IN is loaded into
--  the FIFO and the EMPTY_N status flag is set active. If the ENQ is active
--  for a second time prior to an active DEQ, then the FULL_N status flag
--  is set active. In this status, the first active DEQ will result in
--  buffered data presented on D_OUT, as well as clearing of the FULL_N
--  flags, and a second DEQ (without an ENQ) will result in clearing of the
--  EMPTY_N flag.
--  The CLR signal is provided to synchronously clear the the FIFO and status flags.
--
--  For simulation purposes, enq/deq error flags are provided to indicate when
--  a ENQ or DEQ was active, while the FIFO was FULL_N or EMPTY_N, respectively.
--
-- Generics:
--  WIDTH =
--
-- Latency:
--  1 CLK
--
-- Background:
-- VHDL replacement for core/hdl/primitives/bsv/imports/FIFO2.v
--
---------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity sync_fifo_2xn is
  generic (
    WIDTH : positive := 1);
  port (
    CLK     : in std_logic;
    RST     : in std_logic;
    CLR     : in std_logic;
    D_IN    : in std_logic_vector(WIDTH-1 downto 0);
    ENQ     : in std_logic;
    FULL_N  : out std_logic;
    D_OUT   : out std_logic_vector(WIDTH-1 downto 0);
    DEQ     : in std_logic;
    EMPTY_N : out std_logic);
end entity sync_fifo_2xn;

architecture rtl of sync_fifo_2xn is

  signal d0di        : std_logic;
  signal d0d1        : std_logic;
  signal d0h         : std_logic;
  signal d1di        : std_logic;
  signal s_data_r0   : std_logic_vector(WIDTH-1 downto 0);
  signal s_data_r1   : std_logic_vector(WIDTH-1 downto 0);
  signal s_full      : std_logic;
  signal s_empty     : std_logic;
  signal s_enq_error : std_logic;
  signal s_deq_error : std_logic;

begin

  D_OUT   <= s_data_r0;
  FULL_N  <= s_full;
  EMPTY_N <= s_empty;

  process(CLK)
  begin
    if (rising_edge(CLK)) then
      if (RST = '1') then
        s_full  <= '1';
        s_empty <= '0';
      else
        if (CLR = '1') then
          s_full  <= '1';
          s_empty <= '0';
        elsif (ENQ = '1') then
          s_full  <= not s_empty;
          s_empty <= '1';
        elsif (DEQ = '1') then
          s_full  <= '1';
          s_empty <= not s_full;
        end if;
      end if;
    end if;
  end process;

  d0di <= (ENQ and not s_empty) or (ENQ and DEQ and s_full);
  d0d1 <= DEQ and not s_full;
  d0h <= (not DEQ and not ENQ) or (not DEQ and s_empty) or (not ENQ and s_full);
  d1di <= ENQ and s_empty;

  process(CLK)
  begin
    if (rising_edge(CLK)) then
      if (RST = '1') then
        s_data_r0 <= (others => '0');
        s_data_r1 <= (others => '0');
      else
        s_data_r0 <= ((width-1 downto 0 => d0di) and D_IN) or
                     ((width-1 downto 0 => d0d1) and s_data_r1) or
                     ((width-1 downto 0 => d0h) and s_data_r0);
        
        if (d1di = '1') then
          s_data_r1 <= D_IN;
        end if;
      end if;
    end if;
  end process;

  error_checks : process(CLK)
  begin
    if (rising_edge(CLK)) then
      if (RST = '1') then
        s_enq_error <= '0';
        s_deq_error <= '0';
      elsif (s_empty = '0' and DEQ = '1') then
        s_enq_error <= '0';
        s_deq_error <= '1';
        report "sync_fifo_2xn:error_checks -- Dequeuing from an empty FIFO." severity warning;
      elsif (s_full = '0' and ENQ = '1' and DEQ = '0') then
        s_enq_error <= '1';
        s_deq_error <= '0';
        report "sync_fifo_2xn:error_checks -- Enqueuing to a full FIFO." severity warning;        
      else
        s_enq_error <= '0';
        s_deq_error <= '0';
      end if;
    end if;
  end process;

end architecture rtl;
