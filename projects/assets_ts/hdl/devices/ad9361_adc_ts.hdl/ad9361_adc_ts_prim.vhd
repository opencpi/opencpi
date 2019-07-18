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

entity ad9361_adc_ts_prim is
  generic(
    FIFO_DEPTH : positive := 64;
    DATA_WIDTH : positive := 32;
    ADC_WIDTH  : positive := 12);
  port(
    wsi_clk                 : in  std_logic;
    wsi_reset               : in  std_logic;
    wsi_is_operating        : in  std_logic;
    wsi_ready               : in  std_logic;
    wsi_opcode              : out std_logic_vector(2 downto 0); -- 000 : samples,
                                                              -- 001 : time,
                                                              -- 010 : interval,
                                                              -- 011 : flush,
                                                              -- 1xx : sync
    wsi_give                : out std_logic;
    wsi_som                 : out std_logic;
    wsi_eom                 : out std_logic;
    wsi_valid               : out std_logic;
    wsi_byte_enable         : out std_logic_vector(0 downto 0);
    wsi_data                : out std_logic_vector(DATA_WIDTH-1 downto 0);
    wsi_overrun_clear       : in  std_logic;
    wsi_overrun_sticky      : out std_logic;
    wsi_time_correction     : in  signed(63 downto 0);
    --wsi_nsamps_per_time_in  : in  unsigned(31 downto 0);
    --wsi_nsamps_per_time_out : out unsigned(31 downto 0);
    wsi_message_size        : in  unsigned(31 downto 0);
    wsi_time_valid          : out std_logic;
    adc_clk                 : in  std_logic;
    adc_reset               : out std_logic;
    adc_give                : in  std_logic;
    adc_data_q              : in  signed(ADC_WIDTH-1 downto 0);
    adc_data_i              : in  signed(ADC_WIDTH-1 downto 0);
    adc_time_in_valid       : in  std_logic;
    adc_time_in_sec         : in  unsigned(31 downto 0);
    adc_time_in_fract       : in  unsigned(31 downto 0);
    adc_present             : out std_logic;
    adc_time_clk            : out std_logic);
end entity ad9361_adc_ts_prim;
architecture rtl of ad9361_adc_ts_prim is
  --signal wsi_nsamps_per_time : unsigned(wsi_nsamps_per_time_in'range) :=
  --                                 (others => '0');
  signal wsi_nsamps_per_time : unsigned(wsi_message_size'range) :=
                                   (others => '0');
begin

  wsi_nsamps_per_time <= wsi_message_size srl 2;

  --------------------------------------------------------------------------------
  ---- nsamps_per_time property
  --------------------------------------------------------------------------------
  --
  --wsi_nsamps_per_time_gen : process(wsi_nsamps_per_time_in)
  --begin
  --  if wsi_nsamps_per_time_in > 0 then
  --    wsi_nsamps_per_time(wsi_nsamps_per_time'left downto
  --                        wsi_nsamps_per_time'length-2) <= (others => '0');
  --    wsi_nsamps_per_time(wsi_nsamps_per_time'length-2-1 downto 0) <=
  --        wsi_nsamps_per_time_in(wsi_nsamps_per_time_in'length-2-1 downto 0);
  --  else
  --    wsi_nsamps_per_time <= to_unsigned(1, wsi_nsamps_per_time'length);
  --  end if;
  --end process wsi_nsamps_per_time_gen;
  --wsi_nsamps_per_time_out <= wsi_nsamps_per_time;

  --wsi_opcode_en <= props_in_opcode_en(wsi_opcode_en'range);
  --props_out.opcode_en(wsi_opcode_en'range)  <= wsi_opcode_en;
  --props_out.opcode_en(props_out.opcode_en'left downto wsi_opcode_en'length) <=
  --    (others => '0');

  ------------------------------------------------------------------------------
  -- out port
  ------------------------------------------------------------------------------

  out_port : entity work.out_port_processor
    generic map(
      FIFO_DEPTH => FIFO_DEPTH,
      DATA_WIDTH => DATA_WIDTH,
      ADC_WIDTH => ADC_WIDTH)
    port map(
      wsi_clk             => wsi_clk,
      wsi_reset           => wsi_reset,
      wsi_is_operating    => wsi_is_operating,
      --wsi_opcode_en      => wsi_opcode_en,
      wsi_ready           => wsi_ready,
      wsi_opcode          => wsi_opcode,
      wsi_give            => wsi_give,
      wsi_som             => wsi_som,
      wsi_eom             => wsi_eom,
      wsi_valid           => wsi_valid,
      wsi_byte_enable     => wsi_byte_enable,
      wsi_data            => wsi_data,
      wsi_overrun_clear   => wsi_overrun_clear,
      wsi_overrun_sticky  => wsi_overrun_sticky,
      wsi_time_correction => wsi_time_correction,
      wsi_nsamps_per_time => wsi_nsamps_per_time,
      wsi_time_valid      => wsi_time_valid,
      adc_clk             => adc_clk,
      adc_reset           => adc_reset,
      adc_give            => adc_give,
      adc_data_q          => adc_data_q,
      adc_data_i          => adc_data_i,
      adc_time_in_valid   => adc_time_in_valid,
      adc_time_in_sec     => adc_time_in_sec,
      adc_time_in_fract   => adc_time_in_fract);

  ------------------------------------------------------------------------------
  -- dev_adc port
  ------------------------------------------------------------------------------

  -- ad9361_adc_sub behavior is conditional on which/how many ad9361_adc_ts.hdl
  -- workers are present in the bitstream
  adc_present <= '1';

  ------------------------------------------------------------------------------
  -- time port
  ------------------------------------------------------------------------------

  adc_time_clk <= adc_clk;

end rtl;
