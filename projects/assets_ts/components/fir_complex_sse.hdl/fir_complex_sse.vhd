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
-- symmetry about its midpoint. Thus the NUM_TAPS_p parameter defines N
-- coefficient values. Care should be taken to make sure that the COEFF_WIDTH_p
-- parameter is <= the type (size) of the taps property.
--
-- This implementation uses NUM_TAPS_p/2 multipliers to process input data at the
-- clock rate - i.e. this worker can handle a new input value every clock cycle.
-- It is unnecessary to round the output data from this filter.
--
-- This filter will produce valid outputs one clock after each valid
-- input, but care must be exercised when attempting to align outputs
-- according to the filter's actual group delay and propagation delay.
--
-- For an FIR filter with symmetric impulse response we are guaranteed
-- to have linear phase response and thus constant group delay vs
-- frequency. In general the group delay will be equal to (N-1)/2 where
-- N is the number of filter taps.
--
-- The filter topology itself will add some propagation delay to the
-- response. For this design the total delay from an impulse input to the
-- beginning of the impulse response will be NUM_TAPS_p/2 + 4 samples
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
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions

architecture rtl of fir_complex_sse_worker is

  constant NUM_SECTIONS_c : positive := to_integer(NUM_TAPS_p)/2;
  constant DATA_WIDTH_c   : positive := to_integer(DATA_WIDTH_p);
  constant COEFF_WIDTH_c  : positive := to_integer(COEFF_WIDTH_p);
  constant TAPS_BYTES_c   : positive := ushort_t'length/8;
  constant LATENCY_c      : positive := to_integer(LATENCY_p);
  constant OPCODE_WIDTH_c : positive := ocpi.util.width_for_max(ComplexShortWithMetadata_OpCode_t'pos(ComplexShortWithMetadata_OpCode_t'right));
  constant GROUP_DELAY_c  : positive := to_integer(GROUP_DELAY_p);

  constant PORT_WIDTH_c    : positive := ocpi_port_in_data_width;
  constant BYTE_EN_WIDTH_c : positive := ocpi_port_in_MByteEn_width;
  
  signal enable                  : std_logic;
  signal idata_vld               : std_logic;
  signal opcode_vld              : std_logic;
  signal som                     : std_logic;
  signal eom                     : std_logic;
  signal valid                   : std_logic;
  signal data                    : std_logic_vector(ocpi_port_in_data_width-1 downto 0);
  signal byte_enable             : std_logic_vector(ocpi_port_in_MByteEn_width-1 downto 0);
  signal in_opcode_slv           : std_logic_vector(OPCODE_WIDTH_c-1 downto 0);
  signal out_opcode_slv          : std_logic_vector(OPCODE_WIDTH_c-1 downto 0);
  signal out_opcode_int          : integer;
  signal fir_wdata               : std_logic_vector(COEFF_WIDTH_c-1 downto 0);
  signal fir_rdata               : std_logic_vector(COEFF_WIDTH_c-1 downto 0);
  signal i_idata                 : std_logic_vector(DATA_WIDTH_c-1 downto 0);
  signal q_idata                 : std_logic_vector(DATA_WIDTH_c-1 downto 0);
  signal i_odata                 : std_logic_vector(DATA_WIDTH_c-1 downto 0);
  signal q_odata                 : std_logic_vector(DATA_WIDTH_c-1 downto 0);
  signal prim_rdy                : std_logic;
  signal odata_vld               : std_logic;
  signal missed_odata_vld        : std_logic := '0';
  signal peak_out                : std_logic_vector(15 downto 0);
  signal peak_rst_in             : std_logic;
  signal peak_a_in               : std_logic_vector(15 downto 0);
  signal peak_b_in               : std_logic_vector(15 downto 0);
  signal raw_byte_enable         : std_logic_vector(3 downto 0);
  signal raw_addr                : std_logic_vector(props_in.raw.address'length-1 downto 0);
  signal prim_vld                : std_logic;
  signal raw_done_r              : std_logic;
  signal group_delay_max         : std_logic;
  signal group_delay_zero        : std_logic;
  signal group_delay_counter     : unsigned(31 downto 0);
  signal flush_opcode            : std_logic;
  signal drop_opcode             : std_logic;
  signal flush_in_progress       : std_logic;
  signal flush_in_progress_r     : std_logic;
  signal flush_in_progress_redge : std_logic;

begin

  raw_byte_enable     <= props_in.raw.byte_enable;
  --The FIR primitive uses TAPS_BYTES_c addresses, but the raw properties interface uses
  --single byte addresses, so the incoming address must be shifted
  raw_addr            <= std_logic_vector(props_in.raw.address srl TAPS_BYTES_c-1);
  props_out.raw.done  <= '1';
  
  --opcodes which require data to be flushed out of the FIR primitives
  flush_opcode <= in_in.eof or (in_in.ready and to_bool(in_in.opcode=ComplexShortWithMetadata_flush_op_e));

  --opcodes which require data inside the FIR primitives to be dropped
  drop_opcode <= in_in.ready and
                 to_bool(in_in.opcode=ComplexShortWithMetadata_sync_op_e);

  -- 'enable' circuit when up/downstream Workers ready and operating
  enable <= ctl_in.is_operating and in_in.ready and out_in.ready;

  -- 'idata_vld' enables primitives when enabled and input valid or we're flushing/dropping
  idata_vld <= enable and
               ((in_in.valid and to_bool(in_in.opcode=ComplexShortWithMetadata_samples_op_e)) or
                ((flush_opcode or drop_opcode) and not group_delay_zero));

  -- 'opcode_vld' enables delayline primitive
  opcode_vld <= in_in.valid and to_bool(in_in.opcode/=ComplexShortWithMetadata_samples_op_e);

  -- 'din_vld' is enable for delay line primitive
  -- (wait for group delay zero when flushing)
  prim_rdy <= out_in.ready and group_delay_zero when its(flush_opcode) or its(drop_opcode) else
             out_in.ready;
  
  -- Take when up/downstream Workers ready and operating and when flushing,
  -- wait until flush complete (group delay zero)
  in_out.take <= enable and group_delay_zero when its(flush_opcode) or its(drop_opcode) else enable;

  -- Primitive output valid only once group delay samples have been processed
  -- unless flushing
  prim_vld <= (odata_vld or missed_odata_vld) and not group_delay_zero when its(flush_opcode) else
              group_delay_max and (odata_vld or missed_odata_vld);
  
  -- Give when downstream worker ready and operating and
  -- FIR primitive has valid output or
  -- non-sample valid data or 
  -- zlms
  out_out.give <= ctl_in.is_operating and out_in.ready and
                  ((prim_vld and not drop_opcode)or
                   valid or
                   ((som or eom) and not valid));

  -- Valid when FIR primitive has valid output or non-sample valid data
  out_out.valid <= prim_vld or valid;                            

  -- Translate opcode to slv for delay primitive
  -- Xilinx/ISE 14.6 synthesis doesn't do the t'val(x) function properly
  -- Hence this workaround
  in_opcode_slv <=
    std_logic_vector(to_unsigned(1,in_opcode_slv'length)) when in_in.opcode = ComplexShortWithMetadata_time_op_e else
    std_logic_vector(to_unsigned(2,in_opcode_slv'length)) when in_in.opcode = ComplexShortWithMetadata_interval_op_e else
    std_logic_vector(to_unsigned(3,in_opcode_slv'length)) when in_in.opcode = ComplexShortWithMetadata_flush_op_e else
    std_logic_vector(to_unsigned(4,in_opcode_slv'length)) when in_in.opcode = ComplexShortWithMetadata_sync_op_e else
    std_logic_vector(to_unsigned(0,in_opcode_slv'length)); --in_in.opcode = ComplexShortWithMetadata_samples_op_e

  -- Delay line to match the latency of the primitive for non-sample data
  delay : ocpi.wsi.delayline
  generic map (
    LATENCY         => LATENCY_c)
  port map (
    CLK             => ctl_in.clk,
    RESET           => ctl_in.reset,
    IS_OPERATING    => ctl_in.is_operating,
    IN_READY        => in_in.ready,
    IN_SOM          => in_in.som,
    IN_EOM          => in_in.eom,
    IN_OPCODE       => in_opcode_slv,
    IN_VALID        => opcode_vld,
    IN_BYTE_ENABLE  => in_in.byte_enable,
    IN_DATA         => in_in.data,
    OUT_READY       => prim_rdy,
    OUT_SOM         => som,
    OUT_EOM         => eom,
    OUT_OPCODE      => out_opcode_slv,
    OUT_VALID       => valid,
    OUT_BYTE_ENABLE => byte_enable,
    OUT_DATA        => data);

  flush_in_progress <= flush_opcode and prim_vld;
  flushInProgress : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        flush_in_progress_r <= '0';
      else
        flush_in_progress_r <= flush_in_progress;
      end if;
    end if;
  end process;
  
  flush_in_progress_redge <= flush_in_progress and not flush_in_progress_r;

  --som is passed along from input to output and at the beginning of flushing
  out_out.som         <= som or flush_in_progress_redge;
  --eom is passed along from input to output and at the end of flushing
  out_out.eom         <= eom or (flush_opcode and to_bool(group_delay_counter=1));
  out_out.byte_enable <= byte_enable;
  --data comes from primitive or delayline (when delayline is valid)
  out_out.data        <= std_logic_vector(resize(signed(q_odata),16)) &
                         std_logic_vector(resize(signed(i_odata),16)) when its(not valid) else data;
  
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

  -- handle coefficient resizing and byte enables
  be_16_gen : if COEFF_WIDTH_c <= 16 and COEFF_WIDTH_c > 8 generate
    --Input byte enable decode
    be_input : process (raw_byte_enable, props_in.raw.data)
    begin
      case raw_byte_enable is
        when "0011" => fir_wdata <= std_logic_vector(props_in.raw.data(COEFF_WIDTH_c-1    downto  0));
        when "0110" => fir_wdata <= std_logic_vector(props_in.raw.data(COEFF_WIDTH_c-1+8  downto  8));
        when "1100" => fir_wdata <= std_logic_vector(props_in.raw.data(COEFF_WIDTH_c-1+16 downto 16));
        when others => fir_wdata <= (others => '0');
      end case;
    end process be_input;
  end generate be_16_gen;

  be_8_gen : if COEFF_WIDTH_c <= 8 generate
    --Input byte enable decode
    be_input : process (raw_byte_enable, props_in.raw.data)
    begin
      case raw_byte_enable is
        when "0011" => fir_wdata <= std_logic_vector(props_in.raw.data(COEFF_WIDTH_c-1    downto  0));
        when "0110" => fir_wdata <= std_logic_vector(props_in.raw.data(COEFF_WIDTH_c-1+8  downto  8));
        when "1100" => fir_wdata <= std_logic_vector(props_in.raw.data(COEFF_WIDTH_c-1+16 downto 16));
        when others => fir_wdata <= (others => '0');
      end case;
    end process be_input;
  end generate be_8_gen;

  -- Input data to primitive (zeros when flushing or dropping)
  i_idata <= (others => '0') when its(flush_opcode) or its(drop_opcode) else in_in.data(DATA_WIDTH_c-1 downto 0);
  q_idata <= (others => '0') when its(flush_opcode) or its(drop_opcode) else in_in.data(DATA_WIDTH_c-1+16 downto 16);

  -- FIR Systolic Symmetrical Even primitive
  i_fir : dsp_prims.dsp_prims.fir_systolic_sym_even
    generic map (
      NUM_SECTIONS  => NUM_SECTIONS_c,
      DATA_WIDTH    => DATA_WIDTH_c,
      COEFF_WIDTH   => COEFF_WIDTH_c,
      ACC_PREC      => DATA_WIDTH_c + COEFF_WIDTH_c + 1 + integer(ceil(log2(real(NUM_SECTIONS_c)))),
      DATA_ADDR     => (others => '0'),
      PTR_ADDR      => (others => '0'),
      USE_COEFF_PTR => false)
    port map (
      CLK      => ctl_in.clk,
      RST      => ctl_in.reset,
      DIN      => i_idata,
      DIN_VLD  => idata_vld,
      DOUT     => i_odata,
      DOUT_VLD => odata_vld,
      ADDR     => raw_addr,
      RDEN     => props_in.raw.is_read,
      WREN     => props_in.raw.is_write,
      RDATA    => fir_rdata,
      WDATA    => fir_wdata);

  -- FIR Systolic Symmetrical Even primitive
  q_fir : dsp_prims.dsp_prims.fir_systolic_sym_even
    generic map (
      NUM_SECTIONS  => NUM_SECTIONS_c,
      DATA_WIDTH    => DATA_WIDTH_c,
      COEFF_WIDTH   => COEFF_WIDTH_c,
      ACC_PREC      => DATA_WIDTH_c + COEFF_WIDTH_c + 1 + integer(ceil(log2(real(NUM_SECTIONS_c)))),
      DATA_ADDR     => (others => '0'),
      PTR_ADDR      => (others => '0'),
      USE_COEFF_PTR => false)
    port map (
      CLK      => ctl_in.clk,
      RST      => ctl_in.reset,
      DIN      => q_idata,
      DIN_VLD  => idata_vld,
      DOUT     => q_odata,
      DOUT_VLD => open,
      ADDR     => raw_addr,
      RDEN     => '0',
      WREN     => props_in.raw.is_write,
      RDATA    => open,
      WDATA    => fir_wdata);

  backPressure : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if(ctl_in.reset = '1' or out_in.ready = '1') then
        missed_odata_vld <= '0';
      elsif (out_in.ready = '0' and odata_vld = '1') then
        missed_odata_vld <= '1';
      end if;
    end if;
  end process backPressure;

  -- Peak Detection primitive. Value is cleared when read
  peak_rst_in         <= ctl_in.reset or std_logic(props_in.peak_read);
  peak_a_in           <= std_logic_vector(resize(signed(i_odata),16));
  peak_b_in           <= std_logic_vector(resize(signed(q_odata),16));

  pd : util_prims.util_prims.peakDetect
    port map (
      CLK_IN   => ctl_in.clk,
      RST_IN   => peak_rst_in,
      EN_IN    => odata_vld,
      A_IN     => peak_a_in,
      B_IN     => peak_b_in,
      PEAK_OUT => peak_out);

  props_out.peak <= signed(peak_out);

  groupDelayCounterProc : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        group_delay_counter <= (others => '0');
      elsif its(enable) then
        --Decrement group delay counter when flushing or dropping
        if (its(flush_opcode) or its(drop_opcode)) and group_delay_counter > 0 then
          group_delay_counter <= group_delay_counter - 1;
        --Increment group delay counter when taking samples data
        elsif group_delay_counter < GROUP_DELAY_c  and in_in.opcode = ComplexShortWithMetadata_samples_op_e then
          group_delay_counter <= group_delay_counter + 1;
        end if;
      end if;
    end if;
  end process;
  group_delay_max <= to_bool(group_delay_counter >= GROUP_DELAY_c);
  group_delay_zero <= to_bool(group_delay_counter = 0);

end rtl;
