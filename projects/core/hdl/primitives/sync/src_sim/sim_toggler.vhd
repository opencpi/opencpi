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
-- Simulation Bit Toggler Engine
-------------------------------------------------------------------------------
-- Creates a randomly toggling std_logic output to match a specified
-- aggregate duty cycle.
--
-- MODE = "PATTERN" or "RANDOM"
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity sim_toggler is

generic (

   DUTY_CYCLE   : real;
   PATT_LEN     : positive;
   MODE         : string);

port (

   CLK      : in std_logic;
   RST      : in std_logic;
   EN       : in std_logic;
   DOUT     : out std_logic := '0');

end sim_toggler;

architecture rtl of sim_toggler is

   constant COUNT_WIDTH   : integer := integer(ceil(log2(real(PATT_LEN))));
   constant T_PATTERN_ON  : integer := integer(round(real(PATT_LEN)*DUTY_CYCLE));

   signal count       : std_logic_vector(COUNT_WIDTH-1 downto 0) := (others => '0');

begin

   ----------------------------------------------------------------------------
   -- Counter : Defines pattern boundaries
   ----------------------------------------------------------------------------

   process(CLK)
   begin
      if rising_edge(CLK) then
         if (RST = '1') or (unsigned(count) = PATT_LEN-1) then
            count <= (others => '0');
         elsif (EN = '1') then
            count <= std_logic_vector(unsigned(count)+1);
         end if;
      end if;
   end process;

   ----------------------------------------------------------------------------
   -- Create output based on MODE
   ----------------------------------------------------------------------------

   bmode: if (MODE = "PATTERN") generate

      burstproc: process(CLK)
      begin
         if rising_edge(CLK) then
            if ( unsigned(count) < T_PATTERN_ON ) and ( EN = '1' ) then
               DOUT <= '1';
            else
               DOUT <= '0';
            end if;
         end if;
      end process;

   end generate;

   rmode: if (MODE = "RANDOM") generate

      randproc: process(CLK)
         variable seed1, seed2 : positive;
         variable rand : real;
      begin
         if rising_edge(CLK) then
            if (EN = '1') then
               uniform(seed1, seed2, rand);
               if (rand <= DUTY_CYCLE) then
                  DOUT <= '1';
               else
                  DOUT <= '0';
               end if;
            end if;
         end if;
      end process;

   end generate;

end rtl;

