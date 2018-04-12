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
-- CORDIC Multiplier
-------------------------------------------------------------------------------
--
-- File: cordic_mult.vhd
--
-- Description:
--
-- CORDIC Function Implemented: Multiply
--
-- Angle Mode = Linear
-- Direction Mode = Rotation
-- Sequence = 0 to STAGES-1
-- Microrotation Angles: sigma(i) = 2^(-i)
-- Gain Correction Factor: None
--
-- Inputs:
--   X0 = a
--   Y0 = 0
--   Z0 = b
--
-- Results:
--   YN = X0*Z0 = a*b
--
-- Domain:
--   |X0| <= 2
--   |Z0| <= 2
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity cordic_mult is
generic (
   DATA_WIDTH  : positive := 16;
   STAGES      : positive := 9);

port (
   CLK      : in std_logic;
   RST      : in std_logic;
   A        : in std_logic_vector(DATA_WIDTH-1 downto 0);
   B        : in std_logic_vector(DATA_WIDTH-1 downto 0);
   VLD_IN   : in std_logic;
   AB       : out std_logic_vector(DATA_WIDTH-1 downto 0);
   VLD_OUT  : out std_logic);

end cordic_mult;

architecture behavior of cordic_mult is

   component cordic
   generic (
      DATA_WIDTH  : positive;
      STAGES      : positive;
      ANGLE_MODE  : string  ;
      DIR_MODE    : string  );
   port (
      CLK      : in std_logic;
      RST      : in std_logic;
      X_IN     : in std_logic_vector(DATA_WIDTH-1 downto 0);
      Y_IN     : in std_logic_vector(DATA_WIDTH-1 downto 0);
      Z_IN     : in std_logic_vector(DATA_WIDTH-1 downto 0);
      VLD_IN   : in std_logic;
      X_OUT    : out std_logic_vector(DATA_WIDTH-1 downto 0);
      Y_OUT    : out std_logic_vector(DATA_WIDTH-1 downto 0);
      Z_OUT    : out std_logic_vector(DATA_WIDTH-1 downto 0);
      VLD_OUT  : out std_logic);
   end component;

   signal zero    : std_logic_vector(DATA_WIDTH-1 downto 0);

begin

   ---------------------------------------------------------
   -- Instantiate the CORDIC
   ---------------------------------------------------------

   zero <= (others => '0');

   cordic_inst: cordic
   generic map (
      DATA_WIDTH  => DATA_WIDTH,
      STAGES      => STAGES,
      ANGLE_MODE  => "LINEAR",
      DIR_MODE    => "ROTATION")
   port map (
      CLK      => CLK,
      RST      => RST,
      X_IN     => A,
      Y_IN     => zero,
      Z_IN     => B,
      VLD_IN   => VLD_IN,
      X_OUT    => open,
      Y_OUT    => AB,
      Z_OUT    => open,
      VLD_OUT  => VLD_OUT);

end behavior;

