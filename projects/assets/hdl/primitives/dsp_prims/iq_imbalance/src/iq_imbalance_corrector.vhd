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
-- IQ Imbalance Corrector
-------------------------------------------------------------------------------
--
-- Description:
--
-- The IQ Imbalance Corrector worker compensates for amplitude and phase
-- differences between quadrature I and Q input rails caused by differences in
-- quadrature A/D devices. The goal is to drive the corrected output power
-- difference between rails to zero and to also drive the corrected phase
-- difference between rails to zero. This is accomplished via a feedback loop
-- where the power and phase differences are measured and applied to the Q rail.
-- In other words, the I output is a delayed version of the I input while the
-- Q output has been corrected by removing amplitude and phase errors. This
-- results in removal of the input spectral image.
--
-- The power difference is measured by I^2 - Q^2, while the phase difference is
-- measured by I*Q. These loop errors are averaged over 2^LOG2_AVG_LEN-1 input
-- samples, which defines the update interval. The loop errors are filtered by
-- single-pole IIR filters at the end of the update interval. The filters have
-- a pole on the unit circle and have gain of 2^-NLOG2_LOOP_GAIN. The filter
-- outputs become the current c_corr and d_corr error correction values that
-- are applied to the I and Q input streams, respectively. Input values are
-- presented as corrected output values following four DIN_VLD cycles. This
-- circuit operates at the full clock rate - that is, DIN_VLD may be held
-- asserted every clock cycle.
--
-- The ENABLE input is used to either enable (true) or disable (false) the
-- circuit. Note that bypass registers are not used. Instead, feedback error
-- values are held constant at values that do not alter the Q rail. The UPDATE
-- input, by default true, may be disabled to hold the loop errors to a
-- constant value. Note that the ENABLE input takes priority over the UPDATE
-- input.
--
-- Theory:
--
-- A mathematical representation of a quadrature signal is given by the
-- formula cos(2pi*t/T)+jsin(2pi*t/T). If we let w=2pi*t/T, the equation of an
-- imbalanced quadrature signal is cos(w)+jAsin(w+B), where A is the amplitude
-- difference and B is the phase difference between I/Q rails. If we take the
-- imbalanced signal and mix it with another frequency, such as upconverting or
-- downconverting the signal, we essentially multiply the input signal by the
-- tone cos(w0)+jsin(w0). The output signal then becomes a frequency-shifted
-- version of the input signal given by cos(w-w0)+jAsin(w-w0+B). The result is
-- a spectral image of the original signal at -(w-w0). In the case of a non-
-- zero linear combination of sine and cosine waves (which we have from a
-- quadrature A/D, where the Q rail is simply a phase shift of pi/2 of the I
-- rail), we may use the trigonometric identity Asin(x+B) = Ccos(x) + Dsin(x),
-- where C and D are constants and are the phase and amplitude errors applied
-- to the I and Q input values, respectively. Thus the corrected output rails
-- become I and CI+DQ.
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;
library misc_prims;

entity iq_imbalance_corrector is
  generic (
    DATA_WIDTH : positive := 12;
    ACC_PREC   : positive := 38
    );
  port (
    CLK             : in  std_logic;
    RST             : in  std_logic;
    ENABLE          : in  std_logic;
    UPDATE          : in  std_logic;
    LOG2_AVG_LEN    : in  unsigned(4 downto 0);
    NLOG2_LOOP_GAIN : in  unsigned(4 downto 0);
    DIN_I           : in  signed(DATA_WIDTH-1 downto 0);
    DIN_Q           : in  signed(DATA_WIDTH-1 downto 0);
    DIN_VLD         : in  std_logic;
    DOUT_I          : out signed(DATA_WIDTH-1 downto 0);
    DOUT_Q          : out signed(DATA_WIDTH-1 downto 0);
    DOUT_VLD        : out std_logic;
    C_CORR          : out signed(ACC_PREC-1 downto 0);
    D_CORR          : out signed(ACC_PREC-1 downto 0)
    );
end iq_imbalance_corrector;

architecture rtl of iq_imbalance_corrector is

  signal i_r1                       : signed(DATA_WIDTH-1 downto 0)                      := (others => '0');
  signal i_r2                       : signed(DATA_WIDTH-1 downto 0)                      := (others => '0');
  signal i_r3                       : signed(DATA_WIDTH-1 downto 0)                      := (others => '0');
  signal i_corr                     : signed(DATA_WIDTH-1 downto 0)                      := (others => '0');
  signal q_corr_sum                 : signed(DATA_WIDTH+ACC_PREC+1 downto 0);
  signal q_corr_trunc               : signed(DATA_WIDTH+ACC_PREC-2 downto 0);
  signal q_corr_trunc_r1            : signed(DATA_WIDTH+ACC_PREC-2 downto 0);
  signal q_corr_rnd                 : std_logic_vector(DATA_WIDTH-1 downto 0)            := (others => '0');
  signal q_corr                     : signed(DATA_WIDTH-1 downto 0)                      := (others => '0');
  signal i_corr_sq                  : signed(2*DATA_WIDTH-1 downto 0)                    := (others => '0');
  signal q_corr_sq                  : signed(2*DATA_WIDTH-1 downto 0)                    := (others => '0');
  signal power_diff                 : signed(2*DATA_WIDTH downto 0)                      := (others => '0');
  signal i_corr_q_corr              : signed(2*DATA_WIDTH-1 downto 0)                    := (others => '0');
  signal i_corr_q_corr_r1           : signed(2*DATA_WIDTH-1 downto 0)                    := (others => '0');
  signal c_i_prod                   : signed(DATA_WIDTH+ACC_PREC-1 downto 0)             := (others => '0');
  signal d_q_prod                   : signed(DATA_WIDTH+ACC_PREC-1 downto 0)             := (others => '0');
  signal power_error                : signed(2*DATA_WIDTH+32 downto 0)                   := (others => '0');
  signal phase_error                : signed(2*DATA_WIDTH+31 downto 0)                   := (others => '0');
  signal next_power_error           : signed(2*DATA_WIDTH+32 downto 0);
  signal next_phase_error           : signed(2*DATA_WIDTH+31 downto 0);
  signal next_power_error_padded    : signed(2*DATA_WIDTH+32+ACC_PREC-1 downto 0);
  signal next_phase_error_padded    : signed(2*DATA_WIDTH+31+ACC_PREC-1 downto 0);
  signal scaled_loop_power_error    : signed(ACC_PREC-1 downto 0);
  signal scaled_loop_power_error_r1 : signed(ACC_PREC-1 downto 0);
  signal scaled_loop_phase_error    : signed(ACC_PREC-1 downto 0);
  signal scaled_loop_phase_error_r1 : signed(ACC_PREC-1 downto 0);
  signal next_c_corr                : signed(ACC_PREC-1 downto 0)                        := (others => '0');
  signal next_d_corr                : signed(ACC_PREC-1 downto 0)                        := (others => '0');
  signal c_corr_int                 : signed(ACC_PREC-1 downto 0)                        := (others => '0');
  signal d_corr_int                 : signed(ACC_PREC-1 downto 0)                        := (others => '0');
  signal curr_update_cntr           : unsigned(31 downto 0)                              := (others => '0');
  signal next_update_cntr           : unsigned(31 downto 0);
  signal loop_errors_valid          : std_logic;
  signal zero_pad                   : std_logic_vector(ACC_PREC-2*DATA_WIDTH-1 downto 0) := (others => '0');
  signal gain_index                 : integer range 0 to (2**LOG2_AVG_LEN'length-1 + 2**NLOG2_LOOP_GAIN'length-1);

  function acc_prec_2 return signed is
   variable r : signed(ACC_PREC-1 downto 0) := (others => '0');
  begin
   r(ACC_PREC-2) := '1';
   return r;
  end function acc_prec_2;

begin
  -- delay DIN_I by four clocks to form the corrected version of I
  -- the corrected version of Q is saturated and rounded
  pipeline_reg_proc : process (CLK)
  begin
    if rising_edge(CLK) then
      if (RST = '1') then
        i_r1            <= (others => '0');
        i_r2            <= (others => '0');
        i_r3            <= (others => '0');
        i_corr          <= (others => '0');
        q_corr_trunc_r1 <= (others => '0');
        q_corr          <= (others => '0');
        c_i_prod        <= (others => '0');
        d_q_prod        <= (others => '0');
      else
        if (DIN_VLD = '1') then
          i_r1            <= DIN_I;
          i_r2            <= i_r1;
          i_r3            <= i_r2;
          i_corr          <= i_r3;
          q_corr_trunc_r1 <= q_corr_trunc;
          q_corr          <= signed(q_corr_rnd);  -- this is the output of the round_conv primitive
          c_i_prod        <= DIN_I * c_corr_int;  -- apply the phase error correction to I
          d_q_prod        <= DIN_Q * d_corr_int;  -- apply the power error correction to Q
        end if;
      end if;
    end if;
  end process pipeline_reg_proc;

  -- add the pipelined CI product to the pipelined DQ product
  q_corr_sum <= resize(signed(c_i_prod & '0'),q_corr_sum'length) + resize(signed(d_q_prod & '0'),q_corr_sum'length);

  -- saturate if an overflow condition is detected on signed data; overflow is determined by the top four sign bits
  saturate_proc : process (q_corr_sum)
  begin
    -- default is no overflow detected; simply remove the top three sign bits
    q_corr_trunc   <= q_corr_sum(DATA_WIDTH+ACC_PREC-2 downto 0);
    if (q_corr_sum(DATA_WIDTH+ACC_PREC+1) = '0' and (q_corr_sum(DATA_WIDTH+ACC_PREC) = '1' or q_corr_sum(DATA_WIDTH+ACC_PREC-1) = '1' or q_corr_sum(DATA_WIDTH+ACC_PREC-2) = '1')) then
      report "saturation detected" severity warning;
      q_corr_trunc <= to_signed(2**(DATA_WIDTH-1)-1,q_corr_trunc'length);
    end if;
    if (q_corr_sum(DATA_WIDTH+ACC_PREC+1) = '1' and (q_corr_sum(DATA_WIDTH+ACC_PREC) = '0' or q_corr_sum(DATA_WIDTH+ACC_PREC-1) = '0' or q_corr_sum(DATA_WIDTH+ACC_PREC-2) = '0')) then
      report "saturation detected" severity warning;
      q_corr_trunc <= to_signed(-(2**(DATA_WIDTH-1)),q_corr_trunc'length);
    end if;
  end process saturate_proc;

  -- now that we have removed the top sign bits (or saturated) get q_corr_rnd down to the input width with rounding
  q_corr_sum_rnd : misc_prims.misc_prims.round_conv
    generic map (
      DIN_WIDTH  => DATA_WIDTH+ACC_PREC-1,
      DOUT_WIDTH => DATA_WIDTH)
    port map (
      CLK      => CLK,
      RST      => RST,
      DIN      => std_logic_vector(q_corr_trunc_r1),
      DIN_VLD  => DIN_VLD,
      DOUT     => q_corr_rnd,
      DOUT_VLD => open);

  -- power difference loop to determine amplitude error
  power_diff_proc : process (CLK)
  begin
    if rising_edge(CLK) then
      if (RST = '1') then
        i_corr_sq    <= (others => '0');
        q_corr_sq    <= (others => '0');
        power_diff   <= (others => '0');
      else
        if (DIN_VLD = '1') then
          i_corr_sq  <= i_corr * i_corr;
          q_corr_sq  <= q_corr * q_corr;
          power_diff <= resize(i_corr_sq,power_diff'length) - resize(q_corr_sq,power_diff'length);
        end if;
      end if;
    end if;
  end process power_diff_proc;

  -- mixing product loop to determine phase error
  mixing_prod_proc : process (CLK)
  begin
    if rising_edge(CLK) then
      if (RST = '1') then
        i_corr_q_corr      <= (others => '0');
        i_corr_q_corr_r1   <= (others => '0');
      else
        if (DIN_VLD = '1') then
          i_corr_q_corr    <= i_corr * q_corr;
          i_corr_q_corr_r1 <= i_corr_q_corr;
        end if;
      end if;
    end if;
  end process mixing_prod_proc;

  -- compute loop errors using a top hat filter; first accumulate the power and phase errors
  next_power_error        <= power_error + resize(power_diff,next_power_error'length);
  next_phase_error        <= phase_error + resize(i_corr_q_corr_r1,next_phase_error'length);
  -- grow the two error signals to significant precision
  next_power_error_padded <= resize(signed(std_logic_vector(next_power_error) & zero_pad),next_power_error_padded'length);
  next_phase_error_padded <= resize(signed(std_logic_vector(next_phase_error) & zero_pad),next_phase_error_padded'length);
  -- scale the error signals based upon software register values
  -- create an intermediate gain_index integer with a fixed range because quartus can't handle the non-constant bit index
  gain_index              <= to_integer(LOG2_AVG_LEN) + to_integer(NLOG2_LOOP_GAIN);
  scaled_loop_power_error <= next_power_error_padded(ACC_PREC+gain_index-1 downto gain_index);
  scaled_loop_phase_error <= next_phase_error_padded(ACC_PREC+gain_index-1 downto gain_index);
  -- create a counter that controls the update interval
  next_update_cntr        <= curr_update_cntr + 1;
  loop_errors_valid       <= next_update_cntr(to_integer(LOG2_AVG_LEN)) and DIN_VLD;

  -- register the error values and update counter; only accumulate the errors over the interval period
  accum_and_cntr_proc : process (CLK)
  begin
    if rising_edge(CLK) then
      if (RST = '1') then
        power_error          <= (others => '0');
        phase_error          <= (others => '0');
        curr_update_cntr     <= (others => '0');
      else
        if (DIN_VLD = '1') then
          power_error        <= next_power_error;
          phase_error        <= next_phase_error;
          curr_update_cntr   <= curr_update_cntr + 1;
          if (next_update_cntr(to_integer(LOG2_AVG_LEN)) = '1') then
            power_error      <= (others => '0');
            phase_error      <= (others => '0');
            curr_update_cntr <= (others => '0');
          end if;
        end if;
      end if;
    end if;
  end process accum_and_cntr_proc;

  -- pipeline register
  pipeline_reg2_proc : process (CLK)
  begin
    if rising_edge(CLK) then
      if (RST = '1') then
        scaled_loop_power_error_r1   <= (others => '0');
        scaled_loop_phase_error_r1   <= (others => '0');
      else
        if (DIN_VLD = '1') then
          scaled_loop_power_error_r1 <= scaled_loop_power_error;
          scaled_loop_phase_error_r1 <= scaled_loop_phase_error;
        end if;
      end if;
    end if;
  end process pipeline_reg2_proc;

  -- single-pole IIR filter with loop gain where the phase error becomes c_corr
  c_corr_proc : process (ENABLE, UPDATE, loop_errors_valid, c_corr_int, scaled_loop_phase_error_r1)
  begin
    case ENABLE is
      when '0' => next_c_corr <= (others => '0');
      when '1' => if (UPDATE = '1' and loop_errors_valid = '1') then
                       next_c_corr <= c_corr_int - scaled_loop_phase_error_r1;
                     else
                       next_c_corr <= c_corr_int;
                     end if;
      when others => next_c_corr <= (others => '0');
    end case;
  end process c_corr_proc;

  -- single-pole IIR filter with loop gain where the power error becomes d_corr
  d_corr_proc : process (ENABLE, UPDATE, loop_errors_valid, d_corr_int, scaled_loop_power_error_r1)
  begin
    case ENABLE is
      when '0' => next_d_corr <= acc_prec_2;  -- (ACC_PREC-2 => '1', others => '0');
      when '1' => if (UPDATE = '1' and loop_errors_valid = '1') then
                       next_d_corr <= d_corr_int + scaled_loop_power_error_r1;
                     else
                       next_d_corr <= d_corr_int;
                     end if;
      when others => next_d_corr <= acc_prec_2;  -- (ACC_PREC-2 => '1', others => '0');
    end case;
  end process d_corr_proc;

  -- accumulate the scaled loop errors
  error_accum_proc : process (CLK)
  begin
    if rising_edge(CLK) then
      if (RST = '1') then
        c_corr_int                 <= (others => '0');
        d_corr_int                 <= acc_prec_2;  -- (ACC_PREC-2 => '1', others => '0');
      else
        if (DIN_VLD = '1') then
          c_corr_int               <= next_c_corr;
          d_corr_int               <= next_d_corr;
        end if;
      end if;
    end if;
  end process error_accum_proc;

  -- output ports
  DOUT_I   <= i_corr;
  DOUT_Q   <= q_corr;
  DOUT_VLD <= DIN_VLD;
  C_CORR   <= c_corr_int;
  D_CORR   <= d_corr_int;

end rtl;
