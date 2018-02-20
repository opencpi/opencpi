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
-- Generic Synchronous Cirular Buffer
-------------------------------------------------------------------------------
--
-- File: circ_buffer.vhd
--
-- Description:
--
-- This is a minimalist implementation of a synchronous FIFO. It is based on
-- the simple dual port ram design. The write and read port operations are
-- controlled by three counters:
--
-- Write pointer : Increments on valid writes and determines the address
--               : into the dual port ram where the next write access will
--               : be stored.
-- Read pointer  : Increments on valid reads and determines the address
--               : into the dual port ram where the next read access will
--               : be retrieved from.
-- Word counter  : Increments on valid writes when valid reads are not
--               : occurring and decrements on valid reads when valid
--               : writes are not occurring. If a valid write and read happen
--               : simultaneously, the word count remains the same; as it
--               : should. The counter tracks how many words are currently
--               : stored in the buffer.
--
-- Empty and Full flags are generated from the word count. Valid writes
-- and reads are also determined from the word count as follows:
--
-- Valid write   : When the write enable is asserted and the word counter is
--               : not equal to the buffer depth (buffer is not full).
-- Valid read    : When the read enable is asserted and the word counter is
--               : not equal to zero (buffer is not empty).
--
-------------------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity circ_buffer is

   generic(

      BUFF_WIDTH : positive := 16;
      BUFF_DEPTH : positive := 16;
      REG_OUTS   : boolean  := FALSE);

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

end circ_buffer;

architecture behavior of circ_buffer is

   constant ADDR_WIDTH : integer := integer(CEIL( LOG(real(BUFF_DEPTH)) / LOG(2.0) ));

   component ram_dp_simp
   generic (
      ADDR_WIDTH  : positive;
      DATA_WIDTH  : positive;
      REG_OUTS    : boolean);
   port (
      A_CLK   : in std_logic;
      A_WREN  : in std_logic;
      A_ADDR  : in std_logic_vector(ADDR_WIDTH-1 downto 0);
      A_WDATA : in std_logic_vector(DATA_WIDTH-1 downto 0);
      B_CLK   : in std_logic;
      B_RDEN  : in std_logic;
      B_ADDR  : in std_logic_vector(ADDR_WIDTH-1 downto 0);
      B_RDATA : out std_logic_vector(DATA_WIDTH-1 downto 0));
   end component;

   --BEL: Had to fix this to 32 bits so it could be used as a property :-(
   signal word_cnt_sig  : std_logic_vector(31 downto 0); -- One larger so we can count to BUFF_DEPTH
   signal valid_rd      : std_logic;
   signal valid_wr      : std_logic;
   signal wr_ptr        : std_logic_vector(ADDR_WIDTH-1 downto 0);
   signal rd_ptr        : std_logic_vector(ADDR_WIDTH-1 downto 0);

begin

   -- Don't allow reads from an empty buffer
   -- Don't allow writes to a full buffer

   valid_wr <= '1' when (WREN = '1') and (unsigned(word_cnt_sig) /= BUFF_DEPTH) else '0';
   valid_rd <= '1' when (RDEN = '1') and (unsigned(word_cnt_sig) /= 0) else '0';

   -- Track the Buffer's word count:
   -- Write w/o Read => Add 1 to count
   -- Read w/o Write => Subtract 1 from count
   -- Simultaneous Read & Write => No action, count stays the same

   --Export word count to output point
   WORD_CNT <= word_cnt_sig;
   
   process(CLK, RST)
   begin
      if (RST = '1') then
         word_cnt_sig <= (others => '0');
      elsif rising_edge(CLK) then
         if (valid_wr = '1') and (valid_rd = '0') then
            word_cnt_sig <= std_logic_vector(unsigned(word_cnt_sig) + 1);
         elsif (valid_rd = '1') and (valid_wr = '0') then
            word_cnt_sig <= std_logic_vector(unsigned(word_cnt_sig) - 1);
         end if;
      end if;
   end process;

   -- Generate Empty & Full Flags off word count

   FULL <= '1' when (unsigned(word_cnt_sig) = BUFF_DEPTH) else '0';
   EMPTY <= '1' when (unsigned(word_cnt_sig) = 0) else '0';

   -- Write Pointer
   -- Advance on valid writes
   -- Wrap at BUFF_DEPTH-1

   process(CLK, RST)
   begin
      if (RST = '1') then
         wr_ptr <= (others => '0');
      elsif rising_edge(CLK) then
         if (valid_wr = '1') and (unsigned(wr_ptr) = BUFF_DEPTH-1) then
            wr_ptr <= (others => '0');
         elsif (valid_wr = '1') then
            wr_ptr <= std_logic_vector(unsigned(wr_ptr) + 1);
         end if;
      end if;
   end process;

   -- Read Pointer
   -- Advance on valid reads
   -- Wrap at BUFF_DEPTH-1

   process(CLK, RST)
   begin
      if (RST = '1') then
         rd_ptr <= (others => '0');
      elsif rising_edge(CLK) then
         if (valid_rd = '1') and (unsigned(rd_ptr) = BUFF_DEPTH-1) then
            rd_ptr <= (others => '0');
         elsif (valid_rd = '1') then
            rd_ptr <= std_logic_vector(unsigned(rd_ptr) + 1);
         end if;
      end if;
   end process;

   -- Dual Port RAM : This will infer a Block RAM for the buffer

   buff_dp_ram: ram_dp_simp
   generic map (
      ADDR_WIDTH  => ADDR_WIDTH,
      DATA_WIDTH  => BUFF_WIDTH,
      REG_OUTS    => REG_OUTS)
   port map (
      A_CLK   => CLK,
      A_WREN  => valid_wr, --WREN,
      A_ADDR  => wr_ptr,
      A_WDATA => DIN,
      B_CLK   => CLK,
      B_RDEN  => valid_rd,
      B_ADDR  => rd_ptr,
      B_RDATA => DOUT);

end;

