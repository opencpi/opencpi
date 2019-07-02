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

library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; use ocpi.util.all;

entity delayline is
  generic (
    g_latency     : integer  := 1
  );
  port (
    i_clk         : in std_logic;
    i_reset       : in Bool_t;
    i_enable      : in Bool_t;
    i_ready       : in Bool_t;
    i_som         : in Bool_t;
    i_eom         : in Bool_t;
    i_opcode      : in std_logic_vector;
    i_valid       : in Bool_t;
    i_byte_enable : in std_logic_vector;
    i_data        : in std_logic_vector;
    i_eof         : in Bool_t;
    o_ready       : out Bool_t;
    o_som         : out Bool_t;
    o_eom         : out Bool_t;
    o_opcode      : out std_logic_vector;
    o_valid       : out Bool_t;
    o_byte_enable : out std_logic_vector;
    o_data        : out std_logic_vector;
    o_eof         : out Bool_t
  );
end delayline;

architecture rtl of delayline is

  type t_data_array is array (natural range <>) of std_logic_vector(i_data'range);
  type t_byte_enable_array is array (natural range <>) of std_logic_vector(i_byte_enable'range);
  type t_opcode_array is array (natural range <>) of std_logic_vector(i_opcode'range);

  signal s_ready       : std_logic_vector(g_latency-1 downto 0);
  signal s_som         : std_logic_vector(g_latency-1 downto 0);
  signal s_eom         : std_logic_vector(g_latency-1 downto 0);
  signal s_eof         : std_logic_vector(g_latency-1 downto 0);
  signal s_valid       : std_logic_vector(g_latency-1 downto 0);
  signal s_data        : t_data_array(g_latency-1 downto 0);
  signal s_byte_enable : t_byte_enable_array(g_latency-1 downto 0);
  signal s_opcode      : t_opcode_array(g_latency-1 downto 0);

begin

  latency_eq_one_gen : if g_latency = 1 generate
    delayLine : process (i_clk)
    begin
      if rising_edge(i_clk) then
        if (i_reset = '1') then
          s_ready          <= (others => '0');
          s_som            <= (others => '0');
          s_eom            <= (others => '0');
          s_eof            <= (others => '0');
          s_opcode         <= (others => (others => '0'));
          s_valid          <= (others => '0');
          s_data           <= (others => (others => '0'));
          s_byte_enable    <= (others => (others => '0'));
        elsif i_enable = '1' then
          s_ready(0)       <= i_ready;
          s_som(0)         <= i_som;
          s_eom(0)         <= i_eom;
          s_eof(0)         <= i_eof;
          s_opcode(0)      <= i_opcode;
          s_byte_enable(0) <= i_byte_enable;
          s_valid(0)       <= i_valid;
          s_data(0)        <= i_data;
        end if;    
      end if;
    end process delayLine;
  end generate latency_eq_one_gen;

  latency_gt_one_gen : if g_latency > 1 generate
    delayLine : process (i_clk)
    begin
      if rising_edge(i_clk) then
        if (i_reset = '1') then
          s_ready       <= (others => '0');
          s_som         <= (others => '0');
          s_eom         <= (others => '0');
          s_eof         <= (others => '0');
          s_byte_enable <= (others => (others => '0'));
          s_opcode      <= (others => (others => '0'));
          s_valid       <= (others => '0');
          s_data        <= (others => (others => '0'));
        elsif i_enable = '1' then
          s_ready       <= s_ready(s_ready'high-1 downto 0) & i_ready;
          s_som         <= s_som(s_som'high-1 downto 0) & i_som;
          s_eom         <= s_eom(s_eom'high-1 downto 0) & i_eom;
          s_eof         <= s_eof(s_eof'high-1 downto 0) & i_eof;
          s_byte_enable <= s_byte_enable(s_byte_enable'high-1 downto 0) & i_byte_enable;
          s_opcode      <= s_opcode(s_opcode'high-1 downto 0) & i_opcode;
          s_valid       <= s_valid(s_valid'high-1 downto 0) & i_valid;
          s_data        <= s_data(s_data'high-1 downto 0) & i_data;
        end if;
      end if;
    end process delayLine;
  end generate latency_gt_one_gen;

o_ready       <= s_ready(s_ready'high);
o_som         <= s_som(s_som'high);
o_eom         <= s_eom(s_eom'high);
o_eof         <= s_eof(s_eof'high);
o_opcode      <= s_opcode(s_opcode'high);
o_valid       <= s_valid(s_valid'high);
o_byte_enable <= s_byte_enable(s_byte_enable'high);
o_data        <= s_data(s_data'high);

end rtl;
