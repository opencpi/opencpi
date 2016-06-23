-- THIS FILE WAS ORIGINALLY GENERATED ON Tue Jun  2 08:35:00 2015 EDT
-- BASED ON THE FILE: lime_rx.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: lime_rx

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of ad9361_rx_worker is
begin
  -- Route the raw property signals to the lime_spi
  rawprops_out.present     <= '1';
  rawprops_out.reset       <= ctl_in.reset;
  rawprops_out.raw         <= props_in.raw;
  props_out.raw            <= rawprops_in.raw;
  props_out.other_present  <= rawprops_in.present(1);
  -- Initialize the outputs to the subdevice for muxing etc.
  dev_out.enable           <= '0';
  dev_out.sync_in          <= '0';
  -- Initialize the direct output signals from this worker
  ctrl_in                  <= (others => '0');
  en_agc                   <= '0';
end rtl;