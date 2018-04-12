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
-- Complex Mixer
-------------------------------------------------------------------------------
--
-- File: complex_mult.vhd
--
-- Description:
--
-- This module implements a complex multiplier. Given the following inputs:
--
--    a + jb   AND   c + jd
--
-- The complex product is computed as follows:
--
--    (a + jb) * (c + jd) = (ac - bd) + j(ad + bc)
--                        = (e  - f ) + j(g  + h )
--                        = x + jy
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;
library misc_prims; use misc_prims.all;

entity complex_mult is
  generic(
    A_WIDTH    : positive;
    B_WIDTH    : positive;
    PROD_WIDTH : positive); --PROD_WIDTH <= AWIDTH+BWIDTH
  port(
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
end complex_mult;

architecture rtl of complex_mult is

   signal a           : std_logic_vector(A_WIDTH-1 downto 0);
   signal b           : std_logic_vector(A_WIDTH-1 downto 0);
   signal c           : std_logic_vector(B_WIDTH-1 downto 0);
   signal d           : std_logic_vector(B_WIDTH-1 downto 0);
   signal e_pre       : std_logic_vector(A_WIDTH+B_WIDTH-1 downto 0);
   signal f_pre       : std_logic_vector(A_WIDTH+B_WIDTH-1 downto 0);
   signal g_pre       : std_logic_vector(A_WIDTH+B_WIDTH-1 downto 0);
   signal h_pre       : std_logic_vector(A_WIDTH+B_WIDTH-1 downto 0);
   signal e           : std_logic_vector(A_WIDTH+B_WIDTH-1 downto 0) := (others => '0');
   signal f           : std_logic_vector(A_WIDTH+B_WIDTH-1 downto 0) := (others => '0');
   signal g           : std_logic_vector(A_WIDTH+B_WIDTH-1 downto 0) := (others => '0');
   signal h           : std_logic_vector(A_WIDTH+B_WIDTH-1 downto 0) := (others => '0');
   signal x_pre       : std_logic_vector(A_WIDTH+B_WIDTH-1 downto 0);
   signal y_pre       : std_logic_vector(A_WIDTH+B_WIDTH-1 downto 0);
   signal x           : std_logic_vector(PROD_WIDTH-1 downto 0);
   signal y           : std_logic_vector(PROD_WIDTH-1 downto 0);
  -- temp signals for old VHDL
   signal xrnd_in     : std_logic_vector(A_WIDTH + B_WIDTH-2 downto 0);
   signal yrnd_in     : std_logic_vector(A_WIDTH + B_WIDTH-2 downto 0);
begin

   a <= DIN_A_RE;
   b <= DIN_A_IM;
   c <= DIN_B_RE;
   d <= DIN_B_IM;

   e_pre <= std_logic_vector( signed(a) * signed(c) );
   f_pre <= std_logic_vector( signed(b) * signed(d) );
   g_pre <= std_logic_vector( signed(a) * signed(d) );
   h_pre <= std_logic_vector( signed(b) * signed(c) );

   process(CLK)
   begin
      if rising_edge(CLK) then
        if (DIN_VLD = '1') then
          e <= e_pre;
          f <= f_pre;
          g <= g_pre;
          h <= h_pre;
        end if;
      end if;
   end process;

   x_pre <= std_logic_vector( signed(e) - signed(f) ); 
   y_pre <= std_logic_vector( signed(g) + signed(h) ); 

   xrnd_in <= x_pre(A_WIDTH + B_WIDTH-2 downto 0); -- drop extra sign bit from mult
   yrnd_in <= y_pre(A_WIDTH + B_WIDTH-2 downto 0); -- drop extra sign bit from mult
   
   xrnd: misc_prims.misc_prims.round_conv
   generic map (
      DIN_WIDTH   => A_WIDTH + B_WIDTH -1,
      DOUT_WIDTH  => PROD_WIDTH)
   port map (
      CLK      => CLK,
      RST      => RST,
      DIN      => xrnd_in,
      DIN_VLD  => DIN_VLD,
      DOUT     => x,
      DOUT_VLD => open);

   yrnd: misc_prims.misc_prims.round_conv
   generic map (
      DIN_WIDTH   => A_WIDTH + B_WIDTH -1,
      DOUT_WIDTH  => PROD_WIDTH)
   port map (
      CLK      => CLK,
      RST      => RST,
      DIN      => yrnd_in, 
      DIN_VLD  => DIN_VLD,
      DOUT     => y,
      DOUT_VLD => open);

   DOUT_RE <= x;
   DOUT_IM <= y;

   DOUT_VLD <= DIN_VLD;

end;

