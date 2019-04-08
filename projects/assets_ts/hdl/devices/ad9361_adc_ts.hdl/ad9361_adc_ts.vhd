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

-- THIS FILE WAS ORIGINALLY GENERATED ON Fri Mar  1 12:32:24 2019 EST
-- BASED ON THE FILE: ad9361_adc_ts.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: ad9361_adc_ts

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of ad9361_adc_ts_worker is

  signal wsi_opcode : std_logic_vector(2 downto 0) := (others => '0');
  signal wsi_data   : std_logic_vector(out_out.data'range) := (others => '0');

begin

  prim : entity work.ad9361_adc_ts_prim
    generic map(
      FIFO_DEPTH => positive(to_integer(fifo_depth)),
      DATA_WIDTH => positive(to_integer(ODATA_WIDTH_p)),
      ADC_WIDTH => 12)
    port map(
      wsi_clk             => ctl_in.clk,
      wsi_reset           => ctl_in.reset,
      wsi_is_operating    => ctl_in.is_operating,
      wsi_ready           => out_in.ready,
      wsi_opcode          => wsi_opcode,
      wsi_give            => out_out.give,
      wsi_som             => out_out.som,
      wsi_eom             => out_out.eom,
      wsi_valid           => out_out.valid,
      wsi_byte_enable     => out_out.byte_enable,
      wsi_data            => wsi_data,
      wsi_overrun_clear   => props_in.overrun_written,
      wsi_overrun_sticky  => props_out.overrun,
      wsi_time_correction => props_in.time_correction,
      --wsi_nsamps_per_time_in  => props_in.nsamps_per_time,
      --wsi_nsamps_per_time_out => props_out.nsamps_per_time,
      wsi_message_size    => props_in.messagesize,
      wsi_time_valid      => open,
      adc_clk             => dev_adc_in.adc_clk,
      adc_reset           => open,
      adc_give            => dev_adc_in.adc_give,
      adc_data_q          => signed(dev_adc_in.adc_data_Q),
      adc_data_i          => signed(dev_adc_in.adc_data_I),
      adc_time_in_valid   => '1', -- TODO / FIXME instead drive with time_in.valid?
      adc_time_in_sec     => time_in.seconds,
      adc_time_in_fract   => time_in.fraction,
      adc_present         => dev_adc_out.present,
      adc_time_clk        => time_out.clk);

  -- this only needed to avoid build bug for xsim:
  -- ERROR: [XSIM 43-3316] Signal SIGSEGV received.
  out_out.data <= wsi_data;

  opcode_gen : process(wsi_opcode)
  begin
    case wsi_opcode is
      when "000" =>
        out_out.opcode <= ComplexShortWithMetadata_samples_op_e;
      when "001" =>
        out_out.opcode <= ComplexShortWithMetadata_time_op_e;
      when "010" =>
        out_out.opcode <= ComplexShortWithMetadata_interval_op_e;
      when "011" =>
        out_out.opcode <= ComplexShortWithMetadata_flush_op_e;
      when others =>
        out_out.opcode <= ComplexShortWithMetadata_sync_op_e;
    end case;
  end process opcode_gen;

end rtl;
