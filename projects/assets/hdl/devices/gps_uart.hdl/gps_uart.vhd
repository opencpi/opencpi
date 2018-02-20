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

 ------------------------------------------------------------------------------ 
 --GPS UART Device Worker Source Code

 --File: gps_uart.vhd

 --Description: 
 --This device worker will implement a UART to receive NMEA position data from 
 --a GPS IC
 -------------------------------------------------------------------------------
library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
--library unisim;
--use unisim.vcomponents.all;

architecture rtl of gps_uart_worker is 
  ------------------------------------------------------------------------------
  -- Constants
  -- Extracted from uart_to_bus.vhd from EMBEDDED framework repo
  ------------------------------------------------------------------------------
  -- Set the Baud Enable Rate to 16 times the Link Baud Rate:
  -- Received data will be sampled 16 times per baud counting from detection of the
  -- Start Bit (First Hi-to-Low transition).
  --
  -- The Baud Counter counts 50MHz clock cycles and clears when it reaches 1/16th baud.
  -- This will occur after 50,000,000 / (16 * Baud Rate) clocks.
  --
  -- Set BAUD_CNT_MAX as follows to achieve various baud rates:
  --
  -- Desired    50M Clks  50M Clks  Achieved    Percent
  -- Baud Rate  Actual    Rounded   Baud Rate   Error     BAUD_CNT_MAX
  -- =========  ========  ========  ==========  ========  ============
  -- 110        28409.09  28409     110.000     +0.0003%  28408
  -- 300        10416.67  10417     299.990     -0.0032%  10416
  -- 600        5208.33   5208      600.038     +0.0064%  5207
  -- 1200       2604.17   2604      1200.077    +0.0064%  2603
  -- 2400       1302.08   1302      2400.154    +0.0064%  1301
  -- 4800       651.04    651       4800.307    +0.0064%  650
  -- 9600       325.52    326       9585.890    -0.1470%  325
  -- 14400      217.01    217       14400.922   +0.0064%  216
  -- 19200      162.76    163       19171.779   -0.1470%  162
  -- 38400      81.38     81        38580.247   +0.4694%  80
  -- 57600      54.25     54        57870.370   +0.4694%  53
  -- 115200     27.13     27        115740.741  +0.4694%  26
  -- 128000     24.41     24        130208.333  +1.7253%  23
  -- 256000     12.21     12        260416.667  +1.7253%  11

  -- BAUD_CNT_MAX Setting
  --
  -- Set to floor( {Clock Rate} / ( 16*{Baud Rate} ) )
  --
  -- 9600 baud for hw / 115200 for sim
  --
  -- 50.00 MHz Clock  |  325 (hw)  |  26 (sim)
  -- 30.72 MHz Clock  |  199 (hw)  |  15 (sim)
  --

  constant BAUD_CNT_MAX : positive := 325;

  -- Set the Baud Counter Width. 16 bits is sufficient for all modes listed above

  constant BAUD_CNT_WIDTH : positive := 16;
  
  -----------------------------------------------------------------------------
  -- Component Declarations
  -----------------------------------------------------------------------------
  component uart_tx_rmm
    generic (
      SAMPS_PER_BAUD : positive := 16;
      BUFFER_DEPTH   : positive := 16);
    port (
      CLK     : in  std_logic;
      RST     : in  std_logic;
      DIN     : in  std_logic_vector(7 downto 0);
      WREN    : in  std_logic;
      SAMP_EN : in  std_logic;
      DOUT    : out std_logic;
      READY   : out std_logic);
  end component;

  component uart_rx_rmm
    generic (
      SAMPS_PER_BAUD : positive := 16;
      BUFFER_DEPTH   : positive := 16);
    port (
      CLK      : in  std_logic;
      RST      : in  std_logic;
      DIN      : in  std_logic;
      SAMP_EN  : in  std_logic;
      READY    : out std_logic;
      RDEN     : in  std_logic;
      WORD_CNT : out std_logic_vector(31 downto 0);
      DOUT     : out std_logic_vector(7 downto 0));
  end component;

  -----------------------------------------------------------------------------
  -- Signals
  -----------------------------------------------------------------------------
  signal baud_cnt          : std_logic_vector(BAUD_CNT_WIDTH-1 downto 0) := (others => '0');
  signal baud_en           : std_logic;
  signal sRst              : std_logic;
  signal rx_rden           : std_logic;
  signal rx_buff_nempty    : std_logic;
  signal props_out_nmea_in : std_logic_vector(7 downto 0);
  signal props_out_byte_count : std_logic_vector(31 downto 0);
begin
  
  -- Type conversion for GPS IF outputs
  props_out.nmea_in <= char_t(props_out_nmea_in);

  ----------------------------------------------------------------------------
  -- Baud Count Enable
  ----------------------------------------------------------------------------
  process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if (baud_en = '1') or (ctl_in.reset = '1') then
        baud_cnt <= (others => '0');
      else
        baud_cnt <= std_logic_vector(unsigned(baud_cnt)+1);
      end if;
    end if;
  end process;

  baud_en <= '1' when (unsigned(baud_cnt) = BAUD_CNT_MAX) else '0';

  -- UART TX/RX Modules
  rx_uart : uart_rx_rmm
    generic map (
      SAMPS_PER_BAUD => 16,
      BUFFER_DEPTH   => 512)
    port map (
      CLK     => ctl_in.clk,
      RST     => ctl_in.reset,
      DIN     => rx,
      SAMP_EN => baud_en,
      READY   => rx_buff_nempty,
      RDEN    => rx_rden,
      WORD_CNT => props_out_byte_count,
      DOUT    => props_out_nmea_in);

  props_out.byte_count <= ulong_t(props_out_byte_count);
  rx_rden <= props_in.nmea_in_read when (rx_buff_nempty = '1') else '0';
  
  tx_uart : uart_tx_rmm
    generic map (
      SAMPS_PER_BAUD => 16,
      BUFFER_DEPTH   => 512)
    port map (
      CLK     => ctl_in.clk,
      RST     => ctl_in.reset,
      DIN     => std_logic_vector(props_in.nmea_out),
      WREN    => props_in.nmea_out_written,
      SAMP_EN => baud_en,
      DOUT    => tx,
      READY   => open);
  
end rtl;
