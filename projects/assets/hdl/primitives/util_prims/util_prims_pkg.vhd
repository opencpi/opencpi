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

package util_prims is

component peakDetect
  port (
    CLK_IN    : in  std_logic;
    RST_IN    : in  std_logic;
    EN_IN     : in  std_logic;
    A_IN      : in  std_logic_vector(15 downto 0);
    B_IN      : in  std_logic_vector(15 downto 0);
    PEAK_OUT  : out std_logic_vector(15 downto 0));
end component peakDetect;

component zero_padding_gen
  generic (
    MAX_NUM_ZEROS : positive := 64;
    DIN_WIDTH     : positive := 32;
    DOUT_WIDTH    : positive := 32);
  port (
    RST_IN    : in  std_logic;
    CLK_IN    : in  std_logic;
    NUM_ZEROS : in  integer;
    ENABLE    : in  std_logic;
    BUSY_N    : out std_logic;
    DIN       : in  std_logic_vector(DIN_WIDTH-1 downto 0);
    DIN_VLD   : in  std_logic_vector(DIN_WIDTH-1 downto 0);
    REN       : in  std_logic;
    DOUT      : out std_logic_vector((DOUT_WIDTH)-1 downto 0);
    DOUT_VLD  : out std_logic);
end component zero_padding_gen;

component complex_mult
  generic (
    A_WIDTH    : positive;
    B_WIDTH    : positive;
    PROD_WIDTH : positive); --PROD_WIDTH <= AWIDTH+BWIDTH
  port (
    CLK      : in  std_logic;
    RST      : in  std_logic;
    DIN_A_RE : in  std_logic_vector(A_WIDTH-1 downto 0);
    DIN_A_IM : in  std_logic_vector(A_WIDTH-1 downto 0);
    DIN_B_RE : in  std_logic_vector(B_WIDTH-1 downto 0);
    DIN_B_IM : in  std_logic_vector(B_WIDTH-1 downto 0);
    DIN_VLD  : in  std_logic;
    DOUT_RE  : out std_logic_vector(PROD_WIDTH-1 downto 0);
    DOUT_IM  : out std_logic_vector(PROD_WIDTH-1 downto 0);
    DOUT_VLD : out std_logic);
end component complex_mult;

component AGC
  generic (
    DATA_WIDTH : integer := 16;
    NAVG       : integer := 16);
  port (
    CLK     : in  std_logic;
    RST     : in  std_logic;
    REG_WR  : in  std_logic;
    REF     : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    MU      : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    DIN_VLD : in  std_logic;
    HOLD    : in  std_logic;
    DIN     : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    DOUT    : out std_logic_vector(DATA_WIDTH-1 downto 0));
end component AGC;

component divide_by_constant
  generic(
    g_const      : positive;
    g_num_bits   : positive;
    g_data_width : positive);
  port(
    i_clk        : in std_logic;
    i_rst        : in std_logic;
    i_din        : in unsigned(g_data_width-1 downto 0);
    i_din_vld    : in std_logic;
    o_din_rdy    : out std_logic;
    o_dout       : out unsigned(g_data_width-1 downto 0);
    o_dout_vld   : out std_logic;
    i_dout_rdy   : in std_logic);
end component divide_by_constant;

component set_clr
  port (
    clk : in std_logic;
    rst : in std_logic;
    set : in std_logic;
    clr : in std_logic;
    q   : out std_logic;
    q_r : out std_logic);
end component set_clr;

end package util_prims;
