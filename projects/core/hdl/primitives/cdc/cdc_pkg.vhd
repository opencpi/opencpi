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

library ieee;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

package cdc is

  component reset
    generic (
      RST_DELAY : integer := 2);
    port (
      src_rst : in  std_logic;
      dst_clk : in  std_logic;
      dst_rst : out std_logic);
  end component reset;

  component fifo is
    generic (
      WIDTH       : natural := 1;
      DEPTH       : natural := 2);           --minimum 2
    port (
      src_CLK     : in  std_logic;
      src_RST     : in  std_logic;
      src_ENQ     : in  std_logic;
      src_in      : in  std_logic_vector(WIDTH-1 downto 0);
      src_FULL_N  : out std_logic;
      dst_CLK     : in  std_logic;
      dst_DEQ     : in  std_logic;
      dst_out     : out std_logic_vector(WIDTH-1 downto 0);
      dst_EMPTY_N : out std_logic);
  end component fifo;

  component bit is
    generic (
      N         : natural   := 2;         -- Range 2 - 10
      IREG      : std_logic := '0';       -- 0=no, 1=yes input register
      RST_LEVEL : std_logic := '1');      -- 0=low, 1=high
    port (
      src_clk  : in  std_logic;           -- optional; required when IREG='1'
      src_rst  : in  std_logic;           -- optional; required when IREG='1'
      src_en   : in  std_logic;           -- optional; required when IREG='1'
      src_in   : in  std_logic;
      dst_clk  : in  std_logic;
      dst_rst  : in  std_logic;           -- optional; if not required, tie '0'
      dst_out  : out std_logic);
  end component bit;

  component bits is
    generic (
      N         : natural   := 2;         -- Range 2 - 10
      IREG      : std_logic := '0';       -- 0=no, 1=yes input register
      RST_LEVEL : std_logic := '1';       -- 0=low, 1=high
      WIDTH     : positive  := 1);
    port (
      src_clk   : in  std_logic;
      src_rst   : in  std_logic;
      src_in    : in  std_logic_vector(WIDTH-1 downto 0);
      dst_clk   : in  std_logic;
      dst_rst   : in  std_logic;           -- optional; if not required, tie '0'
      dst_out   : out std_logic_vector(WIDTH-1 downto 0));
  end component bits;

  component count_up is
    generic (
      N         : natural   := 2;
      WIDTH     : positive  := 1);
    port (
      src_clk : in  std_logic;
      src_rst : in  std_logic;
      src_in  : in  std_logic;
      dst_clk : in  std_logic;
      dst_rst : in  std_logic;           -- optional; if not required, tie '0'
      dst_out : out unsigned(WIDTH-1 downto 0));
  end component count_up;

  component pulse is
    generic (
      N       : natural := 2);
    port (
      src_clk : in  std_logic;
      src_rst : in  std_logic;
      src_in  : in  std_logic;
      src_rdy : out  std_logic;
      dst_clk : in  std_logic;
      dst_rst : in  std_logic;
      dst_out : out std_logic);
  end component pulse;
end package cdc;
