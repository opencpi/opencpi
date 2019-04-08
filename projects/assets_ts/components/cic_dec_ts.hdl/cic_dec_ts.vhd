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
library i2c; use i2c.i2c.all;

architecture rtl of worker is

  constant N_c              : integer  := to_integer(unsigned(N));
  constant M_c              : integer  := to_integer(unsigned(M));
  constant R_c              : integer  := to_integer(unsigned(R));
  constant count_ones_in_R  : natural  := count_ones(from_ushort(R));
  constant DATA_WIDTH_c     : integer  := to_integer(unsigned(DATA_WIDTH));
  constant ACC_WIDTH_c      : integer  := to_integer(unsigned(ACC_WIDTH));
  constant LATENCY_c        : integer  := to_integer(LATENCY_p);
  constant GROUP_DELAY_c    : positive := to_integer(GROUP_DELAY_p);
  constant OPCODE_WIDTH_c   : positive := ocpi.util.width_for_max(ComplexShortWithMetadata_OpCode_t'pos(ComplexShortWithMetadata_OpCode_t'right));
  constant RCNT_WIDTH       : integer  := integer(ceil(log(real(to_integer(unsigned(R))))/log(2.0)))+1;

  signal prim_rst           : std_logic;
  signal din_rdy            : std_logic;
  signal in_out_take        : std_logic;
  signal din_vld, dout_vld  : std_logic;
  signal prim_vld           : std_logic;
  signal i_in, q_in         : std_logic_vector(DATA_WIDTH_c-1 downto 0);
  signal i_out, q_out       : std_logic_vector(DATA_WIDTH_c-1 downto 0);
  signal in_opcode_slv      : std_logic_vector(OPCODE_WIDTH_c-1 downto 0);
  signal out_opcode_slv     : std_logic_vector(OPCODE_WIDTH_c-1 downto 0);
  signal out_opcode_int     : natural;
  signal delayline_eom      : std_logic;
  signal delayline_valid    : std_logic;
  signal delayline_data     : std_logic_vector(ocpi_port_in_data_width-1 downto 0);
  signal group_delay_max    : std_logic;
  signal group_delay_rcnt   : std_logic;
  signal group_delay_zero   : std_logic;
  signal group_delay_counter: ushort_t; --type of group delay parameter
  signal samples_opcode_in  : std_logic;
  signal samples_opcode_out : std_logic;
  signal flush_opcode_in    : std_logic;
  signal drop_opcode_in     : std_logic;
  signal interval_opcode_out: std_logic;
  signal interval_data      : std_logic_vector(ocpi_port_in_data_width-1 downto 0);
  signal rcnt               : unsigned(RCNT_WIDTH-1 downto 0);
  signal flush_eom          : std_logic;

begin
  --Opcodes which require data to be flushed out of the CIC primitives 
  flush_opcode_in <= in_in.eof or
                     (in_in.ready and to_bool(in_in.opcode=ComplexShortWithMetadata_flush_op_e));

  --opcodes which require data inside the CIC primitives to be dropped
  drop_opcode_in <= in_in.ready and to_bool(in_in.opcode=ComplexShortWithMetadata_sync_op_e);

  --opcode to be processed by the CIC primitive
  samples_opcode_in  <= in_in.valid and to_bool(in_in.opcode=ComplexShortWithMetadata_samples_op_e);
  samples_opcode_out <= to_bool(out_opcode_int=0);

  --interval opcode output
  interval_opcode_out <= to_bool(out_opcode_int=2);
  
  -- Take when allowed and primitive is ready and when flushing,
  -- wait until flush complete (group delay = rcnt)
  in_out_take <= din_rdy and in_in.ready and out_in.ready and group_delay_rcnt when its(flush_opcode_in) else
                 din_rdy and in_in.ready and out_in.ready;
  in_out.take <= in_out_take;
  
  -- 'din_vld' enables primitives when valid samples on input or we're flushing/dropping
  din_vld <= (out_in.ready and samples_opcode_in) or (flush_opcode_in and not group_delay_rcnt);

  -- Primitive output valid only once group delay samples have been processed
  -- unless flushing
  prim_vld <= dout_vld and not group_delay_zero when its(flush_opcode_in) else dout_vld and group_delay_max;
  
  -- Give to output port when output of CIC or delayline primitive is valid and output is allowed
  out_out.give  <= out_in.ready and ((prim_vld and samples_opcode_out) or
                                     (delayline_valid and not samples_opcode_out) or
                                     delayline_eom);
  
  out_out.valid <= prim_vld when its(samples_opcode_out) else delayline_valid;
  
  --eom is passed along from input to output and at the end of flushing
  out_out.eom <= delayline_eom or flush_eom;--(flush_opcode_in and group_delay_rcnt);

  --generate interval data
  data_width_32 : if ocpi_port_in_data_width = 32 generate
    
    --For powers of 2, division is a shift
    r_is_power_of_2 : if count_ones_in_R = 1 generate
      interval_data <= std_logic_vector(unsigned(delayline_data) srl integer(log2(real(R_c))));
    end generate;

    --Non-powers of 2 currently not supported. Interval data passed along.
    r_is_not_power_of_2 : if count_ones_in_R /= 1 generate
      interval_data <= delayline_data;
    end generate;
    
  end generate;
  
  --data comes from primitive or delayline (when delayline is valid)
  out_out.data <= std_logic_vector(resize(signed(q_out),16)) &
                  std_logic_vector(resize(signed(i_out),16)) when its(samples_opcode_out) else
                  interval_data                              when its(interval_opcode_out)else
                  delayline_data;

  -- Input data to primitive (zeros when flushing)
  i_in <= (others => '0') when its(flush_opcode_in) else in_in.data(DATA_WIDTH_c-1 downto 0);
  q_in <= (others => '0') when its(flush_opcode_in) else in_in.data(DATA_WIDTH_c-1+16 downto 16);

  -- CIC Decimation primitives (I & Q)
  prim_rst <= ctl_in.reset or drop_opcode_in;
  
  Dec_I : entity work.cic_dec_gen2
    generic map(
      N          => N_c,
      M          => M_c,
      R          => R_c,
      DIN_WIDTH  => DATA_WIDTH_c,
      ACC_WIDTH  => ACC_WIDTH_c,
      DOUT_WIDTH => DATA_WIDTH_c
      )
    port map(
      CLK      => ctl_in.clk,
      RST      => prim_rst,
      DIN      => i_in,
      DIN_VLD  => din_vld,
      DIN_RDY  => din_rdy,
      DOUT     => i_out,
      DOUT_VLD => dout_vld,
      DOUT_RDY => out_in.ready
      );
  
  Dec_Q : entity work.cic_dec_gen2
    generic map(
      N          => N_c,
      M          => M_c,
      R          => R_c,
      DIN_WIDTH  => DATA_WIDTH_c,
      ACC_WIDTH  => ACC_WIDTH_c,
      DOUT_WIDTH => DATA_WIDTH_c
      )
    port map(
      CLK      => ctl_in.clk,
      RST      => prim_rst,
      DIN      => q_in,
      DIN_VLD  => din_vld,
      DIN_RDY  => open,
      DOUT     => q_out,
      DOUT_VLD => open,
      DOUT_RDY => out_in.ready
      );

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
    IN_READY        => in_out_take,
    IN_SOM          => in_in.som,
    IN_EOM          => in_in.eom,
    IN_OPCODE       => in_opcode_slv,
    IN_VALID        => in_in.valid,
    IN_BYTE_ENABLE  => in_in.byte_enable,
    IN_DATA         => in_in.data,
    OUT_READY       => out_in.ready,
    OUT_SOM         => open,
    OUT_EOM         => delayline_eom,
    OUT_OPCODE      => out_opcode_slv,
    OUT_VALID       => delayline_valid,
    OUT_BYTE_ENABLE => out_out.byte_enable,
    OUT_DATA        => delayline_data);

  --EOF should be incorporated into the delayline primitive
  eofDelayLine : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        out_out.eof <= '0';
      elsif its(out_in.ready) and its(group_delay_zero) then
        out_out.eof <= in_in.eof;
      end if;
    end if;
  end process;

  out_opcode_int <= to_integer(unsigned(out_opcode_slv));
  out_out.opcode <=
    ComplexShortWithMetadata_samples_op_e  when out_opcode_int = 0 or its(flush_opcode_in) else
    ComplexShortWithMetadata_time_op_e     when out_opcode_int = 1 else
    ComplexShortWithMetadata_interval_op_e when out_opcode_int = 2 else
    ComplexShortWithMetadata_flush_op_e    when out_opcode_int = 3 else
    ComplexShortWithMetadata_sync_op_e     when out_opcode_int = 4;

  --Flushing logic:
  --One output is produced for every R inputs
  --The first valid sample appears after GROUP_DELAY_c or (N+1)*R inputs
  --rcnt tracks the number of samples before the next output
  --To flush, we need to load GROUP_DELAY_c - rcnt samples into the primitive
  groupDelayCounterProc : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(prim_rst) then
        group_delay_counter <= (others => '0');
        rcnt <= (others => '0');
      --Sample is being loaded into the primitive
      elsif its(din_rdy) and its(din_vld) then
        --When flushing
        if its(flush_opcode_in) then
          --Count down to GROUP_DELAY_c - rcnt
          if group_delay_counter > 0 then
            group_delay_counter <= group_delay_counter - 1;
          end if;
        else --samples data
          --Increment group delay counter when taking samples data
          if group_delay_counter < GROUP_DELAY_c then
            group_delay_counter <= group_delay_counter + 1;
          end if;
          --Increment rcnt until rcnt = R
          if rcnt < R then
            rcnt <= rcnt + 1;
          end if;
        end if;
      --Sample is being produced by the primitive  
      elsif its(out_in.ready) and its(dout_vld) then --Note: dout_rdy = out_in.ready
        --Reset rcnt when output is ready and rcnt = R
        if rcnt = R then
          rcnt <= (others => '0');
        end if;
        --When flushing
        if its(flush_opcode_in) then
          --If rcnt is 0 during flush, we only need to count down to R before
          --resetting counters
          if rcnt = 0 and group_delay_counter = R_c then
            group_delay_counter <= (others => '0');
            rcnt                <= (others => '0');  
          --Reset counters when we have loaded GROUP_DELAY_c - rcnt samples into the primitive
          elsif its(group_delay_rcnt) then
            group_delay_counter <= (others => '0');
            rcnt                <= (others => '0');
          end if;
        end if;
      end if;
    end if;
  end process;
  flush_eom <= flush_opcode_in and (group_delay_rcnt or (to_bool(rcnt = 0) and to_bool(group_delay_counter = R_c)));
               
  group_delay_max <= to_bool(group_delay_counter >= GROUP_DELAY_c);
  group_delay_zero <= to_bool(group_delay_counter = 0);
  group_delay_rcnt <= to_bool(group_delay_counter = rcnt);
end rtl;
