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
-- Pulse to pulse CDC with no clock restrictions.
--
-- Description:
--  Pulse Synchronizer (Input and Output are single Pulses)
--  Closed-Loop
--
--  The basic function of a Pulse synchronizer is to take a single clockwide
--  pulse from the source clock domain and create a single clockwide pulse in the
--  destination clock domain.
--
--  This implementation is slow and works with any clock relationship (fast/slow, slow/fast, etc.)
--
-- Generics:
--  N     : Number of synchronizing FFs. Valid range 2 to 10 (default = 2)
--
-- Latency:
--  dest_pulse: 3 src_clk and N dest_clk delays
--
---------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity pulse is
  generic (
    N : natural := 2);
  port (
    src_clk   : in  std_logic;
    src_rst   : in  std_logic;
    src_in    : in  std_logic;
    src_rdy   : out std_logic;
    dst_clk   : in  std_logic;
    dst_rst   : in  std_logic;
    dst_out   : out std_logic);
end entity pulse;

architecture rtl of pulse is

  signal s_src_ack    : std_logic;
  signal s_src_toggle : std_logic;
  signal s_dest_reg0  : std_logic;
  signal s_dest_reg1  : std_logic;
  signal s_src_rdy    : std_logic;

begin

  -- Indicates when circuit is 'ready' to accept new input
  s_src_rdy <= s_src_ack xnor s_src_toggle;
  src_rdy <= s_src_rdy;
  -- Level detection used to generate output
  dst_out <= s_dest_reg0 xor s_dest_reg1;

  input_enabled_toggle : process(src_clk)
  begin
    if (rising_edge(src_clk)) then
      if (src_rst = '1') then
        s_src_toggle <= '0';
      elsif (src_in = '1') then
        -- the user is violating the rules
        assert s_src_rdy = '1' severity failure;
        s_src_toggle <= not s_src_toggle;
      end if;
    end if;
  end process;

  nff_synchronizer : component work.cdc.bit
    generic map (
      N    => N,
      IREG => '0')
    port map (
      src_clk => '0',
      src_rst => '0',
      src_en  => '0',
      src_in  => s_src_toggle,
      dst_clk => dst_clk,
      dst_rst => dst_rst,
      dst_out => s_dest_reg0);

  level_detection_prep_reg : process(dst_clk)
  begin
    if (rising_edge(dst_clk)) then
      if (dst_rst = '1') then
        s_dest_reg1 <= '0';
      else
        s_dest_reg1 <= s_dest_reg0;
      end if;
    end if;
  end process;

  acknowledge_nff_synchronizer : component work.cdc.bit
    generic map (
      N         => N,
      IREG      => '0',
      RST_LEVEL => '1')
    port map (
      src_clk => '0',
      src_rst => '0',
      src_en  => '0',
      src_in  => s_dest_reg0,
      dst_clk => src_clk,
      dst_rst => src_rst,
      dst_out => s_src_ack);

end architecture rtl;
