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
-- Dual-Port RAM : Simple, One Write-Only Port, One Read-Only Port
-------------------------------------------------------------------------------
--
-- File: ram_dp_simp.vhd
--
-- Description:
--
-- This behavioral model of a Dual-Port Write-First RAM follows the example
-- given in Chapter 3 of the Xilinx XST User Guide : HDL Coding Techniques.
-- This version is configurable for both width and depth. The generic
-- parameters are self explanatory. If used properly, this module should infer
-- a block RAM in Xilinx Virtex devices when using XST.
--
-- The DPRAM depth will be defined by the ADDR_WIDTH parameter as follows:
--
--    MEM_DEPTH = 2^ADDR_WIDTH
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity ram_dp_simp is

generic(

   ADDR_WIDTH  : positive := 8;
   DATA_WIDTH  : positive := 16;
   REG_OUTS    : boolean := FALSE);

port (

   A_CLK   : in std_logic;
   A_WREN  : in std_logic;
   A_ADDR  : in std_logic_vector(ADDR_WIDTH-1 downto 0);
   A_WDATA : in std_logic_vector(DATA_WIDTH-1 downto 0);

   B_CLK   : in std_logic;
   B_RDEN  : in std_logic;
   B_ADDR  : in std_logic_vector(ADDR_WIDTH-1 downto 0);
   B_RDATA : out std_logic_vector(DATA_WIDTH-1 downto 0));

end ram_dp_simp;

architecture rtl of ram_dp_simp is

   type DPRAM_T  is array (0 to 2**ADDR_WIDTH-1) of std_logic_vector(DATA_WIDTH-1 downto 0);

   signal dpram       : DPRAM_T := (others => (others => '0'));
   signal b_out       : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal b_out_reg   : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal b_rden_dly  : std_logic;

begin

   a_port: process (A_CLK)
   begin
      if rising_edge(A_CLK) then
         if (A_WREN = '1') then
            dpram(to_integer(unsigned(A_ADDR))) <= A_WDATA;
         end if;
      end if;
   end process;

   b_port: process (B_CLK)
   begin
      if rising_edge(B_CLK) then
         b_out <= dpram(to_integer(unsigned(B_ADDR)));
      end if;
   end process;

   gen_regout: if (REG_OUTS) generate

      regb: process (B_CLK)
      begin
         if rising_edge(B_CLK) then
            b_rden_dly <= B_RDEN;
            if (b_rden_dly = '1') then
               b_out_reg <= b_out;
            end if;
         end if;
      end process;

      B_RDATA <= b_out_reg;

   end generate;

   gen_noregout: if not(REG_OUTS) generate

      b_rden_dly <= '0';
      b_out_reg <= (others => '0');
      B_RDATA <= b_out;

   end generate;

end rtl;

