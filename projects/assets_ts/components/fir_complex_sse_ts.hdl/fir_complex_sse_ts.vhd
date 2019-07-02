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
-- FIR Complex SSE Filter
-------------------------------------------------------------------------------
--
-- Description:
--
-- The FIR Complex SSE (Systolic Symmetric Even) worker inputs complex signed
-- samples and filters them based upon a programmable number of coefficient tap
-- values. The underlying FIR Filter implementation makes use of a symmetric
-- systolic structure to construct a filter with an even number of taps and
-- symmetry about its midpoint. Thus the NUM_TAPS parameter defines N
-- coefficient values. Care should be taken to make sure that the COEFF_WIDTH
-- parameter is <= the type (size) of the taps property.
--
-- This implementation uses NUM_TAPS/2 multipliers to process input data at the
-- clock rate - i.e. this worker can handle a new input value every clock cycle.
-- It is unnecessary to round the output data from this filter.
--
-- This filter will produce valid outputs one clock after each valid
-- input, but care must be exercised when attempting to align outputs
-- according to the filter's actual group delay and propagation delay.
--
-- For an FIR filter with symmetric impulse response we are guaranteed
-- to have linear phase response and thus constant group delay vs
-- frequency. In general the group delay will be equal to (NUM_TAPS-1)/2 where
-- N is the number of filter taps.
--
-- The filter topology itself will add some propagation delay to the
-- response. For this design the total delay from an impulse input to the
-- beginning of the impulse response will be NUM_TAPS/2 + 4 samples
--
-- The worker only outputs samples after the delay has occurred. During flush
-- opcodes, the worker continues to produce valid data until the
-- pipeline is empty. During the sync opcode, the pipeline is emptied, but
-- no valid data is produced. During flush and sync opcodes, the opcode
-- is passed along after the pipeline is empty.
--
-- The worker passes along and does not modify time and interval operations.
-------------------------------------------------------------------------------

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.types.all; use ocpi.util.all;-- remove this to avoid all ocpi name collisions

architecture rtl of worker is
  
  --FIR primitive hard codes address width to 16
  constant c_fir_prim_addr_width   : positive := 16;
  constant c_num_sections          : positive := to_integer(NUM_TAPS)/2;
  constant c_data_width            : positive := to_integer(DATA_WIDTH);
  constant c_coeff_width           : positive := to_integer(COEFF_WIDTH);
  constant c_taps_bytes            : positive := ushort_t'length/8;
  constant c_latency               : positive := to_integer(LATENCY);
  constant c_opcode_width          : positive := width_for_max(to_integer(ocpi_max_opcode_in));

  signal s_raw_byte_enable         : std_logic_vector(props_in.raw.byte_enable'length-1 downto 0);
  signal s_raw_addr                : std_logic_vector(c_fir_prim_addr_width-1 downto 0);
  signal s_fir_wdata               : std_logic_vector(c_coeff_width-1 downto 0);
  
  signal s_do_work                 : std_logic;
  signal s_samples_opcode          : std_logic;
  signal s_samples_eom             : Bool_t;
  signal s_dly_enable, s_dly_eof   : Bool_t;
  signal s_dly_som, s_dly_eom      : Bool_t;
  signal s_dly_ready, s_dly_valid  : Bool_t;
  signal s_dly_data                : std_logic_vector(ocpi_port_in_data_width-1 downto 0);
  signal s_dly_byte_enable         : std_logic_vector(ocpi_port_in_MByteEn_width-1 downto 0);
  signal s_dly_opcode              : std_logic_vector(c_opcode_width-1 downto 0);

  signal s_i_idata, s_q_idata      : std_logic_vector(c_data_width-1 downto 0);
  signal s_i_odata, s_q_odata      : std_logic_vector(c_data_width-1 downto 0);

  signal s_peak_rst_in             : std_logic;
  signal s_peak_a_in, s_peak_b_in  : std_logic_vector(c_data_width-1 downto 0);
  signal s_peak_out                : std_logic_vector(c_data_width-1 downto 0);
  
  type   state_type is (idle_s, samples_s, flush_s, bypass_s);
  signal state : state_type;

begin

  s_raw_byte_enable   <= props_in.raw.byte_enable;
  --The FIR primitive uses TAPS_BYTES_c addresses, but the raw properties interface uses
  --single byte addresses, so the incoming address must be shifted
  s_raw_addr          <= std_logic_vector(props_in.raw.address(c_fir_prim_addr_width-1 downto 0)
                                          srl c_taps_bytes-1);
  props_out.raw.done  <= '1';

  -- handle coefficient resizing and byte enables
  be_16_gen : if c_coeff_width <= 16 generate
    --Input byte enable decode
    s_fir_wdata <= props_in.raw.data(c_coeff_width-1 downto 0) when (s_raw_byte_enable = "0011") else
                   props_in.raw.data(c_coeff_width-1+16 downto 16) when (s_raw_byte_enable = "1100") else (others => '0');
  end generate be_16_gen;  
  
  s_samples_opcode  <= in_in.valid and to_bool(in_in.opcode=ComplexShortWithMetadata_samples_op_e);
  s_samples_eom     <= in_in.eom and s_samples_opcode;
  
  s_do_work   <= in_in.ready and out_in.ready;

  fsm_seq: process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        state <= idle_s;
      elsif its(s_do_work) then
        if state = idle_s then
          if its(s_samples_opcode) then
            state <= samples_s;
          else
            state <= bypass_s;
          end if;
          
        elsif state = samples_s then
          if its(not s_samples_opcode) then
            state <= flush_s;
          end if;
          
        elsif state = flush_s then
          if its(out_in.ready and s_dly_ready and s_dly_eom) then
            if its(in_in.ready) then
              state <= bypass_s;
            else
              state <= idle_s;
            end if;
          end if;

        elsif state = bypass_s then
          if its(in_in.eom) then
            state <= idle_s;
          end if;
        end if;
        
      elsif its(in_in.eof) then
        if state = samples_s then
          state <= flush_s;
        end if;
      end if;
    end if;
  end process;

  fsm_comb : process(state, s_do_work, in_in.valid, in_in.data, in_in.som, in_in.eom, in_in.eof,
                     s_samples_opcode, s_dly_ready, s_dly_valid, s_q_odata, s_i_odata, s_dly_som,
                     s_dly_eom, s_dly_eof, out_in.ready)
  begin
    -- FSM Output Defaults
    s_dly_enable        <= bfalse;
    in_out.take         <= s_do_work;
    out_out.give        <= s_do_work;
    out_out.valid       <= in_in.valid;
    out_out.data        <= in_in.data;
    out_out.eom         <= in_in.eom;
    out_out.eof         <= in_in.eof;
    out_out.opcode      <= in_in.opcode;
    s_i_idata           <= (others => '0');
    s_q_idata           <= (others => '0');
    
    -- Output Assertions
    case (state) is
      
      when idle_s =>
        s_dly_enable        <= bfalse;
        in_out.take         <= bfalse;
        out_out.give        <= bfalse;
        out_out.valid       <= bfalse;
        
      when samples_s =>
        s_dly_enable        <= s_do_work and s_samples_opcode;
        in_out.take         <= s_do_work and s_samples_opcode;
        out_out.give        <= s_do_work and s_samples_opcode and s_dly_ready;
        out_out.valid       <= s_do_work and s_samples_opcode and s_dly_valid;
        out_out.data        <= s_q_odata & s_i_odata;
        out_out.eom         <= '0';
        out_out.eof         <= '0';
        out_out.opcode      <= ComplexShortWithMetadata_samples_op_e;
        s_i_idata           <= in_in.data(c_data_width-1 downto 0);
        s_q_idata           <= in_in.data(2*c_data_width-1 downto c_data_width);
        
      when flush_s =>
        s_dly_enable        <= out_in.ready;
        in_out.take         <= bfalse;
        out_out.give        <= out_in.ready and s_dly_ready;
        out_out.valid       <= out_in.ready and s_dly_valid;
        out_out.data        <= s_q_odata & s_i_odata;
        out_out.eom         <= s_dly_eom;
        out_out.eof         <= s_dly_eof;
        out_out.opcode      <= ComplexShortWithMetadata_samples_op_e;

      when bypass_s =>
        -- Outputs are default
        
    end case;
  end process;

  -- FIR Systolic Symmetrical Even primitive
  i_fir : dsp_prims.dsp_prims.fir_systolic_sym_even
    generic map (
      NUM_SECTIONS  => c_num_sections,
      DATA_WIDTH    => c_data_width,
      COEFF_WIDTH   => c_coeff_width,
      ACC_PREC      => c_data_width + c_coeff_width + 1 + integer(ceil(log2(real(c_num_sections)))),
      DATA_ADDR     => (others => '0'),
      PTR_ADDR      => (others => '0'),
      USE_COEFF_PTR => false)
    port map (
      CLK      => ctl_in.clk,
      RST      => ctl_in.reset,
      DIN      => s_i_idata,
      DIN_VLD  => s_dly_enable,
      DOUT     => s_i_odata,
      DOUT_VLD => open,
      ADDR     => s_raw_addr,
      RDEN     => '0',
      WREN     => props_in.raw.is_write,
      RDATA    => open,
      WDATA    => s_fir_wdata);

  -- FIR Systolic Symmetrical Even primitive
  q_fir : dsp_prims.dsp_prims.fir_systolic_sym_even
    generic map (
      NUM_SECTIONS  => c_num_sections,
      DATA_WIDTH    => c_data_width,
      COEFF_WIDTH   => c_coeff_width,
      ACC_PREC      => c_data_width + c_coeff_width + 1 + integer(ceil(log2(real(c_num_sections)))),
      DATA_ADDR     => (others => '0'),
      PTR_ADDR      => (others => '0'),
      USE_COEFF_PTR => false)
    port map (
      CLK      => ctl_in.clk,
      RST      => ctl_in.reset,
      DIN      => s_q_idata,
      DIN_VLD  => s_dly_enable,
      DOUT     => s_q_odata,
      DOUT_VLD => open,
      ADDR     => s_raw_addr,
      RDEN     => '0',
      WREN     => props_in.raw.is_write,
      RDATA    => open,
      WDATA    => s_fir_wdata);
  
  delay_inst : ocpi.wsi.delayline
  generic map (
    g_latency     => c_latency)
  port map (
    i_clk         => ctl_in.clk,
    i_reset       => ctl_in.reset,
    i_enable      => s_dly_enable,
    i_ready       => s_samples_opcode,
    i_som         => '0',
    i_eom         => s_samples_eom,
    i_opcode      => (c_opcode_width-1 downto 0 => '0'),
    i_valid       => s_samples_opcode,
    i_byte_enable => (ocpi_port_in_MByteEn_width-1 downto 0 => '0'),
    i_data        => (ocpi_port_in_data_width-1 downto 0 => '0'),
    i_eof         => in_in.eof,
    o_ready       => s_dly_ready,
    o_som         => open,
    o_eom         => s_dly_eom,
    o_opcode      => s_dly_opcode,      --not used, but build fails without it
    o_valid       => s_dly_valid,
    o_byte_enable => s_dly_byte_enable, --not used, but build fails without it
    o_data        => s_dly_data,        --not used, but build fails without it
    o_eof         => s_dly_eof);


  -- Peak Detection primitive. Value is cleared when read
  s_peak_rst_in <= ctl_in.reset or std_logic(props_in.peak_read);
  s_peak_a_in   <= std_logic_vector(resize(signed(s_i_odata),16));
  s_peak_b_in   <= std_logic_vector(resize(signed(s_q_odata),16));

  pm_gen : if its(PEAK_MONITOR) generate
    pd : util_prims.util_prims.peakDetect
      port map (
        CLK_IN   => ctl_in.clk,
        RST_IN   => s_peak_rst_in,
        EN_IN    => s_dly_valid,
        A_IN     => s_peak_a_in,
        B_IN     => s_peak_b_in,
        PEAK_OUT => s_peak_out);

    props_out.peak <= signed(s_peak_out);
  end generate pm_gen;

  no_pm_gen : if its(not PEAK_MONITOR) generate
    props_out.peak <= (others => '0');
  end generate no_pm_gen;

end rtl;
