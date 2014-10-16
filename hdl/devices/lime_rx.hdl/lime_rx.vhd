-- THIS FILE WAS ORIGINALLY GENERATED ON Tue Oct 14 11:51:22 2014 EDT
-- BASED ON THE FILE: lime_rx.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: lime_rx

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of lime_rx_worker is
  signal my_rx_clk : std_logic;
  signal overrun_r : bool_t;
begin
  -- Route the raw property signals to the lime_spi
  rawprops_out.present <= '1';
  rawprops_out.reset   <= ctl_in.reset;
  rawprops_out.renable <= props_in.raw_is_read;
  rawprops_out.wenable <= props_in.raw_is_write;
  rawprops_out.addr(props_in.raw_address'left downto 0) <= props_in.raw_address;
  rawprops_out.addr(rawprops_out.addr'left downto props_in.raw_address'left+1) <= (others => '0');
  rawprops_out.benable <= props_in.raw_byte_enable;
  rawprops_out.data    <= props_in.raw_data;
  props_out.raw_data   <= rawprops_in.data;
  props_out.overrun    <= overrun_r;
  props_out.present    <= (0 => dev_in.rx_present, 1 => dev_in.tx_present, others => '0');
  ctl_out.done <= rawprops_in.done when
                  props_in.raw_is_read or props_in.raw_is_write
                  else '1';
  -- FIXME: address issue of the quality of this clock
  my_rx_clk <= dev_in.rx_clk_in when clkmode = to_uchar(0) else dev_in.tx_clk_in;
end rtl;
