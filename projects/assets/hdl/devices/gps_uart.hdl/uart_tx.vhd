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
-- File: uart_tx_rmm.vhd
--
-- Description:
--
-- This module implements the transmit portion of a UART interface. It is
-- modeled after the uart_tx6 module used with Xilinx PicoBlaze (KCPSM6). The
-- uart_tx6 module was designed as a minimalist solution using Xilinx
-- primitives. While this is nice from a resource utilization standpoint, it
-- restricts its use to certain Xilinx devices. While not quite as resource
-- efficient, this design corrects a few weaknesses in the uart_tx6 design.
-- The advantages of uart_tx_rmm over uart_tx6 are:
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

entity uart_tx_rmm is

generic (

   SAMPS_PER_BAUD  : positive := 16;
   BUFFER_DEPTH    : positive := 16);

port (

   CLK         : in std_logic;
   RST         : in std_logic;
   DIN         : in std_logic_vector(7 downto 0);
   WREN        : in std_logic;
   SAMP_EN     : in std_logic;
   DOUT        : out std_logic;
   READY       : out std_logic);

end uart_tx_rmm;

architecture rtl of uart_tx_rmm is

   component circ_buffer
   generic(
      BUFF_WIDTH : positive;
      BUFF_DEPTH : positive;
      REG_OUTS   : boolean);
   port(
      CLK   : in std_logic;
      RST   : in std_logic;
      WREN  : in std_logic;
      DIN   : in std_logic_vector(BUFF_WIDTH-1 downto 0);
      RDEN  : in std_logic;
      DOUT  : out std_logic_vector(BUFF_WIDTH-1 downto 0);
      EMPTY : out std_logic;
      FULL  : out std_logic);
   end component;

   type state_t is (
      IDLE  ,
      START ,
      BIT0  ,
      BIT1  ,
      BIT2  ,
      BIT3  ,
      BIT4  ,
      BIT5  ,
      BIT6  ,
      BIT7  ,
      STOP  ,
      DLY1  ,
      DLY2  );

   signal cstate          : state_t;
   signal nstate          : state_t;

   signal buff_rden       : std_logic;
   signal buff_dout       : std_logic_vector(7 downto 0);
   signal buff_empty      : std_logic;
   signal buff_full       : std_logic;
   signal dout_pre        : std_logic;
   signal samp_cnt        : std_logic_vector(15 downto 0);
   signal next_bit        : std_logic;

begin

   ----------------------------------------------------------------------------
   -- Input Buffer
   ----------------------------------------------------------------------------

   buff_in: circ_buffer
   generic map (
      BUFF_WIDTH => 8,
      BUFF_DEPTH => BUFFER_DEPTH,
      REG_OUTS   => FALSE)  -- FWFT Mode
   port map (
      CLK   => CLK,
      RST   => RST,
      WREN  => WREN,
      DIN   => DIN,
      RDEN  => buff_rden,
      DOUT  => buff_dout,
      EMPTY => buff_empty,
      FULL  => buff_full);

   buff_rden <= '1' when (cstate = BIT7) and (next_bit = '1') else '0';
   READY <= not(buff_full);

   ----------------------------------------------------------------------------
   -- State Machine
   ----------------------------------------------------------------------------

   fsm_seq: process(CLK, RST)
   begin
      if (RST = '1') then
         cstate <= IDLE;
      elsif rising_edge(CLK) then
         if (next_bit = '1') then
            cstate <= nstate;
         end if;
      end if;
   end process;

   fsm_comb: process(cstate, buff_empty, buff_dout)
   begin

      -- FSM Output Defaults

      dout_pre <= '1';

      -- Next State Logic / Output Assertions

      case (cstate) is

      when IDLE =>
         if (buff_empty = '0') then
            nstate <= START;
         else
            nstate <= IDLE;
         end if;

      when START => dout_pre <= '0'         ; nstate <= BIT0;
      when BIT0  => dout_pre <= buff_dout(0); nstate <= BIT1;
      when BIT1  => dout_pre <= buff_dout(1); nstate <= BIT2;
      when BIT2  => dout_pre <= buff_dout(2); nstate <= BIT3;
      when BIT3  => dout_pre <= buff_dout(3); nstate <= BIT4;
      when BIT4  => dout_pre <= buff_dout(4); nstate <= BIT5;
      when BIT5  => dout_pre <= buff_dout(5); nstate <= BIT6;
      when BIT6  => dout_pre <= buff_dout(6); nstate <= BIT7;
      when BIT7  => dout_pre <= buff_dout(7); nstate <= STOP;
      when STOP  => dout_pre <= '1'         ; nstate <= DLY1;
      when DLY1  => dout_pre <= '1'         ; nstate <= DLY2;
      when DLY2  => dout_pre <= '1'         ;
         if (buff_empty = '0') then
            nstate <= START;
         else
            nstate <= IDLE;
         end if;

      --when STOP  =>
      --   if (buff_empty = '0') then
      --      nstate <= START;
      --   else
      --      nstate <= IDLE;
      --   end if;

      end case;

   end process;

   ----------------------------------------------------------------------------
   -- Sample Counter
   ----------------------------------------------------------------------------

   process (CLK, RST)
   begin
      if (RST = '1') then
         samp_cnt <= (others => '0');
         next_bit <= '0';
      elsif rising_edge(CLK) then
         if (SAMP_EN = '1') then
            if (unsigned(samp_cnt) = SAMPS_PER_BAUD-1) then
               next_bit <= '1';
               samp_cnt <= (others => '0');
            else
               samp_cnt <= std_logic_vector(unsigned(samp_cnt)+1);
            end if;
         else
            next_bit <= '0';
         end if;
      end if;
   end process;

   ----------------------------------------------------------------------------
   -- Register final output
   ----------------------------------------------------------------------------

   process (CLK, RST)
   begin
      if (RST = '1') then
         DOUT <= '1';
      elsif rising_edge(CLK) then
         DOUT <= dout_pre;
      end if;
   end process;

end rtl;

