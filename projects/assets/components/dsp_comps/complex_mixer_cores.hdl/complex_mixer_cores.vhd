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

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all, ocpi.util.all; -- remove this to avoid all ocpi name collisions

architecture rtl of worker is

  component complex_multiplier
    port (
      aclk               : in  std_logic;
      aclken             : in  std_logic;
      aresetn            : in  std_logic;
      s_axis_a_tvalid    : in  std_logic;
      s_axis_a_tdata     : in  std_logic_vector(31 downto 0);
      s_axis_b_tvalid    : in  std_logic;
      s_axis_b_tdata     : in  std_logic_vector(31 downto 0);
      m_axis_dout_tvalid : out std_logic;
      m_axis_dout_tdata  : out std_logic_vector(79 downto 0)
      );
  end component;

  component dds_compiler
    port (
      aclk                : in  std_logic;
      aresetn             : in  std_logic;
      s_axis_phase_tvalid : in  std_logic;
      s_axis_phase_tdata  : in  std_logic_vector(15 downto 0);
      m_axis_data_tvalid  : out std_logic;
      m_axis_data_tdata   : out std_logic_vector(31 downto 0)
      );
  end component;

  constant c_opcode_width          : positive := 1;
  constant c_dds_core_latency      : natural  := 7;
  constant c_complex_mixer_latency : natural  := 6+1;

  -- WSI Interface temporary signal
  signal s_enable            : std_logic;
  signal s_data_vld_i        : std_logic;
  signal s_reset_n_i         : std_logic;
  signal s_in_opcode_slv     : std_logic_vector(c_opcode_width-1 downto 0);
  signal s_out_opcode_slv    : std_logic_vector(c_opcode_width-1 downto 0);
  signal s_som               : std_logic;
  signal s_eom               : std_logic;
  signal s_valid             : std_logic;
  signal s_ready             : std_logic;
  signal s_in_som            : std_logic;
  signal s_in_eom            : std_logic;
  signal s_in_valid          : std_logic;
  signal s_data              : std_logic_vector(ocpi_port_in_data_width-1 downto 0);
  signal s_samples_opcode    : std_logic;
  signal s_eof               : std_logic;
  -- DDS (NCO)
  signal s_dds_primer_cnt    : unsigned(3 downto 0);
  signal s_dds_vld           : std_logic;
  signal s_dds_ce_i          : std_logic;
  signal s_dds_sine_cosine_o : std_logic_vector(31 downto 0) := (others => '0');
  signal s_dds_real          : std_logic_vector(15 downto 0);
  signal s_dds_imag          : std_logic_vector(15 downto 0);
  -- Complex Multiplier
  signal s_complx_mult_ce_i  : std_logic;
  signal s1_complx_mult_ce_i : std_logic;
  signal s_complx_mult_d_a_i : std_logic_vector(31 downto 0) := (others => '0');
  signal s_complx_mult_d_b_i : std_logic_vector(31 downto 0) := (others => '0');
  signal s_complx_mult_vld_o : std_logic;
  signal s_complx_mult_d_o   : std_logic_vector(79 downto 0) := (others => '0');
  signal s_complx_mult_real  : std_logic_vector(32 downto 0) := (others => '0');
  signal s_complx_mult_imag  : std_logic_vector(32 downto 0) := (others => '0');
  -- Peak detector
  signal s_peak_rst_i        : std_logic;
  signal s_peak_a_i          : std_logic_vector(15 downto 0);
  signal s_peak_b_i          : std_logic_vector(15 downto 0);
  signal s_peak_o            : std_logic_vector(15 downto 0);

begin

  --'enable' circuit (when up/downstream Workers ready and operating)
  s_enable <= ctl_in.is_operating and in_in.ready and out_in.ready;

  -----------------------------------------------------------------------------
  -- WSI Port assignments
  -----------------------------------------------------------------------------
  -- Take (when up/downstream Workers ready and operating)
  in_out.take <= s_enable;

  s_in_som   <= in_in.som;
  s_in_eom   <= in_in.eom;
  s_in_valid <= in_in.valid;

  -- Delay line to match the latency of the primitive for non-sample data
  delay_inst : ocpi.wsi.delayline
  generic map (
    g_latency     => c_complex_mixer_latency)
  port map (
    i_clk         => ctl_in.clk,
    i_reset       => ctl_in.reset,
    i_enable      => out_in.ready,
    i_ready       => in_in.ready,
    i_som         => in_in.som,
    i_eom         => in_in.eom,
    i_opcode      => s_in_opcode_slv,
    i_valid       => in_in.valid,
    i_byte_enable => in_in.byte_enable,
    i_data        => in_in.data,
    i_eof         => in_in.eof,
    o_ready       => open,
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

  -----------------------------------------------------------------------------
  -- MUX to select output of complex multiplier, input data, or NCO.
  -- Use worker properties 'enable' and 'data_select' to switch between data sources.
  -- NORMAL mode:
  --    's_complx_mult_imag(32 downto 17)' to the upper 16 bits of out_out.data
  --    's_complx_mult_real(32 downto 17)' to the lower 16 bits of out_out.data
  -- 'BYPASS' mode:
  --    Input data (ENABLE=0 & DATA_SELECT=0) or
  --    NCO data   (ENABLE=0 & DATA_SELECT=1)
  -----------------------------------------------------------------------------
  out_out.data <= s_complx_mult_imag(32 downto 17) & s_complx_mult_real(32 downto 17)
                  when (props_in.enable = '1')   -- NORMAL
                  else s_data when (props_in.data_select = '0')  -- BYPASS:Input
                  else s_dds_imag & s_dds_real;  -- BYPASS:NCO output


  -- 's_reset_n_i' is the negation of 'ctl_in.reset' for use with Vivado cores
  s_reset_n_i <= not(ctl_in.reset);

  -----------------------------------------------------------------------------
  -- Xilinx Vivado IP: DDS (NCO) instance
  -----------------------------------------------------------------------------

  -- 'Prime' the DDS Compiler Xilinx Core with 7 clock cycles upon reset or 'phs_inc' is written
  s_dds_ce_i <= s_enable and in_in.valid;

  flag_dds_primer_counter : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1' or props_in.phs_inc_written = '1') then
        s_dds_primer_cnt <= (others => '0');
        s_dds_vld        <= '0';
      elsif (s_dds_ce_i = '1') then
        if s_dds_primer_cnt = c_dds_core_latency then
          s_dds_vld <= '1';
        else
          s_dds_primer_cnt <= s_dds_primer_cnt + 1;
        end if;
      end if;
    end if;
  end process;

  -- Design note:
  -- The 'aresetn' is included, as it was determined that the primitive
  -- contained 'stale' data after an application execution
  inst_DDS : component dds_compiler
    port map (
      aclk                => ctl_in.clk,
      aresetn             => s_reset_n_i,
      s_axis_phase_tvalid => s_dds_ce_i,
      s_axis_phase_tdata  => std_logic_vector(props_in.phs_inc),
      m_axis_data_tvalid  => open,
      m_axis_data_tdata   => s_dds_sine_cosine_o
      );

  s_dds_imag <= s_dds_sine_cosine_o(31 downto 16);
  s_dds_real <= s_dds_sine_cosine_o(15 downto 0);

  -----------------------------------------------------------------------------
  -- Xilinx Vivado IP: Complex Multiplier instance
  -----------------------------------------------------------------------------

  -- Enable multiplier primitive (when enabled and input valid and DDS/NCO output valid)
  s_complx_mult_ce_i <= (s_dds_ce_i and s_dds_vld) or (s1_complx_mult_ce_i and out_in.ready);

  -- Register enable when taking samples
  mult_enable_reg : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
        s1_complx_mult_ce_i <= '0';
      elsif (s_enable = '1' and s_dds_vld = '1') then  --taking samples
        s1_complx_mult_ce_i <= s_complx_mult_ce_i;
      end if;
    end if;
  end process;

  -- The Complex_Multipliers inputs have the real and imaginary sections swapped
  s_complx_mult_d_a_i <= in_in.data(31 downto 16) & in_in.data(15 downto 0);

  -- Output of NCO assigned to input "B" of complex multiplier
  s_complx_mult_d_b_i <= s_dds_sine_cosine_o;

  -- Fully-pipelined latency: 6
  -- Output Width: 33 (truncated)
  inst_ComplexMult : component complex_multiplier
    port map (
      aclk               => ctl_in.clk,
      aclken             => '1',
      aresetn            => s_reset_n_i,
      s_axis_a_tvalid    => s_complx_mult_ce_i,
      s_axis_a_tdata     => s_complx_mult_d_a_i,
      s_axis_b_tvalid    => s_complx_mult_ce_i,
      s_axis_b_tdata     => s_complx_mult_d_b_i,
      m_axis_dout_tvalid => s_complx_mult_vld_o,
      m_axis_dout_tdata  => s_complx_mult_d_o
      );

  -- Parse output of complex multipler for imag and real components
  s_complx_mult_imag <= s_complx_mult_d_o(72 downto 40);
  s_complx_mult_real <= s_complx_mult_d_o(32 downto 0);

  -----------------------------------------------------------------------------
  -- Peak Detection primitive. Value is cleared when read
  -----------------------------------------------------------------------------
  s_peak_rst_i <= ctl_in.reset or std_logic(props_in.peak_read);
  s_peak_a_i   <= std_logic_vector(resize(signed(s_complx_mult_real), 16))  -- default output
                  when (props_in.enable = '1') else in_in.data(15 downto 0);  --BYPASS (ENABLE=0)
  s_peak_b_i <= std_logic_vector(resize(signed(s_complx_mult_imag), 16))  -- default output
                when (props_in.enable = '1') else in_in.data(31 downto 16);  --BYPASS (ENABLE=0)

  pm_gen : if its(PEAK_MONITOR) generate

    pd : util_prims.util_prims.peakDetect
      port map (
        CLK_IN   => ctl_in.clk,
        RST_IN   => s_peak_rst_i,
        EN_IN    => s_complx_mult_vld_o,
        A_IN     => s_peak_a_i,
        B_IN     => s_peak_b_i,
        PEAK_OUT => s_peak_o);

    props_out.peak <= signed(s_peak_o);

  end generate pm_gen;

  no_pm_gen : if its(not PEAK_MONITOR) generate
    props_out.peak <= (others => '0');
  end generate no_pm_gen;

end rtl;
