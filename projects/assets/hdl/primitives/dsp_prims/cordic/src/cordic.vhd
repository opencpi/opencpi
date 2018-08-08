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
-- CORDIC
-------------------------------------------------------------------------------
--
-- File: cordic.vhd.vhd
--
-- Description:
--
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity cordic is
generic (
   DATA_WIDTH  : positive := 16;
   STAGES      : positive := 9;
   ANGLE_MODE  : string   := "CIRCULAR";
   DIR_MODE    : string   := "ROTATION");

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

end cordic;

architecture behavior of cordic is

   component cordic_stage
   generic (
      DATA_WIDTH  : positive;
      ANGLE_MODE  : string   := "CIRCULAR";
      DIR_MODE    : string   := "ROTATION");
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
   end component;

   type sequence_t is array (0 to STAGES-1) of integer;
   type sigma_t is array (0 to STAGES-1) of std_logic_vector(DATA_WIDTH-1 downto 0);
   type data_t is array (0 to STAGES) of std_logic_vector(DATA_WIDTH-1 downto 0);
   type accum_t is array (0 to STAGES) of std_logic_vector(DATA_WIDTH-1 downto 0);

   signal sequence      : sequence_t := (others => 0);
   signal sigma         : sigma_t;
   signal sigma_cpy     : sigma_t;
   signal xi            : data_t;
   signal yi            : data_t;
   signal zi            : accum_t;

begin

   ----------------------------------------------------------------------------
   -- Fill the 'sequence' Array
   ----------------------------------------------------------------------------

   -- The array named 'sequence' establishes the indexing used at each
   -- CORDIC iteration for looking up the proper value of sigma, which
   -- is used in the Z accumulator equation. To guarantee convergence,
   -- certain indices are repeated in the hyperbolic angle mode. In the
   -- other two modes, the sequence proceeds linearly from 0 to
   -- STAGES-1.
   --
   -- For the hyperbolic mode, indices that match the following series
   -- are repeated: 4, 13, 40, ... (3^i-1)/2 for i=2,3,4...

   -- The following 3 generate blocks will all be resolved static at build time
   -- They are not intended to synthesize logic, just to init 'sequence' with
   -- the proper values based on ANGLE_MODE.

   gen_seq_lin: if ANGLE_MODE = "LINEAR" generate
      seq_lin: for i in 0 to STAGES-1 generate
         sequence(i) <= i;
      end generate;
   end generate;

   gen_seq_cir: if ANGLE_MODE = "CIRCULAR" generate
      seq_cir: for i in 0 to STAGES-1 generate
         sequence(i) <= i;
      end generate;
   end generate;

   gen_seq_hyp: if ANGLE_MODE = "HYPERBOLIC" generate
      seq_hyp_proc: process
         variable val, repeat, repeat_index  : integer;
      begin
         repeat := 4;
         repeat_index := 2;
         val := 1;
         for i in 0 to STAGES-1 loop
            sequence(i) <= val;
            if (val /= repeat) then
               val := val + 1;
            else
               repeat_index := repeat_index + 1;
               repeat := integer( (3.0**real(repeat_index) - 1.0) / 2.0 );
            end if;
         end loop;
         wait; -- Needed to prevent infinite loop in simulation
      end process;
   end generate;

   ----------------------------------------------------------------------------
   -- Fill the 'sigma' Array
   ----------------------------------------------------------------------------

   -- ANGLE_MODE  SIGMA
   -- LINEAR      2^(N-1) * 2^(-i) for i = 0 to STAGES-1
   -- CIRCULAR    2^(N-1) * atan(2^(-i)) for i = 0 to STAGES-1
   -- HYPERBOLIC  2^(N-1) * atanh(2^(-i)) for i = 0 to STAGES-1
   --
   -- For Hyperbolic Mode, more values than are actually used will be
   -- generated when STAGES >= 5 due to the repeats in the sequence
   -- array.

   sigma_gen: for i in 0 to STAGES-1 generate

      sigma_lin: if ANGLE_MODE = "LINEAR" generate
         sigma(i) <= std_logic_vector(to_unsigned(integer( 2.0**(DATA_WIDTH-2) * 0.5**i ), DATA_WIDTH));
      end generate;

      sigma_cir: if ANGLE_MODE = "CIRCULAR" generate
         sigma(i) <= std_logic_vector(to_unsigned( integer( 2.0**(DATA_WIDTH-2) * arctan(2.0**(-i)) ), DATA_WIDTH ));
      end generate;

      sigma_hyp: if ANGLE_MODE = "HYPERBOLIC" generate
         sigma(i) <= std_logic_vector(to_unsigned( integer( 2.0**(DATA_WIDTH-2) * arctanh(2.0**(-i)) ), DATA_WIDTH));
      end generate;

   end generate;

   ---------------------------------------------------------
   -- Instantiate the CORDIC stages
   ---------------------------------------------------------

   xi(0) <= X_IN;
   yi(0) <= Y_IN;
   zi(0) <= Z_IN;

   gen_stages: for i in 0 to STAGES-1 generate

      sigma_cpy(i) <= sigma(sequence(i));

      stage: cordic_stage
      generic map(
         DATA_WIDTH  => DATA_WIDTH,
         ANGLE_MODE  => ANGLE_MODE,
         DIR_MODE    => DIR_MODE)
      port map(
         CLK      => CLK,
         RST      => RST,
         X_IN     => xi(i),
         Y_IN     => yi(i),
         Z_IN     => zi(i),
         VLD_IN   => VLD_IN,
         SHIFT    => sequence(i),
         SIGMA    => sigma_cpy(i),
         X_OUT    => xi(i+1),
         Y_OUT    => yi(i+1),
         Z_OUT    => zi(i+1),
         VLD_OUT  => open);

   end generate;

   ---------------------------------------------------------
   -- Assign Outputs
   ---------------------------------------------------------

   X_OUT   <= xi(STAGES);
   Y_OUT   <= yi(STAGES);
   Z_OUT   <= zi(STAGES);
   VLD_OUT <= VLD_IN;

end behavior;

