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

-- THIS FILE WAS ORIGINALLY GENERATED ON Fri Mar  1 13:38:29 2019 EST
-- BASED ON THE FILE: ad9361_config_ts.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: ad9361_config_ts

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library bsv; use bsv.bsv.all;
architecture rtl of ad9361_config_ts_worker is
  constant adc_sub_c  : natural := 0;
  constant dac_sub_c  : natural := 1;

  signal both_present : std_logic := '0';
  signal props_in_s  : worker_props_in_t;

  -- pin control - control clock domain
  signal wci_reset_n : std_logic := '1';
  signal wci_enable  : std_logic := '0';
  signal wci_txnrx   : std_logic := '0';
  -- pin control - DAC (AD9361 FB_CLK) clock domain
  signal dac_clk    : std_logic := '0'; -- AD9361 FB_CLK equivalent
  signal dac_enable : std_logic := '0';
  signal dac_txnrx  : std_logic := '0';

  -- signals interface
  signal ENABLE_s : std_logic := '0';
  signal TXNRX_s  : std_logic := '0';
begin
  -- Route the raw property signals to the ad9361_spi
  rawprops_out.present     <= '1';
  rawprops_out.reset       <= ctl_in.reset;
  rawprops_out.raw         <= props_in_s.raw;
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

  pin_control_true : if pin_control_p = btrue generate
    props_in_s.raw <= props_in.raw;   --No mux needed for pin control mode

    ensm_control_process : process(props_in.ENSM_Pin_Control,
                                   props_in.Half_Duplex_Mode,
                                   props_in.Level_Mode,
                                   props_in.FDD_External_Control_Enable,
                                   props_in.ENABLE_force_set,
                                   props_in.TXNRX_force_set)
    begin
      if props_in.ENABLE_force_set = btrue then
        wci_enable <= '1';
      else -- props_in.ENABLE_force_set = bfalse then
        if props_in.FDD_External_Control_Enable = btrue then
          if props_in.Level_Mode = btrue then
            -- see UG-570 Figure 14
            wci_enable <= dev_rxen_data_sub_in.rxen;
          else -- props_in.Level_Mode = bfalse
            wci_enable <= '0';
          end if;
        else -- props_in.FDD_External_Control_Enable = bfalse
          -- TODO / FIXME properly support all these modes
          wci_enable <= '0';
        end if;
      end if;

      if props_in.TXNRX_force_set = btrue then
        wci_txnrx <= '1';
      else -- props_in.TXNRX_force_set = bfalse then
        if props_in.FDD_External_Control_Enable = btrue then
          if props_in.Level_Mode = btrue then
            -- see UG-570 Figure 14
            wci_txnrx <= dev_txen_data_sub_in.txen;
          else -- props_in.Level_Mode = bfalse
            -- TODO / FIXME properly support this mode
            wci_txnrx <= '0';
          end if;
        else -- props_in.FDD_External_Control_Enable = bfalse
          -- TODO / FIXME properly support all these modes
          wci_txnrx <= '0';
        end if;
      end if;

    end process ensm_control_process;

  end generate pin_control_true;

  pin_control_false : if pin_control_p = bfalse generate
    -- TODO / FIXME - support pin_control_p = false
    ctl_out.error <= '1';
    wci_enable <= '0';
    wci_txnrx  <= '0';
  end generate pin_control_false;

  -- TODO / FIXME - eventually, ENABLE/TXNRX should be driven by signals in-band with
  -- the TX data, and thus would exist in the DAC (AD9361 FBCLK) domain, which
  -- would remove the need for synchronization

  -- this is equivalent to AD9361 FB_CLK (mirrors the inversion done by
  -- ad9361_data_sub.hdl clock forwarding)
  dac_clk <= not dev_data_clk_in.DATA_CLK_P;

  wci_reset_n <= not ctl_in.reset;

  -- TODO / FIXME - for now rxen functionality is not supported, so we are assuming wci_enable is static while wci_txnrx is toggling (this is true after the AD9361 is initialized, but it is not universally true..), this seperate-synchronizers-for-ENABLE-and-TXNRX-solution is a NON-IDEAL synchronization solution for independent control signals
  sync_enable : bsv.bsv.SyncBit
    generic map(
      init   => 0)
    port map(
      sCLK   => ctl_in.clk,
      sRST   => wci_reset_n, -- apparently sRST is active-low
      dCLK   => dac_clk,
      sEN    => '1',
      sD_IN  => wci_enable,
      dD_OUT => dac_enable); -- registered by one WCI clock, two FB_CLKs
  sync_txnrx : bsv.bsv.SyncBit
    generic map(
      init   => 0)
    port map(
      sCLK   => ctl_in.clk,
      sRST   => wci_reset_n, -- apparently sRST is active-low
      dCLK   => dac_clk,
      sEN    => '1',
      sD_IN  => wci_txnrx,
      dD_OUT => dac_txnrx); -- registered by one WCI clock, two FB_CLKs

  -- until rxen functionality is added, dac_enable is expected to toggle only
  -- during AD9361 initialization
  ENABLE_s <= dac_enable;

  TXNRX_s  <= dac_txnrx;

  signals : entity work.signals
    port map(
      w_ENABLE => ENABLE_s,
      w_TXNRX  => TXNRX_s,
      ENABLE   => ENABLE,
      TXNRX    => TXNRX);

end rtl;
