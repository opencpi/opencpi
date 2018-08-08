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
-- Automatic Gain Control
-------------------------------------------------------------------------------
--
-- File: agc.vhd
--
-- Description:
--
-- This is an implemenation of a simple automatic gain control (AGC) with fixed feedback
-- coefficient MU. Given an input signal with varying amplitude envelope, the AGC provides
-- a relatively constant amplitude output at the desired value REF.
--
-- The output signal is sampled and average over N samples, which is compared to a reference
-- level REF to measure the difference between the output and the desired value.
--   PeakDetector:
--                                  sN.15
--                            <------------------------<
--           s0.15            |                        |                s0.15
--   y(n) >----(abs)(R)------(+)-----(R)-------->(+)---(R)--(X)---(>>)(R)----> y_avg(n)
--                       |               sN.15    |-         |
--                       |                        |         1/N
--                       >-------(R[1:N+1])------>
--                         s0.15           s0.15
--
-- The gain is adjusted inversely proportional to the compared differences. The control
-- parameter MU is used to control AGC time constant which determines how fast the gain
-- changes take effect. The system latency is 3 clks via the main path or 9 clks via the
-- feedback path.
--   AGC:
--           s0.15          ss4.42                  s0.15          s0.15
--   x(n) >---(R)---(X)(R)----------------(>>)----------------------(R)----> y(n)
--                   |                                         |
--                   -------->         MU         REF      [PeakDet]
--                   |       |          |          |  _        |
--                   <---(R)(+)<----(R)(X)<----(R)(+)<----------
--                   s4.27     ss0.30     s0.15       s0.15
--
-- The following generic parameters define the AGC structure, these values must be set
-- before synthesis.
--   DATA_WIDTH : Width of the input and output data
--   NAVG       : Number of samples to average, can be same as sample freq
-- The following values control the AGC operation, these values are programmable thus can
-- be changed during operation.
--   REF : Desired output express in % of fullscale expected peak value in rms
--   MU  : Feedback coefficient, scale factor to the gain changes, express as mu*fullscale
--
-- This AGC implementation is a nonlinear, signal dependent feedback system. Users should
-- perform an empirical analysis using the Matlab model 'agc.m' to obtain the proper
-- parameters and control values.
--
-------------------------------------------------------------------------------
-- Revision Log:
-------------------------------------------------------------------------------
-- 02/22/16:
-- File Created
-------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity AGC is
  generic (
    DATA_WIDTH : integer := 16;  -- data width
    NAVG       : integer := 16   -- number of samples to average
  );
  port (
    CLK     : in  std_logic;
    RST     : in  std_logic;
    REG_WR  : in  std_logic;                               -- program control values
    REF     : in  std_logic_vector(DATA_WIDTH-1 downto 0); -- desired output
    MU      : in  std_logic_vector(DATA_WIDTH-1 downto 0); -- feedback error scale factor
    DIN_VLD : in  std_logic;                               -- indicates valid input data
    HOLD    : in  std_logic;                               -- maintain the current gain
    DIN     : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    DOUT    : out std_logic_vector(DATA_WIDTH-1 downto 0)
  );
end AGC;

architecture Structural of AGC is
  constant VGA_WIDTH  : integer := 3*DATA_WIDTH; -- ss4.42 [ss4.3*(DATA_WIDTH-6)]
  constant PDET_WIDTH : integer := DATA_WIDTH;   -- s0.15  [s0.(DATA_WIDTH-1)]
  constant MU_WIDTH   : integer := DATA_WIDTH;   -- s0.15, precision of error scale factor
  constant GAIN_WIDTH : integer := PDET_WIDTH+MU_WIDTH; -- s4.27 [s(GAIN_SIZE).(DATA_WIDTH-5)]
  constant GAIN_SIZE  : integer := 4;                   -- range 0..15
  constant GAIN_UNITY : integer := 2**(DATA_WIDTH-(GAIN_SIZE+1)); -- VGA gain=1 in s4.27

  -- variable gain amplifier
  signal agc_in  : signed(DATA_WIDTH-1 downto 0);
  signal vga_out : signed(VGA_WIDTH-1 downto 0);
  signal agc_out : signed(DATA_WIDTH-1 downto 0);

  -- peak detector
  type   det_dly is array(0 to NAVG) of signed(PDET_WIDTH-1 downto 0);
  signal loop_in  : signed(DATA_WIDTH-1 downto 0);
  signal pk_abs   : signed(PDET_WIDTH-1 downto 0);
  signal pk_acc   : signed(PDET_WIDTH+NAVG-1 downto 0);
  signal pk_sum, pk_sum_p   : signed(PDET_WIDTH+NAVG-1 downto 0);
  signal delay    : det_dly := (others=> (others=>'0'));
  signal pk_avg_f : signed(PDET_WIDTH+NAVG-1 downto 0);
  signal pk_avg   : signed(PDET_WIDTH-1 downto 0);

  -- gain calculation
  signal mu_coef    : signed(MU_WIDTH-1 downto 0);
  signal pk_ref     : signed(PDET_WIDTH-1 downto 0);
  signal err        : signed(PDET_WIDTH-1 downto 0);
  signal delta_gain : signed(PDET_WIDTH+MU_WIDTH-1 downto 0);
  signal gain       : signed(GAIN_WIDTH-1 downto 0);
  signal gain_d     : signed(GAIN_WIDTH-1 downto 0);

begin

  ---- data registers
  data_regs: process (CLK)
  begin
    if rising_edge(CLK) then
      if (RST='1') then
        agc_in <= (others=>'0');
        DOUT   <= (others=>'0');
      elsif (DIN_VLD='1') then
        agc_in <= signed(DIN);
        DOUT   <= std_logic_vector(agc_out);
      end if;
    end if;
  end process;

  ---- control registers
  ctrl_reg: process (CLK)
  begin
    if rising_edge(CLK) then
      if (RST='1') then
        pk_ref  <= (others=>'0');
        mu_coef <= (others=>'0');
      elsif (REG_WR='1') then
        pk_ref  <= signed(REF);    -- s0.15
        mu_coef <= signed(MU);     -- s0.15
      end if;
    end if;
  end process;

  -------------------------------------------------------------------------------
  -- measuring the output, average over N at every cycle,

  ---- abs(x) register
  loop_in <= agc_out;
  pdet_abs: process (CLK)
  begin
    if rising_edge(CLK) then
      if (RST='1') then
        pk_abs <= (others=>'0');
      elsif (DIN_VLD='1') then
        pk_abs <= abs(loop_in); -- s0.15
      end if;
    end if;
  end process;

  ---- sum of abs(x)
  pdet_accum: process (CLK)
  begin
    if rising_edge(CLK) then
      if (RST='1') then
        pk_acc <= (others=>'0');
      elsif (DIN_VLD='1') then
        pk_acc <= pk_abs + pk_sum; -- s0.15+sN.15
      end if;
    end if;
  end process;

  ---- peak detector N+1 delay line
  ---- WARNING: Attempts to force this into a SRL, by removing the reset
  ----          resulted in 'stale' data between executions.
  delay_line: process (CLK)
  begin
    if rising_edge(CLK) then
      if (RST='1') then
        delay <= (others=> (others=>'0'));
      elsif (DIN_VLD='1') then
        delay(0)         <= pk_abs;
        delay(1 to NAVG) <= delay(0 to NAVG-1);
      end if;
    end if;
  end process;

  ---- remove the oldest sample
  pk_sum <= pk_acc - delay(NAVG); -- sN.15-s0.15

  ---- extra pipeline to reduce setup time
  psum_pipe: process (CLK)
  begin
    if rising_edge(CLK) then
      if (RST='1') then
        pk_sum_p <= (others=>'0');
      elsif (DIN_VLD='1') then
        pk_sum_p <= pk_sum;
      end if;
    end if;
  end process;

  ---- peak detector output register, divide by N then resize to s0.15
  pk_avg_f <= pk_sum_p / NAVG;
  pdet_out: process (CLK)
  begin
    if rising_edge(CLK) then
      if (RST='1') then
        pk_avg <= (others=>'0');
      elsif (DIN_VLD='1') then   -- sign+15msb
        pk_avg <= pk_avg_f(PDET_WIDTH+NAVG-1) & pk_avg_f(PDET_WIDTH-2 downto 0);
      end if;
    end if;
  end process;

  -------------------------------------------------------------------------------
  -- computing loop gain, changes in gain are very small so precision must be maintained

  ---- loop error, err < pk_ref or pk_avg
  looperr: process (CLK)
  begin
    if rising_edge(CLK) then
      if (RST='1') then
        err <= (others=>'0');
      elsif (DIN_VLD='1') then
        err <= pk_ref - pk_avg; -- s0.15-s0.15
      end if;
    end if;
  end process;

  ---- calculate the delta gain, mu << 1 therefore delta_gain << err
  gaindelta: process (CLK)
  begin
    if rising_edge(CLK) then
      if (RST='1') then
        delta_gain <= (others=>'0');
      elsif (DIN_VLD='1') then
        delta_gain <= err * mu_coef; -- s0.15*s0.15=ss0.30,
      end if;
    end if;
  end process;

  ---- new loop gain
  previous_gain: process (CLK)
  begin
    if rising_edge(CLK) then
      if (RST='1') then
        gain_d <= to_signed(GAIN_UNITY,GAIN_WIDTH);
      elsif (DIN_VLD='1') then
        gain_d <= gain;  -- s4.27
      end if;
    end if;
  end process;

  ---- adaptive gain control
  gain_sel: process (delta_gain, gain_d, HOLD)
  begin
    if (HOLD='1') then    -- maintain the gain value
      gain <= gain_d;
    else                  -- keep adjusting the gain
      gain <= gain_d + shift_right(delta_gain,(GAIN_SIZE-1));  -- s4.27+sssss0.27=s4.27
    end if;
  end process;

  -------------------------------------------------------------------------------
  -- variable gain amplifier
  vga_mult: process (CLK)
  begin
    if rising_edge(CLK) then
      if (RST='1') then
        vga_out <= (others=>'0');
      elsif (DIN_VLD='1') then
        vga_out <= gain_d * agc_in; -- s4.27*s0.15=ss4.42
      end if;
    end if;
  end process;

  ---- scaling ss4.42 back to s0.15, ?? will it over/underflow ??
  ---- sign-bit & msb's of fractional part
  agc_out(DATA_WIDTH-1)          <= vga_out(VGA_WIDTH-1);
  agc_out(DATA_WIDTH-2 downto 0) <= vga_out(VGA_WIDTH-(GAIN_SIZE+3)
                                     downto VGA_WIDTH-(GAIN_SIZE+DATA_WIDTH+1));

end Structural;
