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

-- The Peak Detection component functions with both signed and unsigned input
-- values. The absolute value of the two inputs are compared against each other
-- to determine which value is greater. This result is compared against a stored
-- maximum value and the maximum value is replaced if either input is greater
-- than the current value. The result is returned as an unsigned value. The
-- current maximum value may be cleared by resetting the component.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;


entity peakDetect is
port(
  CLK_IN      : in  std_logic;
  RST_IN      : in  std_logic;
  EN_IN       : in  std_logic;
  A_IN        : in  std_logic_vector(15 downto 0);
  B_IN        : in  std_logic_vector(15 downto 0);
  PEAK_OUT    : out std_logic_vector(15 downto 0)
);
end entity peakDetect;


architecture rtl of peakDetect is
  signal ab_en,dat_en        : std_logic;
  signal a,b,dat,max         : signed(15 downto 0);

begin

  process(CLK_IN)
  begin
    if(rising_edge(CLK_IN)) then
      if(RST_IN = '1') then
        ab_en  <= '0';
        dat_en <= '0';
        dat    <= (others => '0');
        max    <= (others => '0');
      else
        -----------------------------
        -- Abs() of a and b inputs --
        -----------------------------
        ab_en <= EN_IN;
        if(A_IN(15) = '0') then
          a <= signed(A_IN);
        else
          a <= signed(not A_IN);
        end if;

        if(B_IN(15) = '0') then
          b <= signed(B_IN);
        else
          b <= signed(not B_IN);
        end if;
        --------------------------------------
        -- Use max of abs() of a or b input --
        --------------------------------------
        dat_en <= ab_en;
        if(ab_en = '1') then
          if(a > b) then
            dat <= a;
          else
            dat <= b;
          end if;
        end if;
        ---------------------
        -- Keep Peak value --
        ---------------------
        if(dat_en = '1') then
          if(dat > max) then
            max <= dat;
          end if;
        end if;

      end if; --rst

    end if; --clk
  end process;


PEAK_OUT <= std_logic_vector(max);

end architecture;

