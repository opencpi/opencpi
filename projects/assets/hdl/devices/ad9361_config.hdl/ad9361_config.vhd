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

-- THIS FILE WAS ORIGINALLY GENERATED ON Thu Feb 16 16:40:46 2017 EST
-- BASED ON THE FILE: ad9361_config.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: ad9361_config

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of ad9361_config_worker is
  constant adc_sub_c  : natural := 0;
  constant dac_sub_c  : natural := 1;

  signal both_present : std_logic := '0';
begin
  -- Route the raw property signals to the ad9361_spi
  rawprops_out.present     <= '1';
  rawprops_out.reset       <= ctl_in.reset;
  rawprops_out.raw         <= props_in.raw;
  props_out.raw            <= rawprops_in.raw;
  props_out.other_present  <= rawprops_in.present(1);

  dev_force_spi_reset_out.force_reset <= props_in.force_reset;

  -- note that, just because a adc_sub or dac_sub worker is present in the
  -- bitstream, that doesn't mean it is enabled - these properties convey to
  -- the appropriate sub device which is the present data paths are enabled
  dev_cfg_data_out(adc_sub_c).config_is_two_r  <= props_in.config_is_two_r;
  dev_cfg_data_out(dac_sub_c).config_is_two_r  <= props_in.config_is_two_r;
  dev_cfg_data_tx_out.config_is_two_t          <= props_in.config_is_two_t;
  dev_cfg_data_tx_out.force_two_r_two_t_timing <= props_in.force_two_r_two_t_timing;

  -- use of logic 'or' is prevalent below because (**c_c).config and
  -- (**c_c).present signals are logic '0' when the adc or dac device workers
  -- are not included in a bitsream
  props_out.swap_ports               <= btrue     when   dev_cfg_data_port_in.p0_p1_are_swapped               = '1'  else bfalse;
  props_out.rx_frame_usage           <= toggle_e  when   dev_cfg_data_rx_in.rx_frame_usage                    = '1'  else enable_e;
  props_out.rx_frame_is_inverted     <= btrue     when   dev_cfg_data_rx_in.rx_frame_is_inverted              = '1'  else bfalse;
  props_out.qadc0_is_present         <= btrue     when   dev_cfg_data_in(adc_sub_c).ch0_handler_is_present    = '1'  else bfalse;
  props_out.qadc1_is_present         <= btrue     when   dev_cfg_data_in(adc_sub_c).ch1_handler_is_present    = '1'  else bfalse;
  props_out.qdac0_is_present         <= btrue     when   dev_cfg_data_in(dac_sub_c).ch0_handler_is_present    = '1'  else bfalse;
  props_out.qdac1_is_present         <= btrue     when   dev_cfg_data_in(dac_sub_c).ch1_handler_is_present    = '1'  else bfalse;
  props_out.data_bus_index_direction <= reverse_e when ((dev_cfg_data_in(adc_sub_c).data_bus_index_direction or
                                                         dev_cfg_data_in(dac_sub_c).data_bus_index_direction) = '1') else normal_e;
  props_out.data_clk_is_inverted     <= btrue     when ((dev_cfg_data_in(adc_sub_c).data_clk_is_inverted or
                                                         dev_cfg_data_in(dac_sub_c).data_clk_is_inverted)     = '1') else bfalse;
  props_out.LVDS                     <= bfalse    when ((dev_cfg_data_in(adc_sub_c).islvds or
                                                         dev_cfg_data_in(dac_sub_c).islvds or
                                                         dev_cfg_data_port_in.iostandard_is_lvds)             = '0') else btrue;
  props_out.single_port              <= btrue     when ((dev_cfg_data_in(adc_sub_c).isdualport or
                                                         dev_cfg_data_in(dac_sub_c).isdualport)               = '0') else bfalse;
  props_out.half_duplex              <= btrue     when ((dev_cfg_data_in(adc_sub_c).isfullduplex or
                                                         dev_cfg_data_in(dac_sub_c).isfullduplex)             = '0') else bfalse;
  props_out.data_rate_config         <=  SDR_e    when ((dev_cfg_data_in(adc_sub_c).isddr or
                                                         dev_cfg_data_in(dac_sub_c).isddr)                    = '0') else DDR_e;
  both_present <= dev_cfg_data_in(adc_sub_c).present and dev_cfg_data_in(dac_sub_c).present;
  props_out.data_configs_are_valid <= '1' when
      ((both_present = '1') and
       (dev_cfg_data_in(adc_sub_c).data_bus_index_direction = dev_cfg_data_in(dac_sub_c).data_bus_index_direction)  and
       (dev_cfg_data_in(adc_sub_c).data_clk_is_inverted     = dev_cfg_data_in(dac_sub_c).data_clk_is_inverted)  and
       (dev_cfg_data_in(adc_sub_c).islvds       = dev_cfg_data_in(dac_sub_c).islvds)  and
       ((dev_cfg_data_in(adc_sub_c).present and
        (dev_cfg_data_in(adc_sub_c).islvds      = dev_cfg_data_port_in.iostandard_is_lvds)) ) and
       ((dev_cfg_data_in(dac_sub_c).present and
        (dev_cfg_data_in(dac_sub_c).islvds      = dev_cfg_data_port_in.iostandard_is_lvds)) ) and
       (dev_cfg_data_in(adc_sub_c).isdualport   = dev_cfg_data_in(dac_sub_c).isdualport)  and
       (dev_cfg_data_in(adc_sub_c).isfullduplex = dev_cfg_data_in(dac_sub_c).isfullduplex)  and
       (dev_cfg_data_in(adc_sub_c).isddr        = dev_cfg_data_in(dac_sub_c).isddr)) or
      (both_present = '0') else '0';

end rtl;
