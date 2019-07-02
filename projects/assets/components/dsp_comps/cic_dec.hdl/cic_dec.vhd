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

architecture rtl of worker is

  constant c_n              : integer := to_integer(N);
  constant c_m              : integer := to_integer(M);
  constant c_r              : integer := to_integer(R);
  constant c_din_width      : integer := to_integer(DIN_WIDTH);
  constant c_acc_width      : integer := to_integer(ACC_WIDTH);
  constant c_dout_width     : integer := to_integer(DOUT_WIDTH);

  signal s_idata_vld        : std_logic;
  signal s_idata_rdy        : std_logic;
  signal s_odata_vld        : std_logic;
  signal s_i_out, s_q_out   : std_logic_vector(c_dout_width-1 downto 0);

begin

  ---------------------------------------------------------------------------------
  -- Enable input to primitive (s_idata_vld) and take from input (in_out.take)
  -- when data is present/valid and when primitive input is ready for data
  ---------------------------------------------------------------------------------

  s_idata_vld <= in_in.valid and s_idata_rdy;
  in_out.take <= s_idata_vld;

  --------------------------------------------------------------------------------
  -- Give to output port when output of primitive is valid and output is allowed
  --------------------------------------------------------------------------------

  out_out.valid       <= s_odata_vld;
  out_out.data        <= std_logic_vector(resize(signed(s_q_out), 16)) & std_logic_vector(resize(signed(s_i_out), 16));
  
  -----------------------------------------------------------------------------
  -- CIC Decimation primitives (I & Q)
  -----------------------------------------------------------------------------

  Dec_I : dsp_prims.dsp_prims.cic_dec_gen
    generic map(
      g_n          => c_n,
      g_m          => c_m,
      g_r          => c_r,
      g_din_width  => c_din_width,
      g_acc_width  => c_acc_width,
      g_dout_width => c_dout_width
      )
    port map(
      i_clk      => ctl_in.clk,
      i_rst      => ctl_in.reset,
      i_din      => in_in.data(c_din_width-1 downto 0),
      i_din_vld  => s_idata_vld,
      o_din_rdy  => s_idata_rdy,
      o_dout     => s_i_out,
      o_dout_vld => s_odata_vld,
      i_dout_rdy => out_in.ready
      );
  
  Dec_Q : dsp_prims.dsp_prims.cic_dec_gen
    generic map(
      g_n          => c_n,
      g_m          => c_m,
      g_r          => c_r,
      g_din_width  => c_din_width,
      g_acc_width  => c_acc_width,
      g_dout_width => c_dout_width
      )
    port map(
      i_clk      => ctl_in.clk,
      i_rst      => ctl_in.reset,
      i_din      => in_in.data(2*c_din_width-1 downto c_din_width),
      i_din_vld  => s_idata_vld,
      o_din_rdy  => open,
      o_dout     => s_q_out,
      o_dout_vld => open,
      i_dout_rdy => out_in.ready
      );

end rtl;
