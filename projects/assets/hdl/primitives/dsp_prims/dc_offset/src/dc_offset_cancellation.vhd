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
-- DC Offset Cancellation Filter
-------------------------------------------------------------------------------
--
-- Description:
--
-- The DC Offset Cancellation Filter calculates and removes a DC bias from a
-- stream of digitized input samples. The filter is based upon Richard G. Lyons'
-- "Understanding Digital Signal Processing, Third Edition" DC Removal circuit
-- found on Page 761. The text may also be found online here:
-- http://www.embedded.com/design/configurable-systems/4007653/DSP-Tricks-DC-Removal
--
-- Lyons' circuit in Figure 13-62d implements the transfer function given in
-- (13-118), which is a 1st-order IIR filter. From Lyons: "a zero resides at
-- z = 1 providing infinite attenuation at DC (zero Hz) and a pole at z = α
-- making the magnitude notch at DC very sharp. The closer α is to unity, the
-- narrower the frequency magnitude notch centered at zero Hz."
--
-- Adding a delay element to the output, y, does not change the transfer
-- function. Moving the single delay element following the output adder to two
-- delay elements feeding the output adder also does not change the transfer
-- function.
--
--                   (1 - z^-1)   [1,-1]          (actually [0,1,-1])
--    H(z) = z^-1 * ----------- = ------
--                  1 - (α)z^-1   [1,-α]
--
-- The multiplier output vector size (signal named 'product') is controlled by
-- defining α to be limited to eight bits. For a typical DIN DATA_WIDTH of 16
-- bits the multiplier results in a vector of 24 bits. The upper DATA_WIDTH
-- bits are added to the current input DIN. Since α is a signed 8 bit number
-- we define the time constant input TC = 128 * α. Rearranging the equation,
-- α = TC/128, where a maximum value of +127 is allowed due to signed
-- multiplication. Larger values of TC give both a faster filter response and a
-- narrower frequency magnitude notch at zero Hertz. A typical value of TC = 121
-- (α = 0.95) is used by default.
--
--   BYPASS >----------------------------*
--                                       |
--                                     |\|
--         *-----------------R---------|1\
--         |                           |  |--> DOUT
--   DIN >---(+)--------*----R---(+)---|0/
--            ^         |         ^-   |/
--            |         |         |
--            |  update-R         |
--            |         |         |
--            |         |         |
--            *---(X)---*----R----*
--                 |
--                 α
--
--     R   : Register
--    (X)  : Multiplier
--    (+)  : Adder
--
-- The input should be attenuated by as much as one bit to avoid overflow on
-- the output; i.e. the input should not be driven more than half-scale to
-- avoid overflow.
--
-- The circuit may be bypassed by asserting the BYPASS input. The circuit has a
-- latency of one DIN_VLD clock cycle.
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity dc_offset_cancellation is
  generic (
    DATA_WIDTH : positive := 16
    );
  port (
    CLK        : in  std_logic;
    RST        : in  std_logic;
    BYPASS     : in  std_logic;
    UPDATE     : in  std_logic;
    TC         : in  signed(7 downto 0);
    DIN        : in  signed(DATA_WIDTH-1 downto 0);
    DIN_VLD    : in  std_logic;
    DOUT_RDY   : in  std_logic := '1'; -- take the valid output even if no input, like AXI READY
    DOUT       : out signed(DATA_WIDTH-1 downto 0);
    DOUT_VLD   : out std_logic := '0'
    );
end dc_offset_cancellation;

architecture rtl of dc_offset_cancellation is

  constant ACC_PREC_c : positive := DATA_WIDTH + TC'length;

  signal din_r   : signed(DATA_WIDTH-1 downto 0) := (others => '0');
  signal a       : signed(DATA_WIDTH-1 downto 0) := (others => '0');
  signal a_r1    : signed(DATA_WIDTH-1 downto 0) := (others => '0');
  signal product : signed(ACC_PREC_c-1 downto 0) := (others => '0');
  signal a_r2    : signed(DATA_WIDTH-1 downto 0) := (others => '0');
  signal b_r1    : signed(DATA_WIDTH-1 downto 0) := (others => '0');
  signal y       : signed(DATA_WIDTH-1 downto 0) := (others => '0');

begin

  dc_filter : process (CLK)
  begin
    if rising_edge(CLK) then
      if (RST = '1') then
        DOUT_VLD <= '0';
        din_r    <= (others => '0');
        a_r1     <= (others => '0');
        a_r2     <= (others => '0');
        b_r1     <= (others => '0');
      else
        if (DIN_VLD = '1') then
          DOUT_VLD <= '1'; -- an input data always pushed out the output data
          din_r  <= DIN;
          a_r2   <= a;
          b_r1   <= a_r1;
          if (UPDATE = '1') then
            a_r1 <= a;
          end if;
        elsif DOUT_RDY = '1' then
          DOUT_VLD <= '0'; -- if no input, the output can be taken anyway
        end if;
      end if;
    end if;
  end process dc_filter;

  a       <= DIN + product(ACC_PREC_c-1 downto ACC_PREC_c-DATA_WIDTH);
  product <= TC * a_r1;
  y       <= a_r2 - b_r1;
  DOUT    <= y when (BYPASS = '0') else din_r;

end rtl;
