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
-- Cascaded Integrator-Comb (CIC) Decimator
-------------------------------------------------------------------------------
--
-- Description:
--
-- The CIC Decimation worker instantiates two real primitives to implement a
-- complex version and handles the output message signaling.
--
-- The generic parameters for this design are as follows:
--
--      N = Number of Stages
--      M = Differential Delay
--      R = Interpolation Factor
--      DIN_WIDTH = Input data width
--      ACC_WIDTH = Accumulator width ( >= CEIL(N*log2(R*M))+DIN_WIDTH )
--      DOUT_WIDTH = Output data width
-------------------------------------------------------------------------------

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions

architecture rtl of cic_dec2_worker is

  constant N_c              : integer := to_integer(unsigned(N));
  constant M_c              : integer := to_integer(unsigned(M));
  constant R_c              : integer := to_integer(unsigned(R));
  constant DIN_WIDTH_c      : integer := to_integer(unsigned(DIN_WIDTH));
  constant ACC_WIDTH_c      : integer := to_integer(unsigned(ACC_WIDTH));
  constant DOUT_WIDTH_c     : integer := to_integer(unsigned(DOUT_WIDTH));

  signal idata_vld          : std_logic;
  signal odata_vld          : std_logic;
  signal i_out, q_out       : std_logic_vector(DOUT_WIDTH_c-1 downto 0);
--  signal in_in_eof_r        : bool_t;

begin

  ---------------------------------------------------------------------------------
  -- Enable input to primitive (idata_vld) and take from input (in_out.take)
  -- when data is present/valid and output is allowed
  ---------------------------------------------------------------------------------

  idata_vld   <= in_in.valid and out_in.ready;
  in_out.take <= idata_vld;

  --------------------------------------------------------------------------------
  -- Give to output porty when output of primitive is valid and output is allowed)
  --------------------------------------------------------------------------------

  out_out.give        <= to_bool(out_in.ready and odata_vld = '1');
  out_out.valid       <= odata_vld;
  out_out.data        <= std_logic_vector(resize(signed(q_out), 16)) & std_logic_vector(resize(signed(i_out), 16));
  out_out.byte_enable <= (others => '1');
  -- out_out.eof         <= in_in_eof_r;
  -----------------------------------------------------------------------------
  -- We do not use the default eof-propagation from input to output because
  -- our output is not continuous, and thus we can have pipelined output data
  -- to "ship" that occurs AFTER the input happens on input.  In this case
  -- it is only one cycle, but since the eof input is valid on the next cycle
  -- after the last input value is taken
  -- Delay the input EOF according to the known latency of the primitive so that
  -- we don't assert the output eof while there is still data in the pipeline
  -----------------------------------------------------------------------------
  -- eof_delay : process (ctl_in.clk)
  -- begin
  --   if rising_edge(ctl_in.clk) then
  --     if (ctl_in.reset = '1') then
  --       in_in_eof_r <= bfalse;
  --     elsif its(out_in.ready) and in_in.eof and odata_vld = '0' then
  --       in_in_eof_r <= btrue;
  --     end if;
  --   end if;
  -- end process;  
  



  -----------------------------------------------------------------------------
  -- CIC Decimation primitives (I & Q)
  -----------------------------------------------------------------------------

  Dec_I : dsp_prims.dsp_prims.cic_dec_gen
    generic map(
      N          => N_c,
      M          => M_c,
      R          => R_c,
      DIN_WIDTH  => DIN_WIDTH_c,
      ACC_WIDTH  => ACC_WIDTH_c,
      DOUT_WIDTH => DOUT_WIDTH_c
      )
    port map(
      CLK      => ctl_in.clk,
      RST      => ctl_in.reset,
      DIN_VLD  => idata_vld,
      DIN      => in_in.data(DIN_WIDTH_c-1 downto 0),
      DOUT_VLD => odata_vld,
      DOUT     => i_out
      );
  Dec_Q : dsp_prims.dsp_prims.cic_dec_gen
    generic map(
      N          => N_c,
      M          => M_c,
      R          => R_c,
      DIN_WIDTH  => DIN_WIDTH_c,
      ACC_WIDTH  => ACC_WIDTH_c,
      DOUT_WIDTH => DOUT_WIDTH_c
      )
    port map(
      CLK      => ctl_in.clk,
      RST      => ctl_in.reset,
      DIN_VLD  => idata_vld,
      DIN      => in_in.data(DIN_WIDTH_c-1+16 downto 16),
      DOUT_VLD => open,
      DOUT     => q_out
      );

end rtl;
