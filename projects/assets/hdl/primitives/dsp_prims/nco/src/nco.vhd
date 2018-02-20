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
-- Numerically-Controlled Oscillator (NCO)
-------------------------------------------------------------------------------
--
-- File: nco.vhd
--
-- Description:
--
-- This module implements a numerically controlled oscillator using a simple
-- phase accumulator fed into a polar-to-rectangular CORDIC (cordic_pr), which
-- performs the necessary Phase-to-Amplitude Conversion (PAC). The CORDIC
-- produces both the Cosine (Inphase) and Sine (Quadrature) components of the
-- NCO output.
--
-- When enabled by the EN input, the NCO will produce valid output samples
-- from valid inputs following the propagation delay as determined by the
-- number of stages used in the CORDIC. The number of stages will determine
-- the accuracy of the final I/Q outputs at roughly one bit of accuracy for
-- each stage. There is no VLD_IN input to this module since it is assumed
-- that when EN is high, we are requesting that a valid output be produced.
--
-- The MAG and PHS_INC inputs will determine the behavior of the NCO in the
-- following manner:
--
--    phase = phase + PHS_INC
--    I_OUT = MAG*cos(phase)
--    Q_OUT = MAG*sin(phase)
--
-- On each enabled rising CLK edge, a new phase value will be created by the
-- phase accumulator. This value and the MAG input will be fed into the
-- CORDIC which will produce a valid I/Q Output pair STAGES+1 clocks later.
-- The total propagation delay for the NCO will be STAGES+2 clocks.
--
-- The PHS_INIT port is provided so the user can instantiate multiple NCO's
-- with a fixed phase offset between them. Asserting the RST pin will set the
-- phase accumulator back to PHS_INIT. If it is unspecified, PHS_INIT is
-- assumed to be zero.
--
-- If f_clk is the frequency of the CLK input, then the output frequency of
-- the NCO is determined by the following equation:
--
--    freq = [ PHS_INC / 2^(PHS_ACC_WIDTH) ] * f_clk
--
-- So for a given f_clk and desired freq, the PHS_INC should be set as
-- follows:
--
--    PHS_INC = freq * 2^(PHS_ACC_WIDTH) / f_clk
--
-- Keep in mind that PHS_INC is a signed quantity and its MSB is the sign bit.
-- Setting a negative PHS_INC will produce a phasor rotation corresponding to
-- a negative complex frequency.
--
-- Generic parameters:
--
-- DATA_WIDTH     : Width of the data path. Applies to MAG and defines the
--                : data width for the CORDIC and I/Q Outputs.
-- PHS_INC_WIDTH  : Phase Increment Width
-- PHS_ACC_WIDTH  : Phase Accumulator Width
-- CORDIC_STAGES  : Number of stages to use in the CORDIC. Determines the
--                : accuracy of the final result as well as the propagation
--                : delay for the NCO.
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity nco is
generic (
   DATA_WIDTH     : positive;
   PHS_INC_WIDTH  : positive;
   PHS_ACC_WIDTH  : positive;
   CORDIC_STAGES  : positive);
port (
   CLK      : in std_logic;
   RST      : in std_logic;
   EN       : in std_logic;
   MAG      : in std_logic_vector(DATA_WIDTH-1 downto 0);
   PHS_INC  : in std_logic_vector(PHS_INC_WIDTH-1 downto 0);
   PHS_INIT : in std_logic_vector(PHS_ACC_WIDTH-1 downto 0) := (others => '0');
   I_OUT    : out std_logic_vector(DATA_WIDTH-1 downto 0);
   Q_OUT    : out std_logic_vector(DATA_WIDTH-1 downto 0);
   VLD_OUT  : out std_logic);
end nco;

architecture behavior of nco is

  component cordic_pr
    generic (
      DATA_WIDTH : positive;
      DATA_EXT   : positive;
      STAGES     : positive);
    port (
      CLK     : in  std_logic;
      RST     : in  std_logic;
      MAG     : in  std_logic_vector(DATA_WIDTH-1 downto 0);
      PHASE   : in  std_logic_vector(DATA_WIDTH-1 downto 0);
      VLD_IN  : in  std_logic;
      I       : out std_logic_vector(DATA_WIDTH-1 downto 0);
      Q       : out std_logic_vector(DATA_WIDTH-1 downto 0);
      VLD_OUT : out std_logic);
  end component;
  
  signal phs_accum     : std_logic_vector(PHS_ACC_WIDTH-1 downto 0);

begin
  
   ----------------------------------------------------------------------------
   -- Phase Accumulator
   ----------------------------------------------------------------------------
   process (CLK)
   begin
      if rising_edge(CLK) then
        if (RST = '1') then
          phs_accum <= PHS_INIT;
        elsif (EN = '1') then
          phs_accum <= std_logic_vector(signed(phs_accum) + signed(PHS_INC));
        end if;
      end if;
   end process;

   ----------------------------------------------------------------------------
   -- Phase-to-Amplitude Conversion
   ----------------------------------------------------------------------------
   cordicInst: cordic_pr
   generic map (
      DATA_WIDTH  => DATA_WIDTH,
      DATA_EXT    => 6,
      STAGES      => CORDIC_STAGES)
   port map (
      CLK      => CLK,
      RST      => RST,
      MAG      => MAG,
      PHASE    => phs_accum(PHS_ACC_WIDTH-1 downto PHS_ACC_WIDTH-DATA_WIDTH),
      VLD_IN   => EN,
      I        => I_OUT,
      Q        => Q_OUT,
      VLD_OUT  => VLD_OUT);
   
end behavior;
