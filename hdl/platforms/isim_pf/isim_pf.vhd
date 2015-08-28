-- THIS FILE WAS ORIGINALLY GENERATED ON Thu Sep 26 15:32:12 2013 EDT
-- BASED ON THE FILE: isim_pf.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: isim_pf

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library platform; use platform.platform_pkg.all;
library isim_pf;
architecture rtl of isim_pf_worker is
  signal   ctl_clk          : std_logic;
  signal   ctl_reset        : std_logic;
  signal   ctl_rst_n        : std_logic;
begin
  ctl_rst_n <= not ctl_reset; -- for those that need it
  timebase_out.clk   <= ctl_clk;
  timebase_out.reset <= ctl_reset;
  timebase_out.ppsIn <= '0';

  -- generate a clock
  clock : sim_clk
    port map(clk => ctl_clk, reset => ctl_reset);

  -- The control plan connection for simulators
  dcp : sim_dcp
    port map(
      clk                     => ctl_clk,
      reset                   => ctl_reset,
      cp_in                   => cp_in,
      cp_out                  => cp_out
      );
    
  props_out.platform          <= to_string("isim_pf", props_out.platform'length-1);
  props_out.dna               <= (others => '0');
  props_out.nSwitches         <= (others => '0');
  props_out.switches          <= (others => '0');
  props_out.memories_length   <= to_ulong(1);
  props_out.memories          <= (others => to_ulong(0));
  props_out.nLEDs             <= (others => '0');
  props_out.UUID              <= metadata_in.UUID;
  props_out.romData           <= metadata_in.romData;
  metadata_out.clk            <= ctl_clk;
  metadata_out.romAddr        <= props_in.romAddr;
  metadata_out.romEn          <= props_in.romData_read;
end rtl;
