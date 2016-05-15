-- THIS FILE WAS ORIGINALLY GENERATED ON Thu May 15 22:31:46 2014 EDT
-- BASED ON THE FILE: ptest.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: ptest

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library util;              
architecture rtl of ptest_worker is
  signal dout : std_logic_vector(31 downto 0);
begin
  props_out.romData <= unsigned(dout);
  props_out.ledv <= orange_e;
  rom : component  util.util.ROM
    generic map(INITFILE => "../metadatarom.dat",
                WIDTH    => 32,
                SIZE     => 1024)
    port map(CLK  => ctl_in.clk,
             ADDR => std_logic_vector(props_in.romAddr(9 downto 0)),
             DO   => dout);
end rtl;
