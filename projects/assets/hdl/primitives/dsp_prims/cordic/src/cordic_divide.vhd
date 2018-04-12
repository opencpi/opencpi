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
-- CORDIC Divider
-------------------------------------------------------------------------------
--
-- File: cordic_divide.vhd
--
-- Description:
--
-- CORDIC Function Implemented: Divide
--
-- Angle Mode = Linear
-- Direction Mode = Vectoring
-- Sequence = 0 to STAGES-1
-- Microrotation Angles: sigma(i) = 2^(-i)
-- Gain Correction Factor: None
--
-- Inputs:
--   X0 =  b
--   Y0 =  a
--   Z0 =  0
--
-- Results:
--   ZN = Y0/X0 = a/b
--
-- Domain:
--   |Y0/X0| <= 2
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity cordic_divide is
generic (
   DATA_WIDTH  : positive;
   STAGES      : positive);

port (
   CLK      : in std_logic;
   RST      : in std_logic;
   A        : in std_logic_vector(DATA_WIDTH-1 downto 0);
   B        : in std_logic_vector(DATA_WIDTH-1 downto 0);
   VLD_IN   : in std_logic;
   AOVERB   : out std_logic_vector(DATA_WIDTH-1 downto 0);
   VLD_OUT  : out std_logic);

end cordic_divide;

architecture behavior of cordic_divide is

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
      DIR_MODE    => "VECTORING")
   port map (
      CLK      => CLK,
      RST      => RST,
      X_IN     => B,
      Y_IN     => A,
      Z_IN     => zero,
      VLD_IN   => VLD_IN,
      X_OUT    => open,
      Y_OUT    => open,
      Z_OUT    => AOVERB,
      VLD_OUT  => VLD_OUT);

end behavior;

