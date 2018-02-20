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
-- Convergent Rounder
-------------------------------------------------------------------------------
--
-- File: round_conv.vhd
--
-- Description:
-- This component implements a convergent rounder that rounds up when the
-- fractional portion is greater than 0.5 or the fractional portion is equal to
-- 0.5 and the lsb of the non-fractional portion is set.
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity round_conv is
generic (
   DIN_WIDTH   : positive;
   DOUT_WIDTH  : positive);

port (
   CLK      : in std_logic;
   RST      : in std_logic;
   DIN      : in std_logic_vector(DIN_WIDTH-1 downto 0);
   DIN_VLD  : in std_logic;
   DOUT     : out std_logic_vector(DOUT_WIDTH-1 downto 0) := (others => '0');
   DOUT_VLD : out std_logic);

end round_conv;

architecture behavior of round_conv is
   constant FRAC_WIDTH_c : integer := DIN_WIDTH-DOUT_WIDTH;
   function PT5_c return std_logic_vector is
     variable v : std_logic_vector(FRAC_WIDTH_c-1 downto 0) := (others => '0');
   begin
     v(FRAC_WIDTH_c-1) := '1';
     return v;
   end PT5_c;
--   picky mode of modelsim doesn't like this
--   constant PT5_c : std_logic_vector(FRAC_WIDTH_c-1 downto 0) := (FRAC_WIDTH_c-1 => '1', others => '0');
   signal frac  : std_logic_vector(FRAC_WIDTH_c-1 downto 0);
   signal intg  : std_logic_vector(DOUT_WIDTH-1 downto 0);
   signal lsb   : std_logic;
   signal inc   : std_logic;

begin

   frac <= DIN(FRAC_WIDTH_c-1 downto 0);
   intg <= DIN(DIN_WIDTH-1 downto FRAC_WIDTH_c);
   lsb  <= DIN(FRAC_WIDTH_c);

   -- determine whether to round up or not
   process(frac, lsb)
   begin
      if (frac > PT5_c) or ((frac = PT5_c) and (lsb = '1')) then
         inc <= '1';
      else
         inc <= '0';
      end if;
   end process;

   -- implement the round up by adding one
   process(CLK)
   begin
      if rising_edge(CLK) then
        if (RST = '1') then
          DOUT <= (others => '0');
        else
          DOUT_VLD <= DIN_VLD;
          if (DIN_VLD = '1') then
            if (inc = '1') then
              DOUT <= std_logic_vector(signed(intg)+1);
            else
              DOUT <= intg;
            end if;
          end if;
        end if;
      end if;
   end process;

end;

