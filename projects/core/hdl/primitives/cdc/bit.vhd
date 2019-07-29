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
-- Single Bit (N-FF Synchronizer) with optional source input register
--
-- Description:
--  Level Synchronizer (Input and Output are Level signals)
--  Open-loop solution (i.e. without Feedback Acknowledgement)
--
--  Synchronizes a single bit from the source clock domain to the destination
--  clock domain.
--
--  For NORMAL operation, the input signal must be sampled two or more times
--  by the destination clock.
--
--  To achieve this requirement when synchronizing from:
--  a) Slow Signals into Fast Clock Domain:
--     - The destination clock must be >= 2x frequency of the source clock.
--  b) Fast Signals into Slow Clock Domain:
--     - The minimum input signal width must be 2x the period of the destination
--       clock and the input pulses must be separated by 2x src_clk cycles to
--       ensure proper crossing of the CDC boundary. Depending on the phase
--       and frequency relationship of the source and destination clocks,
--       the generated pulse in the destination domain may be +1 cycle more
--       than the input pulse.
--
--  (While this is NOT the recommend operating condition, it is worth noting
--  that some references claim that the minimum input pulse must only be 1.5x
--  the period of the destination clock frequency. This case "assumes" that
--  the CDC signal will be sampled at least once and possibly twice by the
--  destination clock. Additionally, if the fast clock is < 1.5x frequency of
--  the slow clock,then there can be a potential problem and a different CDC
--  scheme should be employed.)
--
--  The number of register stages used in the synchronizers is configurable.
--  An optional input register may be used to register the input signal in
--  source clock domain prior to being synchronized.
--
-- Generics:
--  N         : Number of synchronizing FFs. Valid range 2 to 10 (default = 2)
--  IREG      : Optional input register for the asynchronous input. '0'(NO), '1'(YES) (default)
--  RST_LEVEL : Set output level upon reset '0' = low (default), '1' = high
--
-- Latency:
--  dst_out: 1 src_clk (optional) and N dst_clk delays
--
-- Background:
--  Much of the description below is from:
--  http://www.sunburst-design.com/papers/CummingsSNUG2008Boston_CDC.pdf - Section 4.4
--  https://m.eet.com/media/1137372/17561-310388.pdf
--
--  The most basic synchronizer is a 2-FF Synchronizer, both registers clocked by
--  the destination clock. Very high speed designs may require additional FFs
--  (N-FF Synchronizer) to give suffienct MTBF.
--
--  When possible, it is recommended (default) to register the input signal in the
--  source clock domain before sending it across the clock domain crossing (CDC)
--  into synchronizers. The first stage of a synchronizer is sensitive to
--  glitches that combination logic produces. A long enough glitch that occurs
--  at the correct time could meet the setup-and-hold requirements of the first
--  flip-flop in the synchronizer, leading the synchronizer to pass a false-valid
--  indication to the rest of the logic in the new clock domain.
--
---------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity bit is
  generic (
    N         : natural   := 2;         -- Range 2 - 10
    IREG      : std_logic := '1';       -- 0=no, 1=yes input register
    RST_LEVEL : std_logic := '1');      -- 0=low, 1=high
  port (
    src_clk : in  std_logic;           -- optional; required when IREG='1'
    src_rst : in  std_logic;           -- optional; required when IREG='1'
    src_en  : in  std_logic;           -- optional; required when IREG='1'
    src_in  : in  std_logic;
    dst_clk : in  std_logic;
    dst_rst : in  std_logic;           -- optional; if not required, tie '0'
    dst_out : out std_logic);
end entity bit;

architecture rtl of bit is

  signal s_src_reg  : std_logic;
  signal s_dest_reg : std_logic_vector(0 to N-1);

  -- TODO:
  -- Determine the vendor-independent method for applying the proper
  -- constraints which place the synchronizer circuitry in close proximity. (i.e. same slice).
  -- Note:
  -- This may necessitate the removal of the sEN, so that, all registers
  -- in this primitive require the same control signals.
  --
  -- For Xilinx:
  -- https://www.xilinx.com/support/documentation/sw_manuals/xilinx2018_2/ug912-vivado-properties.pdf
  --
  -- In VHDL, where the signal are declared:
  --
  -- XST: Prevent XST from translating two FFs into SRL plus FF
  --attribute SHREG_EXTRACT : string;
  --attribute SHREG_EXTRACT of s_dest_reg : signal is "no";
  --
  -- Vivado: set ASYNC_REG to specify registers receive asynchronous data also acts as DONT_TOUCH
  attribute ASYNC_REG               : string;
  attribute ASYNC_REG of s_dest_reg : signal is "TRUE";
  -- attribute ASYNC_REG of proc_dest_synchronizer : label  is "TRUE";
  --OR
  -- *Ensure that signal names, s_dest_reg, are not used
  -- elsewhere (i.e. outside of a synchronizer) in the project!?*
  -- set_property ASYNC_REG TRUE [get_cells -hierarchical *s_dest_reg]
  --
  --OR
  -- In VHDL, where the sync module is instanced:
  -- attribute ASYNC_REG : string;
  -- attribute ASYNC_REG of sync_my_bit :label is "TRUE";
  -- sync_my_bit : sync_bit
  --   generic map (...
  --OR
  -- In an XDC:
  -- set_property ASYNC_REG TRUE [get_cells top/module1/module2/sync_my_bit]
  --
  -- Create an SDC file, supports Altera and Xilinx:
  -- set_false_path -from [get_pins regA|clk] -to [get_pins regB|aclr]
  --OR
  -- set_false_path -to s_dest_reg/D
  --OR
  -- set_clock_groups -asynchronous -group ACLK -group BCLK
  --OR (in XDC)
  -- set_property ASYNC_REG true [get_cells -regexp {gen\[\d+\]\.Sync/FF2}]
  -- set_property ASYNC_REG true [get_cells -regexp {gen\[\d+\]\.Sync/FF1_METASTABILITY_FFS}]
  -- set_false_path -from [all_clocks] -to [get_pins -regexp {gen\[\d+\]\.Sync/FF1_METASTABILITY_FFS/D}]

  -- Intel/Quartus:
  -- https://forums.intel.com/s/question/0D50P00003yyGc1SAE/does-altera-have-equivalent-of-xilinx-asyncreg-attribute?language=en_US
  -- Quartus automatically detects the synchroniser chains in your design;
  -- the only caveat is that the two clocks must have been constrained
  -- as exclusive in a set_clock_groups command in the sdc-file.
  -- You can read all about this in the quartus manual
  -- (https://www.altera.com/content/dam/altera-www/global/en_us/pdfs/literature/hb/qts/qts-qpp-5v1.pdf): page 914 ff.
  -- There is an option in the Settings menu to specify the (minimum?) length of the synchroniser chain
  --

begin

  assert (true) report "Synchronizer is configured for number stages = " & integer'image(N);

  assert (N >= 2 and N <= 10) report "Generic is out of range, N = " & integer'image(N) severity failure;

  gen_input_reg : if IREG = '1' generate
    input_reg : process(src_clk)
    begin
      if (rising_edge(src_clk)) then
        if (src_rst = '1') then
          s_src_reg <= '0';
        elsif (src_en = '1') then
          s_src_reg <= src_in;
        end if;
      end if;
    end process;
  end generate gen_input_reg;

  gen_no_input_reg : if IREG = '0' generate
    s_src_reg <= src_in;
  end generate gen_no_input_reg;

  nff_synchronizer : process(dst_clk)
  begin
    if (rising_edge(dst_clk)) then
      if (dst_rst = '1') then
        s_dest_reg <= (others => RST_LEVEL);
      else
        s_dest_reg(0)        <= s_src_reg;  -- metastability CDC register
        s_dest_reg(1 to N-1) <= s_dest_reg(0 to N-2);
      end if;
    end if;
  end process;

  dst_out <= s_dest_reg(N-1);

end architecture rtl;
