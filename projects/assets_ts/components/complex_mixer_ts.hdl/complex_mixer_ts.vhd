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
-- Complex Mixer
-------------------------------------------------------------------------------
--
-- Description:
--
-- The Complex Mixer worker inputs complex signed samples and performs a
-- complex multiply with a digital sine wave produced by an numerically
-- controlled oscillator (NCO). The resulting output data is a frequency
-- shifted version of the input data. The frequency and amplitude of the sine
-- wave are programmable as well as the ability to bypass the module. Generics
-- can be used to control the width of the data, phase accumulator, and phase
-- increment of the NCO as well as control insertion of a peak detection
-- circuit.
-------------------------------------------------------------------------------

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.types.all, ocpi.util.all; -- remove this to avoid all ocpi name collisions
architecture rtl of complex_mixer_ts_worker is

  constant NCO_DATA_WIDTH_c   : positive := to_integer(NCO_DATA_WIDTH_p);
  constant INPUT_DATA_WIDTH_c : positive := to_integer(INPUT_DATA_WIDTH_p);
  constant CORDIC_STAGES_c    : positive := to_integer(CORDIC_STAGES_p);
  constant LATENCY_c          : positive := to_integer(LATENCY_p);
  constant RND_DATA_WIDTH_c   : positive := short_t'length; --I/Q are type short in ComplexShortWithMetadata protocol

  constant OPCODE_WIDTH_c  : positive := ocpi.util.width_for_max(ComplexShortWithMetadata_OpCode_t'pos(ComplexShortWithMetadata_OpCode_t'right));
  constant PORT_WIDTH_c    : positive := ocpi_port_in_data_width;
  constant BYTE_EN_WIDTH_c : positive := ocpi_port_in_MByteEn_width;

  type data_array is array (natural range <>) of std_logic_vector(ocpi_port_in_data_width-1 downto 0);
  type byte_enable_array is array (natural range <>) of std_logic_vector(ocpi_port_in_MByteEn_width-1 downto 0);
  type opcode_array is array (natural range <>) of ComplexShortWithMetadata_OpCode_t;

  signal enable              : std_logic;
  signal in_opcode_slv       : std_logic_vector(OPCODE_WIDTH_c-1 downto 0);
  signal out_opcode_slv      : std_logic_vector(OPCODE_WIDTH_c-1 downto 0);
  signal out_opcode_int      : natural;
  signal som                 : std_logic;
  signal eom                 : std_logic;
  signal valid               : std_logic;
  signal ready               : std_logic;
  signal in_som              : std_logic;
  signal in_eom              : std_logic;
  signal in_valid            : std_logic;
  signal data                : std_logic_vector(ocpi_port_in_data_width-1 downto 0);
  signal samples_opcode      : std_logic;
  signal eof                 : std_logic;

  -- NCO signals
  signal nco_reset           : std_logic;
  signal nco_enable          : std_logic;
  signal nco_init_cnt        : natural;
  signal nco_vld             : std_logic;
  signal nco_out_i           : std_logic_vector(NCO_DATA_WIDTH_c-1 downto 0);
  signal nco_out_q           : std_logic_vector(NCO_DATA_WIDTH_c-1 downto 0);

  -- Complex Multiplier
  signal mult_enable : std_logic;
  signal mult_enable_r : std_logic;
  signal mix_i       : std_logic_vector(RND_DATA_WIDTH_c-1 downto 0);
  signal mix_q       : std_logic_vector(RND_DATA_WIDTH_c-1 downto 0);

  -- Peak detector
  signal peak_rst_in : std_logic;
  signal peak_a_in   : std_logic_vector(15 downto 0);
  signal peak_b_in   : std_logic_vector(15 downto 0);
  signal peak_out    : std_logic_vector(15 downto 0);

begin

  samples_opcode <= in_in.ready and
                    to_bool(in_in.opcode=ComplexShortWithMetadata_samples_op_e);

  --nco primitive needs to be reset during sync
  nco_reset <= ctl_in.reset or (in_in.ready and to_bool(in_in.opcode=ComplexShortWithMetadata_sync_op_e));
  
  --'enable' circuit (when up/downstream Workers ready and operating)
  enable <= ctl_in.is_operating and in_in.ready and out_in.ready;

  -- 'enable' NCO primitive (when enabled and input valid)
  nco_enable <= enable and in_in.valid and samples_opcode;

  -- 'enable' multiplier primitive (when enabled and input valid and NCO output valid)
  mult_enable <= (nco_enable and nco_vld) or (mult_enable_r and out_in.ready);
  -- Register enable when taking samples
  mult_enable_reg : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(nco_reset) then
        mult_enable_r <= '0';
      elsif its(enable) and its(nco_vld) and its(samples_opcode) then --taking samples
        mult_enable_r <= mult_enable;
      end if;
    end if;
  end process;

  -- Take (when up/downstream Workers ready and operating)
  in_out.take <= enable and nco_vld when its(samples_opcode) and its(in_in.valid) else enable;

  -- Translate opcode to slv for delay primitive
  -- Xilinx/ISE 14.6 synthesis doesn't do the t'val(x) function properly
  -- Hence this workaround
  in_opcode_slv <=
    std_logic_vector(to_unsigned(1,in_opcode_slv'length)) when in_in.opcode = ComplexShortWithMetadata_time_op_e else
    std_logic_vector(to_unsigned(2,in_opcode_slv'length)) when in_in.opcode = ComplexShortWithMetadata_interval_op_e else
    std_logic_vector(to_unsigned(3,in_opcode_slv'length)) when in_in.opcode = ComplexShortWithMetadata_flush_op_e else
    std_logic_vector(to_unsigned(4,in_opcode_slv'length)) when in_in.opcode = ComplexShortWithMetadata_sync_op_e else
    std_logic_vector(to_unsigned(0,in_opcode_slv'length)); --in_in.opcode = ComplexShortWithMetadata_samples_op_e

  -- Inputs to the delayline primitive must qualify valid samples opcodes with nco_vld
  in_som   <= in_in.som   and nco_vld when its(samples_opcode) and its(in_in.valid) and its(in_in.ready) else in_in.som;
  in_eom   <= in_in.eom   and nco_vld when its(samples_opcode) and its(in_in.valid) and its(in_in.ready) else in_in.eom;
  in_valid <= in_in.valid and nco_vld when its(samples_opcode) and its(in_in.valid) and its(in_in.ready) else in_in.valid;
  
  -- Delay line to match the latency of the primitive for non-sample data
  delay_inst : ocpi.wsi.delayline
  generic map (
    g_latency     => LATENCY_c)
  port map (
    i_clk         => ctl_in.clk,
    i_reset       => ctl_in.reset,
    i_enable      => out_in.ready,
    i_ready       => in_in.ready,
    i_som         => in_som,
    i_eom         => in_eom,
    i_opcode      => in_opcode_slv,
    i_valid       => in_valid,
    i_byte_enable => in_in.byte_enable,
    i_data        => in_in.data,
    i_eof         => in_in.eof,
    o_ready       => ready,
    o_som         => som,
    o_eom         => eom,
    o_opcode      => out_opcode_slv,
    o_valid       => valid,
    o_byte_enable => out_out.byte_enable,
    o_data        => data,
    o_eof         => eof);

  out_out.eof <= eof;
  out_out.som <= som;
  out_out.eom <= eom;
  
  -- Give (when downstream Worker ready & primitive has valid output OR not
  -- valid message flags)
  out_out.give <= ctl_in.is_operating and out_in.ready and ready and
                  (valid or ((som or eom) and not valid));

  -- Valid (when downstream Worker ready & primitive has valid output)
  out_out.valid <= out_in.ready and valid;

  -- Translate opcode output of delayline primitive to enumerated type
  -- Xilinx/ISE 14.6 synthesis doesn't do the t'pos(x) function properly
  -- Hence this workaround
  out_opcode_int <= to_integer(unsigned(out_opcode_slv));
  out_out.opcode <=
    ComplexShortWithMetadata_samples_op_e  when out_opcode_int = 0 else
    ComplexShortWithMetadata_time_op_e     when out_opcode_int = 1 else
    ComplexShortWithMetadata_interval_op_e when out_opcode_int = 2 else
    ComplexShortWithMetadata_flush_op_e    when out_opcode_int = 3 else
    ComplexShortWithMetadata_sync_op_e     when out_opcode_int = 4;

  out_out.data <= std_logic_vector(resize(signed(mix_q),16)) & std_logic_vector(resize(signed(mix_i),16))
                  when its(props_in.enable) and out_opcode_int=0 else data;
  
  -- nco requires CORDIC_STAGES_c+1 enabled clock cycles before
  -- producing valid output. Reset on phs_inc or mag change.
  ncoInitCounter : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(nco_reset) or its(props_in.phs_inc_written) or its(props_in.mag_written) then
        nco_init_cnt <= 0;
        nco_vld      <= '0';
      elsif its(nco_enable) then
        if nco_init_cnt = CORDIC_STAGES_c+1 then
          nco_vld <= '1';
        else
          nco_init_cnt <= nco_init_cnt + 1;
        end if;
      end if;
    end if;
  end process;

  -- NCO Primitive
  nco_inst : dsp_prims.dsp_prims.nco
    generic map (
      DATA_WIDTH    => NCO_DATA_WIDTH_c,
      PHS_INC_WIDTH => props_in.phs_inc'length,
      PHS_ACC_WIDTH => props_in.phs_init'length,
      CORDIC_STAGES => CORDIC_STAGES_c)
    port map (
      CLK      => ctl_in.clk,
      RST      => nco_reset,
      EN       => nco_enable,
      MAG      => std_logic_vector(props_in.mag(NCO_DATA_WIDTH_c-1 downto 0)),
      PHS_INC  => std_logic_vector(props_in.phs_inc),
      PHS_INIT => std_logic_vector(props_in.phs_init),
      I_OUT    => nco_out_i,
      Q_OUT    => nco_out_q,
      VLD_OUT  => open);

  -- Complex Multiply
  mult_inst : util_prims.util_prims.complex_mult
    generic map (
      A_WIDTH    => NCO_DATA_WIDTH_c,
      B_WIDTH    => INPUT_DATA_WIDTH_c,
      PROD_WIDTH => RND_DATA_WIDTH_c
      )
    port map (
      CLK      => ctl_in.clk,
      RST      => ctl_in.reset,
      DIN_A_RE => nco_out_i,
      DIN_A_IM => nco_out_q,
      DIN_B_RE => in_in.data(INPUT_DATA_WIDTH_c-1 downto 0),
      DIN_B_IM => in_in.data(INPUT_DATA_WIDTH_c+15 downto 16),
      DIN_VLD  => mult_enable,
      DOUT_RE  => mix_i,
      DOUT_IM  => mix_q,
      DOUT_VLD => open);

  -- Peak Detection primitive. Value is cleared when read
  peak_rst_in <= ctl_in.reset or std_logic(props_in.peak_read);
  peak_a_in  <= std_logic_vector(resize(signed(mix_i), 16))  -- default output
                when (props_in.enable = '1') else in_in.data(15 downto 0);  --BYPASS (ENABLE=0)
  peak_b_in  <= std_logic_vector(resize(signed(mix_q), 16))  -- default output
                when (props_in.enable = '1') else in_in.data(31 downto 16);  --BYPASS (ENABLE=0)

  pm_gen : if its(PEAK_MONITOR) generate
    pd : util_prims.util_prims.peakDetect
      port map (
        CLK_IN   => ctl_in.clk,
        RST_IN   => peak_rst_in,
        EN_IN    => mult_enable,
        A_IN     => peak_a_in,
        B_IN     => peak_b_in,
        PEAK_OUT => peak_out);

    props_out.peak <= signed(peak_out);
  end generate pm_gen;

end rtl;
