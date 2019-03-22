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

library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity out_port_processor is
  generic(
    FIFO_DEPTH : positive := 64;
    DATA_WIDTH : positive := 32;
    ADC_WIDTH  : positive := 12);
  port(
    wsi_clk             : in  std_logic;
    wsi_reset           : in  std_logic;
    wsi_is_operating    : in  std_logic;
    wsi_ready           : in  std_logic;
    wsi_opcode          : out std_logic_vector(2 downto 0); -- 000 : samples,
                                                            -- 001 : time,
                                                            -- 010 : interval,
                                                            -- 011 : flush,
                                                            -- 1xx : sync
    wsi_give            : out std_logic;
    wsi_som             : out std_logic;
    wsi_eom             : out std_logic;
    wsi_valid           : out std_logic;
    wsi_byte_enable     : out std_logic_vector(0 downto 0);
    wsi_data            : out std_logic_vector(DATA_WIDTH-1 downto 0);
    wsi_overrun_clear   : in  std_logic;
    wsi_overrun_sticky  : out std_logic;
    wsi_time_correction : in  signed(63 downto 0);
    wsi_nsamps_per_time : in  unsigned(31 downto 0);
    wsi_time_valid      : out std_logic;
    adc_clk             : in  std_logic;
    adc_reset           : out std_logic;
    adc_give            : in  std_logic;
    adc_data_q          : in  signed(ADC_WIDTH-1 downto 0);
    adc_data_i          : in  signed(ADC_WIDTH-1 downto 0);
    adc_time_in_valid   : in  std_logic;
    adc_time_in_sec     : in  unsigned(31 downto 0);
    adc_time_in_fract   : in  unsigned(31 downto 0));
end entity out_port_processor;
architecture rtl of out_port_processor is
  constant FIFO_WIDTH : natural := 1 + adc_time_in_sec'length +
                                   adc_time_in_fract'length + (ADC_WIDTH*2);
  ------------------------------------------------------------------------------
  -- wsi_clk (worker streaming interface clock) domain
  ------------------------------------------------------------------------------

  -- mandatory output port (v1) logic
  signal wsi_data_ready_for_out_port : std_logic := '0';
  signal wsi_meta_is_reserved        : std_logic := '0';
  signal wsi_som_s                   : std_logic := '0';
  signal wsi_eom_s                   : std_logic := '0';
  signal wsi_valid_s                 : std_logic := '0';

  signal wsi_time_in_valid        : std_logic := '0';
  signal wsi_time_in              : unsigned(adc_time_in_sec'length +
                                        adc_time_in_fract'length-1 downto 0) :=
                                        (others => '0');
  signal wsi_corrected_time       : unsigned(wsi_time_in'range) :=
                                        (others => '0');
  signal wsi_corrected_time_valid : std_logic := '0';
  signal wsi_overrun_event        : std_logic := '0';

  signal wsi_fifo_ready    : std_logic := '0';
  signal wsi_fifo_give     : std_logic := '0';
  signal wsi_fifo_valid    : std_logic := '0';
  signal wsi_fifo_som      : std_logic := '0';
  signal wsi_fifo_eom      : std_logic := '0';
  signal wsi_fifo_data     : std_logic_vector(FIFO_WIDTH-1 downto 0) :=
                                 (others => '0');
  signal wsi_data_q        : signed(adc_data_q'range) := (others => '0');
  signal wsi_data_i        : signed(adc_data_i'range) := (others => '0');
  signal wsi_data_q_ext    : signed((DATA_WIDTH/2)-1 downto 0) :=
                                 (others => '0');
  signal wsi_data_i_ext    : signed((DATA_WIDTH/2)-1 downto 0) :=
                                 (others => '0');
  -- protocol's struct argument members
  signal wsi_arg_mbr_q  : std_logic_vector(15 downto 0) := (others => '0');
  signal wsi_arg_mbr_i  : std_logic_vector(15 downto 0) := (others => '0');

  signal wsi_message_size_bytes : unsigned(wsi_nsamps_per_time'range) :=
                                      (others => '0');

  ------------------------------------------------------------------------------
  -- adc_clk domain
  ------------------------------------------------------------------------------

  signal adc_fifo_data : std_logic_vector(FIFO_WIDTH-1 downto 0);
begin

  ------------------------------------------------------------------------------
  -- wsi_clk (worker streaming interface clock) domain
  ------------------------------------------------------------------------------

  -- mandatory output port (v1) logic, (note that
  -- wsi_data_ready_for_out_port must be clock-aligned with out_out.data)
  -- (note that reserved messages will be dropped on the floor)
  wsi_give <= wsi_is_operating and wsi_ready and
              (not wsi_meta_is_reserved) and wsi_data_ready_for_out_port;
  wsi_meta_is_reserved <= (not wsi_som_s) and (not wsi_valid_s) and
                          (not wsi_eom_s);

  wsi_som   <= wsi_som_s;
  wsi_eom   <= wsi_eom_s;
  wsi_valid <= wsi_valid_s;

  wsi_byte_enable <= (others => '1');

  data_width_32 : if DATA_WIDTH = 32 generate

    out_port_fsm : entity work.out_port_data_width_32_fsm
      port map(
        clk               => wsi_clk,
        reset             => wsi_reset,
        is_operating      => wsi_is_operating,
        out_ready         => wsi_ready,
        fifo_give         => wsi_fifo_give,
        fifo_som          => wsi_fifo_som,
        fifo_eom          => wsi_fifo_eom,
        fifo_valid        => wsi_fifo_valid,
        time_valid        => wsi_corrected_time_valid,
        overrun_event     => wsi_overrun_event,
        time_sec          => std_logic_vector(wsi_corrected_time(
                                 wsi_corrected_time'left downto 32)),
        time_fract_sec    => std_logic_vector(wsi_corrected_time(31 downto 0)),
        data_q            => wsi_data_q_ext,
        data_i            => wsi_data_i_ext,
        opcode            => wsi_opcode,
        som               => wsi_som_s,
        eom               => wsi_eom_s,
        valid             => wsi_valid_s,
        data              => wsi_data,
        data_ready        => wsi_data_ready_for_out_port,
        fifo_ready        => wsi_fifo_ready);

  end generate data_width_32;

  -- sign extend each I/Q part
  wsi_data_q_ext <= (wsi_arg_mbr_q'length-1 downto ADC_WIDTH =>
                    wsi_data_q(wsi_data_q'left)) & wsi_data_q;
  wsi_data_i_ext <= (wsi_arg_mbr_i'length-1 downto ADC_WIDTH =>
                    wsi_data_i(wsi_data_i'left)) & wsi_data_i;

  wsi_corrected_time_and_valid_gen : entity work.time_corrector
    generic map(
      TIME_WIDTH => wsi_time_in'length)
    port map(
      time_in              => wsi_time_in,
      time_in_valid        => wsi_time_in_valid,
      time_correction      => wsi_time_correction,
      corrected_time       => wsi_corrected_time,
      corrected_time_valid => wsi_corrected_time_valid);

  wsi_time_valid <= wsi_corrected_time_valid;

  -- unpack data from cdc_fifo output
  wsi_time_in_valid <= wsi_fifo_data(wsi_fifo_data'left);
  wsi_time_in       <= unsigned(wsi_fifo_data((ADC_WIDTH*2)+wsi_time_in'length-1
                                downto (ADC_WIDTH*2)));
  wsi_data_q        <= signed(wsi_fifo_data((ADC_WIDTH*2)-1 downto ADC_WIDTH));
  wsi_data_i        <= signed(wsi_fifo_data(ADC_WIDTH-1 downto 0));

  wsi_message_size_bytes <= wsi_nsamps_per_time sll 2;

  cdc_fifo : entity work.adc_ts_fifo
    generic map(
      width => FIFO_WIDTH,
      depth => FIFO_DEPTH)
    port map(
      clk           => wsi_clk,
      reset         => wsi_reset,
      operating     => wsi_is_operating,
      wsi_ready     => wsi_fifo_ready,
      wsi_give      => wsi_fifo_give,
      wsi_valid     => wsi_fifo_valid,
      wsi_som       => wsi_fifo_som,
      wsi_eom       => wsi_fifo_eom,
      wsi_data      => wsi_fifo_data,
      wsi_overrun_event => wsi_overrun_event,
      clear         => wsi_overrun_clear,
      overrun       => wsi_overrun_sticky,
      messageSize   => wsi_message_size_bytes,

  ------------------------------------------------------------------------------
  -- adc_clk domain
  ------------------------------------------------------------------------------

      adc_clk       => adc_clk,
      adc_reset     => adc_reset,
      adc_give      => adc_give,
      adc_data      => adc_fifo_data);

  -- pack data for cdc_fifo input
  adc_fifo_data <= adc_time_in_valid & std_logic_vector(adc_time_in_sec) &
                   std_logic_vector(adc_time_in_fract) &
                   std_logic_vector(adc_data_q) & std_logic_vector(adc_data_i);

end rtl;
