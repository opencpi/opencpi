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
-- CORDIC STAGE
-------------------------------------------------------------------------------
--
-- File: cordic_stage.vhd
--
-- Description:
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity cordic_stage is
generic (

   DATA_WIDTH  : positive;
   DIR_MODE    : string   := "ROTATION";
   ANGLE_MODE  : string   := "CIRCULAR");

port (

   CLK      : in std_logic;
   RST      : in std_logic;
   X_IN     : in std_logic_vector(DATA_WIDTH-1 downto 0);
   Y_IN     : in std_logic_vector(DATA_WIDTH-1 downto 0);
   Z_IN     : in std_logic_vector(DATA_WIDTH-1 downto 0);
   VLD_IN   : in std_logic;
   SHIFT    : in integer;
   SIGMA    : in std_logic_vector(DATA_WIDTH-1 downto 0);
   X_OUT    : out std_logic_vector(DATA_WIDTH-1 downto 0);
   Y_OUT    : out std_logic_vector(DATA_WIDTH-1 downto 0);
   Z_OUT    : out std_logic_vector(DATA_WIDTH-1 downto 0);
   VLD_OUT  : out std_logic);

end cordic_stage;

architecture behavior of cordic_stage is

--   signal pt5             : std_logic_vector(DATA_WIDTH-1 downto 0);
--   signal x_plus_pt5      : std_logic_vector(DATA_WIDTH-1 downto 0);
--   signal y_plus_pt5      : std_logic_vector(DATA_WIDTH-1 downto 0);
--   signal x_rnd_sat       : std_logic_vector(DATA_WIDTH-1 downto 0);
--   signal y_rnd_sat       : std_logic_vector(DATA_WIDTH-1 downto 0);

   signal mu              : integer;
   signal delta           : integer;
   signal x_add_ctrl      : integer;
   signal y_add_ctrl      : integer;
   signal z_add_ctrl      : integer;

   signal x_shift         : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal x_reg           : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal y_shift         : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal y_reg           : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal z_reg           : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal vld_reg         : std_logic;

   
begin

   ---------------------------------------------------------
   -- Shift the X and Y inputs by SHIFT bits
   ---------------------------------------------------------

   -- Use round(num) = trunc(num+0.5) with post-shift LSB as radix point

-- pt5 <= std_logic_vector(to_signed(2**(SHIFT-1),DATA_WIDTH)) when (SHIFT > 0) else (others => '0');
--
-- x_plus_pt5 <= std_logic_vector( signed(X_IN) + signed(pt5) );
-- y_plus_pt5 <= std_logic_vector( signed(Y_IN) + signed(pt5) );
--
-- -- Saturation for the +0.5 result
-- -- Compare sign of result to original value. Saturate if a sign flip occurred
--
-- process (x_plus_pt5, X_IN)
-- begin
--    if (x_plus_pt5(DATA_WIDTH-1) = X_IN(DATA_WIDTH-1)) then
--       x_rnd_sat <= x_plus_pt5;
--    else
--       x_rnd_sat(DATA_WIDTH-1) <= X_IN(DATA_WIDTH-1);
--       x_rnd_sat(DATA_WIDTH-2 downto 0) <= (others => not(X_IN(DATA_WIDTH-1)));
--    end if;
-- end process;
--
-- process (y_plus_pt5, Y_IN)
-- begin
--    if (y_plus_pt5(DATA_WIDTH-1) = Y_IN(DATA_WIDTH-1)) then
--       y_rnd_sat <= y_plus_pt5;
--    else
--       y_rnd_sat(DATA_WIDTH-1) <= Y_IN(DATA_WIDTH-1);
--       y_rnd_sat(DATA_WIDTH-2 downto 0) <= (others => not(Y_IN(DATA_WIDTH-1)));
--    end if;
-- end process;

--x_rnd_sat <= X_IN;
--y_rnd_sat <= Y_IN;

--x_rnd_sat <= x_plus_pt5;
--y_rnd_sat <= y_plus_pt5;

   -- Truncate and shift for the result (for later: may want to round)

   x_shift <= std_logic_vector(shift_right(signed(X_IN), SHIFT));
   y_shift <= std_logic_vector(shift_right(signed(Y_IN), SHIFT));

   --------------------------------------------------------------------------------
   -- Universal CORDIC Equations:                                                --
   --                                                                            --
   --   x(i+1) = x(i) - mu * delta(i) * y(i) * 2^(-i)                            --
   --   y(i+1) = y(i) + 1  * delta(i) * x(i) * 2^(-1)                            --
   --   z(i+1) = z(i) - 1  * delta(i) * sigma(i)                                 --
   --                                                                            --
   --------------------------------------------------------------------------------
   --                       |                  Direction Mode                    --
   --                       |------------------------------------------------------
   --                       | Rotation                | Vectoring                --
   --                       | delta(i)=sgn(z(i))      | delta(i)=-sgn(y(i))      --
   -- Angle Mode            | z => 0                  | y => 0                   --
   --------------------------------------------------------------------------------
   -- Circular              | xn=K(xcosz-ysinz)       | xn=Ksqrt(x^2+y^2)        --
   -- mu=1                  | yn=K(ycosz+xsinz)       | yn=0                     --
   -- sigma(i)=atan(2^-i)   | zn=0                    | zn=z+atan(y/x)           --
   --------------------------------------------------------------------------------
   -- Linear                | xn=x                    | xn=x                     --
   -- mu=0                  | yn=y+xz                 | yn=0                     --
   -- sigma(i)=2^i          | zn=0                    | zn=z+y/x                 --
   --------------------------------------------------------------------------------
   -- Hyperbolic            | xn=K'(xcoshz-ysinhz)    | xn=Ksqrt(x^2-y^2)        --
   -- mu=-1                 | yn=K'(ycoshz-xsinhz)    | yn=0                     --
   -- sigma(i)=atanh(2^-i)  | zn=0                    | zn=z+atanh(y/x)          --
   --------------------------------------------------------------------------------

   mu_circ: if (ANGLE_MODE = "CIRCULAR") generate
      mu <= 1;
   end generate;

   mu_lin: if (ANGLE_MODE = "LINEAR") generate
      mu <= 0;
   end generate;

   mu_hyp: if (ANGLE_MODE = "HYPERBOLIC") generate
      mu <= -1;
   end generate;

   -- Rotation Mode : delta = sgn( z(i) )

   d_rot: if (DIR_MODE = "ROTATION") generate

      process (Z_IN)
      begin
         if (signed(Z_IN) = 0) then
            delta <= 1;  -- force to 1 instead of 0 to avoid lockup
         elsif (Z_IN(DATA_WIDTH-1) = '0') then
            delta <= 1;
         else
            delta <= -1;
         end if;
      end process;

   end generate;

   -- Vectoring Mode : delta = -sgn( y(i) )

   d_vect: if (DIR_MODE = "VECTORING") generate

      process (Y_IN)
      begin
         if (signed(Y_IN) = 0) then
            delta <= 0;
         elsif (Y_IN(DATA_WIDTH-1) = '0') then
            delta <= -1;
         else
            delta <= 1;
         end if;
      end process;

   end generate;

   ---------------------------------------------------------
   -- Instantiate the X, Y, and Z adders
   ---------------------------------------------------------

   x_add_ctrl <= -mu * delta;
   y_add_ctrl <= delta;
   z_add_ctrl <= -delta;

   PIPELINEREG: process(CLK)
   begin
      if (CLK'event and CLK = '1') then
         if (RST = '1') then
           vld_reg <= '0';
           x_reg   <= (others => '0');
           y_reg   <= (others => '0');
           z_reg   <= (others => '0');
         elsif (VLD_IN = '1') then
           vld_reg <= '1';
           if (x_add_ctrl = 1) then
             x_reg <= std_logic_vector( signed(X_IN) + signed(y_shift) );
           elsif (x_add_ctrl = -1) then
             x_reg <= std_logic_vector( signed(X_IN) - signed(y_shift) );
           else
             x_reg <= X_IN;
           end if;

           if (y_add_ctrl = 1) then
             y_reg <= std_logic_vector( signed(Y_IN) + signed(x_shift) );
           elsif (y_add_ctrl = -1) then
             y_reg <= std_logic_vector( signed(Y_IN) - signed(x_shift) );
           else
             y_reg <= Y_IN;
           end if;

           if (z_add_ctrl = 1) then
             z_reg <= std_logic_vector( signed(Z_IN) + signed(SIGMA) );
           elsif (z_add_ctrl = -1) then
             z_reg <= std_logic_vector( signed(Z_IN) - signed(SIGMA) );
           else
             z_reg <= Z_IN;
           end if;
         else
            vld_reg <= '0';
         end if;
      end if;
   end process;

   X_OUT   <= x_reg;
   Y_OUT   <= y_reg;
   Z_OUT   <= z_reg;
   VLD_OUT <= vld_reg;

end behavior;
