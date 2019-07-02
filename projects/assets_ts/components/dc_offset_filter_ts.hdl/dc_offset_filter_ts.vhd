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
-- The DC Offset Cancellation Filter worker inputs complex signed samples and
-- removes the DC bias from both I and Q input rails using separate 1st-order
-- IIR filters. The response time of the filter is programmable, as is the
-- ability to bypass the filter and to update/hold the calculated DC value to
-- be removed. A generic controls insertion of a peak detection circuit.
--
-- The time constant input TC = 128 * α, which is limited to a signed eight-bit
-- number, helps to control the bit growth of the multiplier. Rearranging the
-- equation, α = TC/128, where a maximum value of +127 is allowed due to signed
-- multiplication. Larger values of TC give both a faster filter response and a
-- narrower frequency magnitude notch at zero Hertz. A typical value of TC = 121
-- (α = 0.95) is used.
--
-- The input should be attenuated by as much as one bit to avoid overflow on
-- the output; i.e. the input should not be driven more than half-scale to
-- avoid overflow.
--
-- The circuit may be bypassed by asserting the BYPASS input. The circuit has a
-- latency of one DIN_VLD clock cycle.
-------------------------------------------------------------------------------

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.types.all, ocpi.util.all; -- remove this to avoid all ocpi name collisions

architecture rtl of worker is

  constant c_opcode_width : positive := ocpi.util.width_for_max(ComplexShortWithMetadata_OpCode_t'pos(ComplexShortWithMetadata_OpCode_t'right));
  constant c_latency      : positive := to_integer(LATENCY);
  constant c_data_width   : integer  := to_integer(unsigned(DATA_WIDTH));
  --
  signal s_bypass_i       : std_logic;
  signal s_enable         : std_logic;
  signal s_data_vld_i     : std_logic;
  signal s_in_opcode_slv  : std_logic_vector(c_opcode_width-1 downto 0);
  signal s_out_opcode_slv : std_logic_vector(c_opcode_width-1 downto 0);
  signal s_out_opcode_int : natural;
  signal s_som            : std_logic;
  signal s_eom            : std_logic;
  signal s_valid          : std_logic;
  signal s_ready          : std_logic;
  signal s_in_som         : std_logic;
  signal s_in_eom         : std_logic;
  signal s_in_valid       : std_logic;
  signal s_data           : std_logic_vector(ocpi_port_in_data_width-1 downto 0);
  signal s_samples_opcode : std_logic;
  signal s_eof            : std_logic;
  --
  signal s_i_o            : signed(c_data_width-1 downto 0);
  signal s_q_o            : signed(c_data_width-1 downto 0);
  -- Peak Detector
  signal s_peak_rst_i     : std_logic;
  signal s_peak_a_i       : std_logic_vector(15 downto 0);
  signal s_peak_b_i       : std_logic_vector(15 downto 0);
  signal s_peak_o         : std_logic_vector(15 downto 0);

begin

  s_bypass_i <= not std_logic(props_in.enable);

  s_samples_opcode <= in_in.ready and
                      to_bool(in_in.opcode = ComplexShortWithMetadata_samples_op_e);

  --'enable' circuit (when up/downstream Workers ready and operating)
  s_enable <= ctl_in.is_operating and in_in.ready and out_in.ready;

  s_data_vld_i <= s_enable and in_in.valid and s_samples_opcode;

  -- Take (when up/downstream Workers ready and operating)
  in_out.take <= s_enable;

  -- Translate opcode to slv for delay primitive
  -- Xilinx/ISE 14.6 synthesis doesn't do the t'val(x) function properly
  -- Hence this workaround
  s_in_opcode_slv <=
    std_logic_vector(to_unsigned(1, s_in_opcode_slv'length)) when in_in.opcode = ComplexShortWithMetadata_time_op_e else
    std_logic_vector(to_unsigned(2, s_in_opcode_slv'length)) when in_in.opcode = ComplexShortWithMetadata_interval_op_e else
    std_logic_vector(to_unsigned(3, s_in_opcode_slv'length)) when in_in.opcode = ComplexShortWithMetadata_flush_op_e else
    std_logic_vector(to_unsigned(4, s_in_opcode_slv'length)) when in_in.opcode = ComplexShortWithMetadata_sync_op_e else
    std_logic_vector(to_unsigned(0, s_in_opcode_slv'length));  --in_in.opcode = ComplexShortWithMetadata_samples_op_e

  s_in_som   <= in_in.som;
  s_in_eom   <= in_in.eom;
  s_in_valid <= in_in.valid;

  -- Delay line to match the latency of the primitive for non-sample data
  delay_inst : ocpi.wsi.delayline
  generic map (
    g_latency     => c_latency)
  port map (
    i_clk         => ctl_in.clk,
    i_reset       => ctl_in.reset,
    i_enable      => out_in.ready,
    i_ready       => in_in.ready,
    i_som         => s_in_som,
    i_eom         => s_in_eom,
    i_opcode      => s_in_opcode_slv,
    i_valid       => s_in_valid,
    i_byte_enable => in_in.byte_enable,
    i_data        => in_in.data,
    i_eof         => in_in.eof,
    o_ready       => s_ready,
    o_som         => s_som,
    o_eom         => s_eom,
    o_opcode      => s_out_opcode_slv,
    o_valid       => s_valid,
    o_byte_enable => out_out.byte_enable,
    o_data        => s_data,
    o_eof         => s_eof);

  out_out.eof <= s_eof;
  out_out.som <= s_som;
  out_out.eom <= s_eom;

  -- Give (when downstream Worker ready & primitive has valid output OR not
  -- valid message flags)
  out_out.give <= ctl_in.is_operating and out_in.ready and s_ready and
                  (s_valid or ((s_som or s_eom) and not s_valid));

  -- Valid (when downstream Worker ready & primitive has valid output)
  out_out.valid <= out_in.ready and s_valid;

  -- Translate opcode output of delayline primitive to enumerated type
  -- Xilinx/ISE 14.6 synthesis doesn't do the t'pos(x) function properly
  -- Hence this workaround
  s_out_opcode_int <= to_integer(unsigned(s_out_opcode_slv));
  out_out.opcode <=
    ComplexShortWithMetadata_samples_op_e  when s_out_opcode_int = 0 else
    ComplexShortWithMetadata_time_op_e     when s_out_opcode_int = 1 else
    ComplexShortWithMetadata_interval_op_e when s_out_opcode_int = 2 else
    ComplexShortWithMetadata_flush_op_e    when s_out_opcode_int = 3 else
    ComplexShortWithMetadata_sync_op_e     when s_out_opcode_int = 4;

  out_out.data <= std_logic_vector(resize(s_q_o, c_data_width)) &
                  std_logic_vector(resize(s_i_o, c_data_width))
                  when its(props_in.enable) and s_out_opcode_int = 0 else s_data;

  -----------------------------------------------------------------------------
  -- DC offset cancellation filter
  -----------------------------------------------------------------------------

  i_dc_filter : dsp_prims.dsp_prims.dc_offset_cancellation
    generic map (
      DATA_WIDTH => c_data_width)
    port map (
      CLK      => ctl_in.clk,
      RST      => ctl_in.reset,
      BYPASS   => s_bypass_i,
      UPDATE   => std_logic(props_in.update),
      TC       => signed(props_in.tc),
      DIN      => signed(in_in.data(c_data_width-1 downto 0)),
      DIN_VLD  => s_data_vld_i,
      DOUT_RDY => out_in.ready,
      DOUT     => s_i_o,
      DOUT_VLD => open);

  q_dc_filter : dsp_prims.dsp_prims.dc_offset_cancellation
    generic map (
      DATA_WIDTH => c_data_width)
    port map (
      CLK      => ctl_in.clk,
      RST      => ctl_in.reset,
      BYPASS   => s_bypass_i,
      UPDATE   => std_logic(props_in.update),
      TC       => signed(props_in.tc),
      DIN      => signed(in_in.data(c_data_width-1+16 downto 16)),
      DIN_VLD  => s_data_vld_i,
      DOUT_RDY => out_in.ready,
      DOUT     => s_q_o,
      DOUT_VLD => open);

  -----------------------------------------------------------------------------
  -- Peak Detection primitive. Value is cleared when read
  -----------------------------------------------------------------------------

  s_peak_rst_i <= ctl_in.reset or std_logic(props_in.peak_read);
  s_peak_a_i   <= std_logic_vector(resize(signed(s_i_o), 16))  -- default output
                  when (props_in.enable = '1') else in_in.data(15 downto 0);  --BYPASS (ENABLE=0)
  s_peak_b_i <= std_logic_vector(resize(signed(s_q_o), 16))  -- default output
                when (props_in.enable = '1') else in_in.data(31 downto 16);  --BYPASS (ENABLE=0)

  pm_gen : if its(PEAK_MONITOR) generate
    pd : util_prims.util_prims.peakDetect
      port map (
        CLK_IN   => ctl_in.clk,
        RST_IN   => s_peak_rst_i,
        EN_IN    => s_data_vld_i,
        A_IN     => s_peak_a_i,
        B_IN     => s_peak_b_i,
        PEAK_OUT => s_peak_o);

    props_out.peak <= signed(s_peak_o);
  end generate pm_gen;

  no_pm_gen : if its(not PEAK_MONITOR) generate
    props_out.peak <= (others => '0');
  end generate no_pm_gen;
end rtl;
