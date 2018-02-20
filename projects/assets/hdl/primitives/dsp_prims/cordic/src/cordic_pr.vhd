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
-- CORDIC : Polar-to-Rectangular Conversion
-------------------------------------------------------------------------------
--
-- File: cordic_pr.vhd
--
-- Description:
--
-- CORDIC Function Implemented: Polar-to-Rectangular Conversion
--
-- Angle Mode = Circular
-- Direction Mode = Rotation
-- Sequence = 0 to STAGES-1
-- Microrotation Angles: sigma(i) = atan(2^(-i))
-- Gain Correction Factor: Kc = prod(cos(sigma(i)))
--
-- Inputs:
--   X0 = Kc * Mag
--   Y0 = 0
--   Z0 = Phase
--
-- Results:
--   XN = Mag * cos(Phase)
--   YN = Mag * sin(Phase)
--
-- Domain:
--   |X0| <= pi/2
--   |Z0| <= pi/2
--
-- Input Scaling:
--
-- The input phase can range from -pi to +pi. It is assumed that the input
-- is prescaled by 2^(DATA_WIDTH-1)/pi so that a max negative value
-- corresponds to -pi and a max positive value corrsponds to +pi-delta
-- where delta = 2*pi/2^(DATA_WIDTH).
--
-- The phase input will be scaled inside this module by pi/4*2^(DATA_WIDTH-1)
-- or pi*2^(DATA_WIDTH-3). This will allow it to be used directly in the
-- adjustments described below which involve arithmetic with pi
--
-- Input Adjustments:
--
-- To guarantee CORDIC convergence in this mode, we need to ensure that the
-- absolute value of the input phase is less than or equal to pi/2. If it is
-- greater than pi/2, the input vector is in either quadrant 2 or 3. In this
-- case we will mirror the vector back into quadrant 1 or 4 respectively.
-- This is accomplished by subtracting the phase from pi (quadrant 2 to 1),
-- or from -pi (quadrant 3 to 4).
--
-- If the phase is adjusted we keep track of that fact using the flip_dly
-- vector and apply a compensating adjustment at the CORDIC output for the
-- I component.
--
-- Output Adjustment:
--
-- If flip_dly(STAGES) indicates that the corresponding input vector has been
-- mirrored (flipped), we simply negate the I component of the CORDIC output
-- to compensate for that.
--
-- In summary:
--
--   Phase In (p)           Phase Adjust (p')   CORDIC Out
--   ====================   =================   ================
--   p = (-pi   : -pi/2)    p' = -pi-p          I = -XN, Q = ZN
--   p = (-pi/2 : +pi/2)    p' = p              I =  XN, Q = ZN
--   p = (+pi/2 : +pi  )    p' = pi-p           I = -XN, Q = ZN
--
-- Phase input to this module is signed and scaled to Pi. Granularity is
-- therefore delta = 2*Pi/2^(DATA_WIDTH-1). So a max negative value for PHASE
-- corresponds to -Pi. And a max positive value for PHASE corresponds to
-- Pi - delta.
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;
library misc_prims; use misc_prims.all;

entity cordic_pr is
generic (
   DATA_WIDTH  : positive;
   DATA_EXT    : positive;
   STAGES      : positive);

port (
   CLK      : in std_logic;
   RST      : in std_logic;
   MAG      : in std_logic_vector(DATA_WIDTH-1 downto 0);
   PHASE    : in std_logic_vector(DATA_WIDTH-1 downto 0);
   VLD_IN   : in std_logic;
   I        : out std_logic_vector(DATA_WIDTH-1 downto 0) := (others => '0');
   Q        : out std_logic_vector(DATA_WIDTH-1 downto 0) := (others => '0');
   VLD_OUT  : out std_logic);

end cordic_pr;

architecture behavior of cordic_pr is

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

   component round_conv
   generic (
      DIN_WIDTH   : positive;
      DOUT_WIDTH  : positive);
   port (
      CLK      : in std_logic;
      RST      : in std_logic;
      DIN      : in std_logic_vector(DIN_WIDTH-1 downto 0);
      DIN_VLD  : in std_logic;
      DOUT     : out std_logic_vector(DOUT_WIDTH-1 downto 0);
      DOUT_VLD : out std_logic);
   end component;

   ----------------------------------------------------------------------------
   -- Calculate K for CORDIC Instance
   ----------------------------------------------------------------------------

   -- The value K is used to correct for CORDIC gain. It depends on the user
   -- parameters: STAGES, DIR_MODE, and ANGLE_MODE. The function below
   -- calculates K when ANGLE_MODE is CIRCULAR. Since it depends only upon
   -- static values it will not result in an infinite loop for simulation. And
   -- for synthesis it will reduce to a static value assignment for K with no
   -- logic inferred. The floor function is used so gain won't produce
   -- overflows for a full scale input to the cordic.

   function  k_calc (
      DATA_WIDTH  : integer;
      STAGES      : integer)
   return std_logic_vector is
      variable k_real : real;
      variable k : std_logic_vector(DATA_WIDTH-1 downto 0);
   begin
      k_real := 1.0;
      for i in 0 to STAGES-1 loop
         k_real := k_real * cos(arctan(2.0**(-i)));
      end loop;
      k := std_logic_vector(to_signed(integer(floor((2.0**(DATA_WIDTH-2)-1.0) * k_real)),DATA_WIDTH));
      return k;
   end function k_calc;

   ----------------------------------------------------------------------------

   constant DW_EXT        : integer := DATA_WIDTH + DATA_EXT;
   constant ZERO          : std_logic_vector(DW_EXT-1 downto 0) := (others => '0');
   constant PI            : std_logic_vector(DW_EXT-1 downto 0) := std_logic_vector(to_signed(integer(2.0**(DW_EXT-3)*(MATH_PI)),DW_EXT));
   constant NEG_PI        : std_logic_vector(DW_EXT-1 downto 0) := std_logic_vector(to_signed(integer(2.0**(DW_EXT-3)*(-MATH_PI)),DW_EXT));
   constant PI_OVER_2     : std_logic_vector(DW_EXT-1 downto 0) := std_logic_vector(to_signed(integer(2.0**(DW_EXT-3)*(MATH_PI/2.0)),DW_EXT));
   constant NEG_PI_OVER_2 : std_logic_vector(DW_EXT-1 downto 0) := std_logic_vector(to_signed(integer(2.0**(DW_EXT-3)*(-MATH_PI/2.0)),DW_EXT));

   type pha_rot_sel_t is (LOW, MID, HIGH);

   signal mag_ext                : std_logic_vector(DW_EXT-1 downto 0);
   signal pha_ext                : std_logic_vector(DW_EXT-1 downto 0);
   signal K                      : std_logic_vector(DW_EXT-1 downto 0);
   signal mag_k_long             : std_logic_vector(2*DW_EXT-1 downto 0) := (others => '0');
   signal mag_k_pre              : std_logic_vector(DW_EXT-1 downto 0);
   signal mag_k                  : std_logic_vector(DW_EXT-1 downto 0) := (others => '0');
   signal pi_minus_pha           : std_logic_vector(DW_EXT-1 downto 0);
   signal neg_pi_minus_pha       : std_logic_vector(DW_EXT-1 downto 0);
   signal pha_rot_sel            : pha_rot_sel_t;
   signal pha_scaled_long        : std_logic_vector(2*DW_EXT-1 downto 0) := (others => '0');
   signal pha_scaled             : std_logic_vector(DW_EXT-1 downto 0);
   signal pha_rot_pre            : std_logic_vector(DW_EXT-1 downto 0);
   signal pha_rot                : std_logic_vector(DW_EXT-1 downto 0) := (others => '0');
   signal flip_pre               : std_logic;
   signal flip                   : std_logic := '0';
   signal flip_dly               : std_logic_vector(STAGES downto 0) := (others => '0');
   signal cordic_x               : std_logic_vector(DW_EXT-1 downto 0);
   signal cordic_x_neg           : std_logic_vector(DW_EXT-1 downto 0);
   signal cordic_y               : std_logic_vector(DW_EXT-1 downto 0);
   signal cordic_vld             : std_logic;
   signal x_fix                  : std_logic_vector(DW_EXT-1 downto 0);
   signal y_fix                  : std_logic_vector(DW_EXT-1 downto 0);
   signal x_rnd                  : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal y_rnd                  : std_logic_vector(DATA_WIDTH-1 downto 0);

begin

   ----------------------------------------------------------------------------
   -- Calculate K for CORDIC Instance
   ----------------------------------------------------------------------------

   K <= k_calc ( DW_EXT , STAGES );

   ----------------------------------------------------------------------------
   -- Sign extend to internal vector length
   ----------------------------------------------------------------------------

   mag_ext(DW_EXT-1 downto DATA_EXT) <= MAG;
   mag_ext(DATA_EXT-1 downto 0) <= (others => '0');

   pha_ext(DW_EXT-1 downto DATA_EXT) <= PHASE;
   pha_ext(DATA_EXT-1 downto 0) <= (others => '0');

   ----------------------------------------------------------------------------
   -- Apply adjustments and pipeline register added due to timing issues
   -- observed on Zynq-based platforms @ 100MHz.
   ----------------------------------------------------------------------------

   process(CLK)
   begin
      if rising_edge(CLK) then
        if (VLD_IN = '1') then
          mag_k_long       <= std_logic_vector(signed(mag_ext) * signed(K));
          pha_scaled_long  <= std_logic_vector(signed(pha_ext) * signed(PI));
        end if;
      end if;
   end process;

   ----------------------------------------------------------------------------
   -- Scale Mag to K:
   ----------------------------------------------------------------------------

   mag_k_pre <= mag_k_long(2*DW_EXT-3 downto DW_EXT-2);

   ----------------------------------------------------------------------------
   -- Input Phase ranges from max negative (-pi) to max positive (+pi)
   -- Scale it by PI so that
   -- Max positive value ==> +Pi (minus one delta)
   -- Max negative value ==> -Pi
   ----------------------------------------------------------------------------

   pha_scaled  <= pha_scaled_long(2*DW_EXT-2 downto DW_EXT-1);

   ----------------------------------------------------------------------------
   -- Pre-rotate Phase so that |Phase| <= pi/2 for CORDIC Convergence
   ----------------------------------------------------------------------------

   pi_minus_pha <= std_logic_vector(signed(PI) - signed(pha_scaled));
   neg_pi_minus_pha <= std_logic_vector(signed(NEG_PI) - signed(pha_scaled));

   process(pha_scaled)
   begin
      if (signed(pha_scaled) < signed(NEG_PI_OVER_2)) then
         pha_rot_sel <= LOW;
      elsif (signed(pha_scaled) < signed(PI_OVER_2)) then
         pha_rot_sel <= MID;
      else
         pha_rot_sel <= HIGH;
      end if;
   end process;

   process( pha_rot_sel,
            pha_scaled,
            pi_minus_pha,
            neg_pi_minus_pha )
   begin
      case (pha_rot_sel) is

         when LOW  =>
            pha_rot_pre <= std_logic_vector(shift_left(signed(neg_pi_minus_pha),1));
            flip_pre <= '1';

         when MID  =>
            pha_rot_pre <= std_logic_vector(shift_left(signed(pha_scaled),1));
            flip_pre <= '0';

         when HIGH =>
            pha_rot_pre <= std_logic_vector(shift_left(signed(pi_minus_pha),1));
            flip_pre <= '1';

      end case;
   end process;

   mag_k <= mag_k_pre;
   pha_rot <= pha_rot_pre;
   flip <= flip_pre;

   ----------------------------------------------------------------------------
   -- Create a delay line for "flip" to match prop delay of CORDIC
   ----------------------------------------------------------------------------

   flip_dly(0) <= flip;

   gen_flip_dly: for i in 0 to STAGES-1 generate
      flip_ff: process(CLK)
      begin
         if rising_edge(CLK) then
           if (VLD_IN = '1') then
            flip_dly(i+1) <= flip_dly(i);
           end if;
         end if;
      end process;
   end generate;

   ----------------------------------------------------------------------------
   -- Instantiate the CORDIC
   ----------------------------------------------------------------------------

   cordic_inst: cordic
   generic map (
      DATA_WIDTH  => DW_EXT,
      STAGES      => STAGES,
      ANGLE_MODE  => "CIRCULAR",
      DIR_MODE    => "ROTATION")
   port map (
      CLK      => CLK,
      RST      => RST,
      X_IN     => mag_k,
      Y_IN     => ZERO,
      Z_IN     => pha_rot,
      VLD_IN   => VLD_IN,
      X_OUT    => cordic_x,
      Y_OUT    => cordic_y,
      Z_OUT    => open,
      VLD_OUT  => cordic_vld);

   cordic_x_neg <= std_logic_vector(-signed(cordic_x));

   -- Inphase: Flip the X component when indicated by phase pre-rotate logic
   x_fix <= cordic_x when (flip_dly(STAGES) = '0') else cordic_x_neg;

   -- Quadrature: Always the Y component
   y_fix <= cordic_y;

   -- Round off to DATA_WIDTH bits

   rndx: round_conv
   generic map (
      DIN_WIDTH   => DW_EXT,
      DOUT_WIDTH  => DATA_WIDTH)
   port map (
      CLK      => CLK,
      RST      => RST,
      DIN      => x_fix,
      DIN_VLD  => cordic_vld,
      DOUT     => x_rnd,
      DOUT_VLD => open);

   rndy: round_conv
   generic map (
      DIN_WIDTH   => DW_EXT,
      DOUT_WIDTH  => DATA_WIDTH)
   port map (
      CLK      => CLK,
      RST      => RST,
      DIN      => y_fix,
      DIN_VLD  => cordic_vld,
      DOUT     => y_rnd,
      DOUT_VLD => open);

   I <= x_rnd;
   Q <= y_rnd;
   VLD_OUT <= VLD_IN;

end behavior;

