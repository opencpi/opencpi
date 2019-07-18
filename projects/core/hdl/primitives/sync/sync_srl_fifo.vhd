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
-- SRL-based FIFO with Common Clock
--
-- Description:
--  Implements a multi-bit bus SRL-based FIFO buffer in a single clock domain.
--  The width, depth, and optional output register are configurable via generics.
--
--  Without Optional Output Register:
--  When ENQ is active for a single clock cycle, D_IN is loaded into the FIFO.
--  After 1 clock cycle, the EMPTY_N status flag is set active and
--  data is made available on D_OUT two clock cycles later.
--  The FULL_N status flag is set active the rising edge of the clock following
--  an ENQ.
--
--  With Optional Output Register:
--  When ENQ is active for a single clock cycle, D_IN is loaded into the FIFO.
--  After 2 clock cycles, the EMPTY_N status flag is set active and
--  data is made available on D_OUT two clock cycles later.
--  The FULL_N status flag is set active the rising edge of the clock following
--  an ENQ.
--
--  The CLR signal is provided to synchronously clear the status flags, but not
--  the SRL FIFO itself, as adding a reset to the SRL circuit results in the
--  synthesis tools failing to map to SRL resources.
--
--  For simulation purposes, enq/deq error flags are provided to indicate when
--  a ENQ or DEQ was active, while the FIFO was FULL_N or EMPTY_N, respectively.
--
-- Generics:
--  WIDTH   : # of bits for input and output data vectors. (>= 1)
--  L2DEPTH : Set the depth of the FIFO via 2^L2DEPTH. (>= 1)
--  OREG    : Optional output register for improve timing.  '0'(NO) (default), '1'(YES)
--
-- Latency:
--  1 CLK or 2 CLK (with optional output register)
--
-- Background:
--  VHDL replacement for core/hdl/primitives/util/{arSRLFIFO.v and arSRLFIFOD.v}.
--  - However, it is not an 'exact' replacement, because the reset is active high.
--
---------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity sync_srl_fifo is
  generic (
    WIDTH   : positive  := 8;
    L2DEPTH : positive  := 2;
    OREG    : std_logic := '0');
  port (
    clk     : in  std_logic;
    rst     : in  std_logic;
    clr     : in  std_logic;
    d_in    : in  std_logic_vector(WIDTH-1 downto 0);
    enq     : in  std_logic;
    full_n  : out std_logic;
    d_out   : out std_logic_vector(WIDTH-1 downto 0);
    deq     : in  std_logic;
    empty_n : out std_logic);
end entity sync_srl_fifo;

architecture rtl of sync_srl_fifo is

  constant c_depth : positive := 2**L2DEPTH;

  type slv_array is array (natural range 0 to c_depth-1) of std_logic_vector(WIDTH-1 downto 0);
  signal s_data_srl  : slv_array;       -- SRL FIFO
  signal s_data_oreg : std_logic_vector(WIDTH-1 downto 0);  -- optional output register
  signal s_position  : unsigned(integer(L2DEPTH)-1 downto 0);  -- head position
  signal s_deq       : std_logic;
  signal s_full      : std_logic;
  signal s_empty     : std_logic;
  signal s_dempty    : std_logic;

  -- Xilinx UG901
  attribute shreg_extract               : string;
  attribute shreg_extract of s_data_srl : signal is "yes";
  attribute srl_style                   : string;
  attribute srl_style of s_data_srl     : signal is "srl";

begin

  full_n <= not s_full;

  inferred_srl : process(clk)
  begin
    if (rising_edge(clk)) then
      if (enq = '1') then               -- SRL should be inferred here...
        for i in c_depth-1 downto 1 loop
          s_data_srl(i) <= s_data_srl(i-1);
          s_data_srl(0) <= d_in;
        end loop;
      end if;
    end if;
  end process;

  data_pointer : process(clk)
  begin
    if (rising_edge(clk)) then
      if (rst = '1' or clr = '1') then
        s_position <= (others => '0');
      elsif (enq = '0' and s_deq = '1') then
        s_position <= s_position - 1;
      elsif (enq = '1' and s_deq = '0') then
        s_position <= s_position + 1;
      end if;
    end if;
  end process;

  full_flag : process(clk)
  begin
    if (rising_edge(clk)) then
      if (rst = '1' or clr = '1') then
        s_full <= '0';
      else
        if (s_deq = '0' and (s_position = (c_depth-1) or
                             (s_position = (c_depth-2) and enq = '1'))) then
          s_full <= '1';
        else
          s_full <= '0';
        end if;
      end if;
    end if;
  end process;

  empty_flag : process(clk)
  begin
    if (rising_edge(clk)) then
      if (rst = '1' or clr = '1') then
        s_empty <= '1';
      else
        if (enq = '0' and (s_position = 0 or (s_position = 1 and s_deq = '1'))) then
          s_empty <= '1';
        else
          s_empty <= '0';
        end if;
      end if;
    end if;
  end process;

  gen_no_oreg : if (OREG = '0') generate

    empty_n <= not s_empty;
    d_out   <= (others => '0') when s_position = 0 else s_data_srl(to_integer(s_position-1));
    s_deq   <= deq;

  end generate;

  gen_yes_oreg : if (OREG = '1') generate

    empty_n <= not s_dempty;
    d_out   <= s_data_oreg;
    s_deq   <= (s_dempty and not s_empty) or (not s_dempty and deq and not s_empty);

    -- This registered version of the SRLFIFO operates as if there is 1-deep FIFO
    -- appended to the output of the SRL FIFO. An enq of the 1-deep FIFO
    -- must happen with a deq of the SRL FIFO, this internal signal is "s_deq"
    empty_flag_for_oreg : process(clk)
    begin
      if (rising_edge(clk)) then
        if (rst = '1' or clr = '1') then
          s_dempty <= '1';
        elsif (s_deq = '1') then
          s_dempty <= '0';              -- False when we load the 1-deep FIFO
        elsif (deq = '1' and s_empty = '1') then
          s_dempty <= '1';  -- True when we deq and nothing to s_deq
        end if;
      end if;
    end process;

    oreg : process(clk)
    begin
      if (rising_edge(clk)) then
        if (s_deq = '1') then
          s_data_oreg <= s_data_srl(to_integer(s_position-1));  -- transfer the SRL to the D reg
        end if;
      end if;
    end process;

  end generate;

end architecture rtl;
