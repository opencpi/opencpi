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
-- Cascaded Integrator-Comb (CIC) Interpolator
-------------------------------------------------------------------------------
--
-- Implements a Cascaded Integrator-Comb Decimator
-- References for theory of operation:
--
--     Whitepaper by Matthew P. Donadio
--     http://home.mit.bme.hu/~kollar/papers/cic.pdf
--
--     Understanding Digital Signal Processing, 2nd Ed, Ricard G. Lyons
--     Section 10.5 - Cascaded Integrator-Comb Filters
--
-- The generic parameters for this design are as follows:
--
--    N = Number of CIC stages
--    M = Differential delay for comb sections
--    R = Interpolation Factor
--    DIN_WIDTH = Input data width
--    DOUT_WIDTH = Output data width
--    ACC_WIDTH = Internal accumulator width
--
-- For proper operation, the accumulator width needs to be set according to
-- the following formula:
--
--    ACC_WIDTH >= CEIL(N*log2(R*M))+DIN_WIDTH
--
-- This will ensure that the accumulators can accommodate the maximum
-- difference between any two adjacent samples. Because of this and the use
-- of twos complement math, the comb sections will always compute the proper
-- difference from consecutive integrator outputs.
--
-- The DC gain of the CIC will be unity when R is a power of 2. A general
-- formula for the DC gain given N, M and R is:
--
--    CIC Gain = (R*M)^N / 2^CEIL(N*log2(R*M))
--
-------------------------------------------------------------------------------
-- Revision Log:
-------------------------------------------------------------------------------
-- 11/21/12:
-- File Created. Adapted from cic_dec.vhd
-- 08/19/15:
-- Changed all resets from async to sync
-- Change interpolator to accommodate DIN_VLD < 100%
-- 01/29/16:
-- Update comments, cleanup, use integ_vld drive DOUT_VLD
--------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity cic_int_gen is
  generic(
    N          : positive := 3;
    M          : positive := 1;
    R          : positive := 4;
    DIN_WIDTH  : positive := 16;
    ACC_WIDTH  : positive := 22;
    DOUT_WIDTH : positive := 16);
  port(
    CLK       : in  std_logic;
    RST       : in  std_logic;
    DIN_VLD   : in  std_logic;
    DIN       : in  std_logic_vector(DIN_WIDTH-1 downto 0);
    DOUT_EN   : in std_logic;
    DOUT_VLD  : out std_logic;
    DOUT      : out std_logic_vector(DOUT_WIDTH-1 downto 0));
end cic_int_gen;

architecture rtl of cic_int_gen is

  constant INT_CNT_WIDTH : integer := integer(ceil(log(real(N*R+N))/log(2.0)));

  type comb_t is array (0 to N) of std_logic_vector(ACC_WIDTH-1 downto 0);
  type comb_dly_t is array (0 to N, 0 to M) of std_logic_vector(ACC_WIDTH-1 downto 0);
  type integ_t is array (0 to N) of std_logic_vector(ACC_WIDTH-1 downto 0);

  signal comb      : comb_t;
  signal comb_dly  : comb_dly_t;
  signal vld_cnt   : unsigned(INT_CNT_WIDTH-1 downto 0);
  signal integ_vld : std_logic;
  signal integ     : integ_t;

begin

  ----------------------------------------------------------------------------
  -- Comb Stages: Operate at 1/R * CLK Rate via DIN_VLD
  ----------------------------------------------------------------------------

  comb(0)(ACC_WIDTH-1 downto DIN_WIDTH) <= (others => DIN(DIN_WIDTH-1));
  comb(0)(DIN_WIDTH-1 downto 0)         <= DIN;

  gen_comb : for i in 1 to N generate

    comb_i : process(CLK)
    begin
      if rising_edge(CLK) then
        if (RST = '1') then
          comb(i) <= (others => '0');
        elsif (DIN_VLD = '1') then
          comb(i) <= std_logic_vector(signed(comb(i-1)) - signed(comb_dly(i, M)));
        end if;
      end if;
    end process;

    comb_dly(i, 0) <= comb(i-1);

    gen_comb_dly : for j in 1 to M generate
      comb_dly_j : process(CLK)
      begin
        if rising_edge(CLK) then
          if (RST = '1') then
            comb_dly(i, j) <= (others => '0');
          elsif (DIN_VLD = '1') then
            comb_dly(i, j) <= comb_dly(i, j-1);
          end if;
        end if;
      end process;
    end generate;

  end generate;

  ----------------------------------------------------------------------------
  -- Set DOUT_VLD
  -- DOUT_VLD is set high after first sample of DIN plus latency
  ----------------------------------------------------------------------------

  proc_LatencyCount : process(CLK)
  begin
    if rising_edge(CLK) then
      if (RST = '1') then
        vld_cnt <= (others => '0');
      else
        if (DOUT_EN = '1' and vld_cnt = 0) then
          vld_cnt <= vld_cnt+1;
        elsif (vld_cnt > 0 and vld_cnt < N*R+N) then
          vld_cnt <= vld_cnt+1;
        end if;
      end if;
    end if;
  end process;

  integ_vld <= '1' when (vld_cnt = N*R+N and DOUT_EN = '1') else '0';

  ----------------------------------------------------------------------------
  -- Integrator Stages
  ----------------------------------------------------------------------------

  integ(0) <= comb(N);

  gen_integ : for i in 1 to N generate

    integ_i : process(CLK)
    begin
      if rising_edge(CLK) then
        if (RST = '1') then
          integ(i) <= (others => '0');
        elsif (DOUT_EN = '1') then
          integ(i) <= std_logic_vector(signed(integ(i)) + signed(integ(i-1)));
        end if;
      end if;
    end process;

  end generate;

  ----------------------------------------------------------------------------
  -- Final Output
  ----------------------------------------------------------------------------

  proc_oReg : process(CLK)
  begin
    if rising_edge(CLK) then
      if (RST = '1') then
        DOUT     <= (others => '0');
        DOUT_VLD <= '0';
      elsif (DOUT_EN = '1') then
        DOUT     <= integ(N)(ACC_WIDTH-1 downto ACC_WIDTH-DOUT_WIDTH);
        DOUT_VLD <= integ_vld;
      end if;
    end if;
  end process;
  
  -----------------------------------------------------------------------------
  
end rtl;
