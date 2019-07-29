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

package sync is

  component sync_sync_rst
    generic (
      RSTDELAY : integer);
    port (
      src_rst  : in  std_logic;
      dest_clk : in  std_logic;
      dest_rst : out std_logic
      );
  end component sync_sync_rst;

  component sync_async_rst
    generic (
      RSTDELAY : integer := 2);
    port (
      src_arst  : in  std_logic;
      dest_clk  : in  std_logic;
      dest_arst : out std_logic
      );
  end component sync_async_rst;

  component sync_bit
    generic (
      N         : natural;
      IREG      : std_logic;
      RST_LEVEL : std_logic);
    port (
      src_clk  : in  std_logic;
      src_rst  : in  std_logic;
      src_en   : in  std_logic;
      src_in   : in  std_logic;
      dest_clk : in  std_logic;
      dest_rst : in  std_logic;
      dest_out : out std_logic);
  end component sync_bit;

  component sync_bit_ack
    generic (
      N : natural);
    port (
      src_clk  : in  std_logic;
      src_rst  : in  std_logic;
      src_en   : in  std_logic;
      src_in   : in  std_logic;
      src_ack  : out std_logic;
      dest_clk : in  std_logic;
      dest_rst : in  std_logic;
      dest_out : out std_logic);
  end component;

  component sync_bits_feedback is
    generic (
      WIDTH : positive := 1);
    port (
      src_clk   : in  std_logic;
      src_rst   : in  std_logic;
      src_en    : in  std_logic;
      src_rdy   : out std_logic;
      src_data  : in  std_logic_vector(WIDTH-1 downto 0);
      dest_clk  : in  std_logic;
      dest_rst  : in  std_logic;
      dest_data : out std_logic_vector(WIDTH-1 downto 0)
      );
  end component;

  component sync_pulse
    generic (
      N : natural);
    port (
      src_clk    : in  std_logic;
      src_rst    : in  std_logic;
      src_en     : in  std_logic;
      dest_clk   : in  std_logic;
      dest_rst   : in  std_logic;
      dest_pulse : out std_logic);
  end component;

  component sync_pulse_handshake
    generic (
      N : natural := 2);
    port (
      src_clk    : in  std_logic;
      src_rst    : in  std_logic;
      src_pulse  : in  std_logic;
      src_rdy    : out std_logic;
      dest_clk   : in  std_logic;
      dest_rst   : in  std_logic;
      dest_pulse : out std_logic);
  end component;

  component sync_fifo_1x0
    port (
      CLK     : in  std_logic;
      RST     : in  std_logic;
      ENQ     : in  std_logic;
      DEQ     : in  std_logic;
      CLR     : in  std_logic;
      FULL_N  : out std_logic;
      EMPTY_N : out std_logic);
  end component;

  component sync_fifo_1xn
    generic (
      WIDTH : positive);
    port (
      CLK     : in  std_logic;
      RST     : in  std_logic;
      CLR     : in  std_logic;
      D_IN    : in  std_logic_vector(width-1 downto 0);
      ENQ     : in  std_logic;
      D_OUT   : out std_logic_vector(width-1 downto 0);
      DEQ     : in  std_logic;
      FULL_N  : out std_logic;
      EMPTY_N : out std_logic);
  end component;

  component sync_fifo_2x0
    port (
      CLK     : in  std_logic;
      RST     : in  std_logic;
      ENQ     : in  std_logic;
      DEQ     : in  std_logic;
      CLR     : in  std_logic;
      FULL_N  : out std_logic;
      EMPTY_N : out std_logic);
  end component;

  component sync_fifo_2xn
    generic (
      WIDTH : positive);
    port (
      CLK     : in  std_logic;
      RST     : in  std_logic;
      CLR     : in  std_logic;
      D_IN    : in  std_logic_vector(width-1 downto 0);
      ENQ     : in  std_logic;
      D_OUT   : out std_logic_vector(width-1 downto 0);
      DEQ     : in  std_logic;
      FULL_N  : out std_logic;
      EMPTY_N : out std_logic);
  end component;

  component sync_fifo_ic
    generic (
      DATAWIDTH : natural;
      DEPTH     : natural;
      INDXWIDTH : natural);
    port (
      sCLK     : in  std_logic;
      sRST     : in  std_logic;
      sENQ     : in  std_logic;
      sD_IN    : in  std_logic_vector(DATAWIDTH-1 downto 0);
      sFULL_N  : out std_logic;
      dCLK     : in  std_logic;
      dDEQ     : in  std_logic;
      dD_OUT   : out std_logic_vector(DATAWIDTH-1 downto 0);
      dEMPTY_N : out std_logic);
  end component;

  component sync_srl_fifo
    generic (
      WIDTH   : positive;
      L2DEPTH : positive;
      OREG    : std_logic);
    port (
      CLK     : in  std_logic;
      RST     : in  std_logic;
      CLR     : in  std_logic;
      D_IN    : in  std_logic_vector(width-1 downto 0);
      ENQ     : in  std_logic;
      D_OUT   : out std_logic_vector(width-1 downto 0);
      DEQ     : in  std_logic;
      FULL_N  : out std_logic;
      EMPTY_N : out std_logic);
  end component;

  component sync_fifo_gray is
    generic (
      DATAWIDTH : natural := 1;
      DEPTH     : natural := 2;           --minimum 2
      INDXWIDTH : natural := 1);          --minimum 1
    port (
      sCLK     : in  std_logic;
      sRST     : in  std_logic;
      sENQ     : in  std_logic;
      sD_IN    : in  std_logic_vector(DATAWIDTH-1 downto 0);
      sFULL_N  : out std_logic;
      dCLK     : in  std_logic;
      dDEQ     : in  std_logic;
      dD_OUT   : out std_logic_vector(DATAWIDTH-1 downto 0);
      dEMPTY_N : out std_logic);
  end component;

end package sync;
