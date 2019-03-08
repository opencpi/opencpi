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
-- Description:
--
-- The CIC Interpolation worker instantiates two real primitives to implement a
-- complex version and handles the output message signaling. The worker provides
-- parameters for configuring the primitive's generic ports.
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

  constant N_c              : integer := to_integer(unsigned(N));
  constant M_c              : integer := to_integer(unsigned(M));
  constant R_c              : integer := to_integer(unsigned(R));
  constant DIN_WIDTH_c      : integer := to_integer(unsigned(DIN_WIDTH));
  constant ACC_WIDTH_c      : integer := to_integer(unsigned(ACC_WIDTH));
  constant DOUT_WIDTH_c     : integer := to_integer(unsigned(DOUT_WIDTH));
  constant MAX_MESSAGE_VALUES_c : integer := 4096;  -- from iqstream_protocol
  signal int_cnt            : unsigned(15 downto 0);
  signal idata_vld          : std_logic;
  signal enable_out         : std_logic;
  signal odata_vld          : std_logic;
  signal i_out, q_out       : std_logic_vector(DOUT_WIDTH_c-1 downto 0);

begin
  -----------------------------------------------------------------------------
  -- Everything is throttled based on output being allowed: out_in.ready
  -----------------------------------------------------------------------------
  -- idata_vld (enable the primitive when data is available, output is allowed,
  -- and interpolation counter at 'begin')
  -----------------------------------------------------------------------------
  idata_vld <= to_bool(out_in.ready and its(in_in.valid) and int_cnt = 0);
  -----------------------------------------------------------------------------
  -- enable_out (when data can move, either at "begin" or during interpolation
  -----------------------------------------------------------------------------
  enable_out <= '1' when out_in.ready and (in_in.valid or (in_in.eof and int_cnt /= 0)) else '0';
  -----------------------------------------------------------------------------
  -- Take when in at "begin", otherwise wait for next begin
  -- "Take" will actually happen when data is available
  -----------------------------------------------------------------------------
  in_out.take <= to_bool(out_in.ready and int_cnt = 0);
  -----------------------------------------------------------------------------
  -- Give when primitive has valid output (will take effect when output is allowed)
  -----------------------------------------------------------------------------
  out_out.give  <= odata_vld;
  out_out.valid <= odata_vld;
  out_out.data  <= std_logic_vector(resize(signed(q_out), 16)) & std_logic_vector(resize(signed(i_out), 16));

  -----------------------------------------------------------------------------
  -- Interpolation count tracker
  -----------------------------------------------------------------------------

  proc_InterpolationCounter : process (ctl_in.clk)
  begin
    if (rising_edge(ctl_in.clk)) then
      if (ctl_in.reset = '1') then
        int_cnt <= (others => '0');
      elsif (out_in.ready = '1' and in_in.ready = '1') then
        if (in_in.valid = '1' and int_cnt = 0) then
          int_cnt <= to_unsigned(1, 16);
        elsif (int_cnt = unsigned(R)-1) then
          int_cnt <= (others => '0');
        else
          int_cnt <= int_cnt + 1;
        end if;
      end if;
    end if;
  end process;

  -----------------------------------------------------------------------------
  -- CIC Interpolation primitives (I & Q)
  -----------------------------------------------------------------------------

  Int_I : dsp_prims.dsp_prims.cic_int_gen
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
      DOUT_EN  => enable_out,
      DOUT_VLD => odata_vld,
      DOUT     => i_out
      );
  Int_Q : dsp_prims.dsp_prims.cic_int_gen
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
      DOUT_EN  => enable_out,
      DOUT_VLD => open,
      DOUT     => q_out
      );

  -----------------------------------------------------------------------------
  -- DEBUG: CHIPSCOPE
  -----------------------------------------------------------------------------
  gen_DEBUG : if its(VIVADO_ILA_p) or its(CHIPSCOPE_p) generate
    signal DATA     : std_logic_vector(111 downto 0);
    signal TRIG0    : std_logic_vector(47 downto 0);
  begin

    gen_VILA: if its(VIVADO_ILA_p) generate
      component vivado_ila
        port (
          clk     : in    std_logic;
          probe0  : in    std_logic_vector(111 downto 0);
          probe1  : in    std_logic_vector(47 downto 0));
      end component;
  
    begin
      inst_ILA : vivado_ila
        port map (
          clk     => ctl_in.clk,
          probe0  => DATA,
          probe1  => TRIG0
          );
  
    end generate;
  
    gen_CS : if its(CHIPSCOPE_p) generate

      component chipscope_icon
        port (
          CONTROL0 : inout std_logic_vector(35 downto 0));
      end component;
  
      component chipscope_ila_trig_n_data
        port (
          CONTROL : inout std_logic_vector(35 downto 0);
          CLK     : in    std_logic;
          DATA    : in    std_logic_vector(111 downto 0);
          TRIG0   : in    std_logic_vector(47 downto 0));
      end component;
  
      signal CONTROL0 : std_logic_vector(35 downto 0);
  
    begin
  
      inst_ICON : chipscope_icon
        port map (
          CONTROL0 => CONTROL0
          );
      inst_ILA : chipscope_ila_trig_n_data
        port map (
          CONTROL => CONTROL0,
          CLK     => ctl_in.clk,
          DATA    => DATA,
          TRIG0   => TRIG0
          );

    end generate;
  
    DATA(111 downto 104) <= std_logic_vector(int_cnt(15 downto 8));
    DATA(103 downto 96)  <= std_logic_vector(int_cnt(7 downto 0));
    DATA(95 downto 92)   <= '0' & '0' & '0' & '0';
    DATA(91 downto 88)   <= '0' & odata_vld & enable_out & idata_vld;
    DATA(87 downto 84)   <= '0' & '0' & '0' & out_in.ready;
    DATA(83 downto 80)   <= in_in.ready & in_in.valid & in_in.som & in_in.eom;
    DATA(79 downto 64)   <= (others => '0');
    DATA(63 downto 48)   <= in_in.data(DIN_WIDTH_c-1+16 downto 16);
    DATA(47 downto 32)   <= in_in.data(DIN_WIDTH_c-1 downto 0);
    DATA(31 downto 16)   <= i_out;
    DATA(15 downto 0)    <= q_out;

    TRIG0(47 downto 40) <= std_logic_vector(int_cnt(15 downto 8));
    TRIG0(39 downto 32) <= std_logic_vector(int_cnt(7 downto 0));
    TRIG0(31 downto 28) <= '0' & '0' & '0' & '0';
    TRIG0(27 downto 24) <= '0' & odata_vld & enable_out & idata_vld;
    TRIG0(23 downto 20) <= '0' & '0' & '0' & out_in.ready;
    TRIG0(19 downto 16) <= in_in.ready & in_in.valid & in_in.som & in_in.eom;
    TRIG0(15 downto 0)  <= (others => '0');

  end generate;

end rtl;
