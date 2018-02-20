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

-------------------------------------------------------------------------------
-- UART TX Module
-------------------------------------------------------------------------------
--
-- File: uart_rx_rmm.vhd
--
-- Description:
--
-- This module implements the transmit portion of a UART interface. It is
-- modeled after the uart_tx6 module used with Xilinx PicoBlaze (KCPSM6). The
-- uart_tx6 module was designed as a minimalist solution using Xilinx
-- primitives. While this is nice from a resource utilization standpoint, it
-- restricts its use to certain Xilinx devices. While not quite as resource
-- efficient, this design corrects a few weaknesses in the uart_tx6 design.
-- The advantages of uart_rx_rmm over uart_tx6 are:
--
-- * Ability to define samples per baud as a generic parameter
-- * Ability to define buffer depth as a generic parameter
-- * Protection against overflow in buffer. Writing to a full buffer in
--   uart_tx6 will corrupt the buffer contents and move teh write pointer!
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity uart_rx_rmm is

generic (

   SAMPS_PER_BAUD  : positive := 16;
   BUFFER_DEPTH    : positive := 16);

port (

   CLK         : in std_logic;
   RST         : in std_logic;
   DIN         : in std_logic;
   SAMP_EN     : in std_logic;
   READY       : out std_logic;
   RDEN        : in std_logic;
   WORD_CNT    : out std_logic_vector(31 downto 0);
   DOUT        : out std_logic_vector(7 downto 0));

end uart_rx_rmm;

architecture rtl of uart_rx_rmm is

   constant SAMPS_PER_BAUD_OVER2 : integer := integer( real(SAMPS_PER_BAUD) / 2.0 );

   component circ_buffer
   generic(
      BUFF_WIDTH : positive;
      BUFF_DEPTH : positive;
      REG_OUTS   : boolean);
   port(
     CLK      : in  std_logic;
     RST      : in  std_logic;
     WREN     : in  std_logic;
     DIN      : in  std_logic_vector(BUFF_WIDTH-1 downto 0);
     RDEN     : in  std_logic;
     DOUT     : out std_logic_vector(BUFF_WIDTH-1 downto 0);
     WORD_CNT : out std_logic_vector(31 downto 0);
     EMPTY    : out std_logic;
     FULL     : out std_logic);
   end component;

   signal din_z1            : std_logic;
   signal din_z2            : std_logic;
   signal din_z3            : std_logic;
   signal din_fall          : std_logic;
   signal din_sync          : std_logic;
   signal start_rcvd        : std_logic;
   signal delay_cnt         : std_logic_vector(15 downto 0);
   signal delay_done        : std_logic;
   signal samp_cnt          : std_logic_vector(15 downto 0);
   signal capture_bit_pre   : std_logic;
   signal capture_bit       : std_logic;
   signal bit_cnt           : std_logic_vector(3 downto 0);
   signal bit_en            : std_logic_vector(7 downto 0);
   signal done              : std_logic;
   signal captured_data     : std_logic_vector(7 downto 0);
   signal valid_wren_pre    : std_logic;
   signal valid_wren        : std_logic;
   signal buff_empty        : std_logic;
   signal buff_full         : std_logic;

begin

   ----------------------------------------------------------------------------
   -- Sync DIN and Detect Start Bit
   ----------------------------------------------------------------------------

   process (CLK, RST)
   begin
      if (RST = '1') then
         din_z1 <= '0';
         din_z2 <= '0';
         din_z3 <= '0';
      elsif rising_edge(CLK) then
         din_z1 <= DIN;
         din_z2 <= din_z1;
         din_z3 <= din_z2;
      end if;
   end process;

   din_fall <= '1' when ((din_z3 = '1') and (din_z2 = '0')) else '0';
   din_sync <= din_z3;

   ----------------------------------------------------------------------------
   -- START Delay Count
   ----------------------------------------------------------------------------

   process (CLK, RST)
   begin
      if (RST = '1') then
         start_rcvd <= '0';
      elsif rising_edge(CLK) then
         if (done = '1') then
            start_rcvd <= '0';
         elsif (din_fall = '1') then
            start_rcvd <= '1';
         end if;
      end if;
   end process;

   process (CLK, RST)
   begin
      if (RST = '1') then
         delay_cnt <= (others => '0');
      elsif rising_edge(CLK) then
         if (done = '1') then
            delay_cnt <= (others => '0');
         elsif (delay_done = '0') and (start_rcvd = '1') and (SAMP_EN = '1') then
            delay_cnt <= std_logic_vector(unsigned(delay_cnt)+1);
         end if;
      end if;
   end process;

   delay_done <= '1' when (unsigned(delay_cnt) = SAMPS_PER_BAUD_OVER2) else '0';

   ----------------------------------------------------------------------------
   -- Sample Counter
   ----------------------------------------------------------------------------

   process (CLK, RST)
   begin
      if (RST = '1') then
         samp_cnt <= (others => '0');
      elsif rising_edge(CLK) then
         if (capture_bit = '1') then
            samp_cnt <= (others => '0');
         elsif (delay_done = '1') and (SAMP_EN = '1') then
            samp_cnt <= std_logic_vector(unsigned(samp_cnt)+1);
         end if;
      end if;
   end process;

   capture_bit_pre <= '1' when (unsigned(samp_cnt) = SAMPS_PER_BAUD-1) and (SAMP_EN = '1') else '0';

   process (CLK, RST)
   begin
      if (RST = '1') then
         capture_bit <= '0';
      elsif rising_edge(CLK) then
         capture_bit <= capture_bit_pre;
      end if;
   end process;

   ----------------------------------------------------------------------------
   -- Bit Counter
   ----------------------------------------------------------------------------

   process (CLK, RST)
   begin
      if (RST = '1') then
         bit_cnt <= (others => '0');
      elsif rising_edge(CLK) then
         if (done = '1') then
            bit_cnt <= (others => '0');
         elsif (capture_bit = '1') then
            bit_cnt <= std_logic_vector(unsigned(bit_cnt)+1);
         end if;
      end if;
   end process;

   done <= '1' when (unsigned(bit_cnt) = 7) and (capture_bit = '1') else '0';

   ----------------------------------------------------------------------------
   -- Bit Capture
   ----------------------------------------------------------------------------

   gen_out_bits: for i in 0 to 7 generate

      gen_en: bit_en(i) <= '1' when (unsigned(bit_cnt) = i) else '0';

      gen_bit: process (CLK, RST)
      begin
         if (RST =  '1') then
            captured_data(i) <= '0';
         elsif rising_edge(CLK) then
            if (bit_en(i) = '1') and (capture_bit = '1') then
               captured_data(i) <= din_sync;
            end if;
         end if;
      end process;

   end generate;

   ----------------------------------------------------------------------------
   -- Store Captured Words
   ----------------------------------------------------------------------------

   valid_wren_pre <= '1' when (done = '1') and (buff_full = '0') else '0';

   process (CLK, RST)
   begin
      if (RST = '1') then
         valid_wren <= '0';
      elsif rising_edge(CLK) then
         valid_wren <= valid_wren_pre;
      end if;
   end process;

   buff_out: circ_buffer
   generic map (
      BUFF_WIDTH => 8,
      BUFF_DEPTH => BUFFER_DEPTH,
      REG_OUTS   => FALSE)  -- FWFT Mode
   port map (
      CLK   => CLK,
      RST   => RST,
      WREN  => valid_wren,
      DIN   => captured_data,
      RDEN  => RDEN,
      DOUT  => DOUT,
      WORD_CNT => WORD_CNT,
      EMPTY => buff_empty,
      FULL  => buff_full);

   READY <= '1' when (buff_empty = '0') else '0';

end rtl;

