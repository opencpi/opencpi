-- THIS FILE WAS ORIGINALLY GENERATED ON Sat Dec 28 17:15:49 2013 EST
-- BASED ON THE FILE: zed.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: zed

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library platform; use platform.platform_pkg.all;
library zed; use zed.all;
library bsv;
architecture rtl of zed_worker is
  signal ps_axi_gp_in  : zynq_pkg.axi_gp_in_t;
  signal ps_axi_gp_out : zynq_pkg.axi_gp_out_t;
  signal clk           : std_logic;
  signal raw_rst_n     : std_logic; -- FCLKRESET_Ns need synchronization
  signal rst_n         : std_logic; -- the synchronized negative reset
  signal reset         : std_logic; -- our positive reset
begin
  -- The FCLKRESET signals from the PS are documented as asynchronous with the
  -- associated FCLK for whatever reason.  Here we make a synchronized reset from it.
  sr : bsv.bsv.SyncResetA
    generic map(RSTDELAY => 17)

    port map(IN_RST  => raw_rst_n,
             CLK     => clk,
             OUT_RST => rst_n);
  reset <= not rst_n;

  -- Instantiate the processor system (i.e. the interface to it).
  ps : entity zed_ps
    port map(
      -- Connect PS-only pins to the PS  These are external signals
      ps_in.PS_SRSTB        => PS_SRSTB,
      ps_in.PS_CLK          => PS_CLK,
      ps_in.PS_PORB         => PS_PORB,
      ps_inout.DDR_WEB      => DDR_WEB,
      ps_inout.MIO          => MIO,
      ps_inout.DDR_Clk      => DDR_Clk,
      ps_inout.DDR_Clk_n    => DDR_Clk_n,
      ps_inout.DDR_CKE      => DDR_CKE,
      ps_inout.DDR_CS_n     => DDR_CS_n,
      ps_inout.DDR_RAS_n    => DDR_RAS_n,
      ps_inout.DDR_CAS_n    => DDR_CAS_n,
      ps_inout.DDR_BankAddr => DDR_BankAddr,
      ps_inout.DDR_Addr     => DDR_Addr,
      ps_inout.DDR_ODT      => DDR_ODT,
      ps_inout.DDR_DRSTB    => DDR_DRSTB,
      ps_inout.DDR_DQ       => DDR_DQ,
      ps_inout.DDR_DM       => DDR_DM,
      ps_inout.DDR_DQS      => DDR_DQS,
      ps_inout.DDR_DQS_n    => DDR_DQS_n,
      ps_inout.DDR_VRN      => DDR_VRN,
      ps_inout.DDR_VRP      => DDR_VRP,
      -- Signals from the PS used in the PL
      pl_out.FCLK           => clk,
      pl_out.FCLKRESET_N    => raw_rst_n,
      -- The axi_gp port of the PS made available here.
      axi_gp_in             => ps_axi_gp_in,
      axi_gp_out            => ps_axi_gp_out
      );
  -- Adapt the axi master from the PS to be a CP Master
  cp : entity axi2cp
    port map(
      clk     => clk,
      reset   => reset,
      axi_in  => ps_axi_gp_out,
      axi_out => ps_axi_gp_in,
      cp_in   => cp_in,
      cp_out  => cp_out
    );
  -- This piece of generic infrastructure in is instantiated here because
  -- it localizes all these signals here in the platform worker, and thus
  -- the platform worker simply produces clock, reset, and time, all in the
  -- clock domain of the timekeepping clock.
  ts : time_server
    port map(
      CLK                 => clk,
      RST_N               => rst_n,
      timeCLK             => clk,
      timeRST_N           => rst_n,

      timeControl         => props_in.timeControl,
      timeControl_written => props_in.timeControl_written,
      timeStatus          => props_out.timeStatus,
      timeNowIn           => props_in.timeNow,
      timeNow_written     => props_in.timeNow_written,
      timeNowOut          => props_out.timeNow,
      timeDeltaIn         => props_in.timeDelta,
      timeDelta_written   => props_in.timeDelta_written,
      timeDeltaOut        => props_out.timeDelta,
      ticksPerSecond      => props_out.ticksPerSecond,
      
      -- PPS interface
      ppsIn               => '0',
      ppsOut              => open,

      -- Time service output
      time_service        => time_out
      );
  -- Output/readable properties
  props_out.platform        <= to_string("ml605", props_out.platform'length);
  props_out.dna             <= (others => '0');
  props_out.nSwitches       <= (others => '0');
  props_out.switches        <= (others => '0');
  props_out.memories_length <= to_ulong(1);
  props_out.memories        <= (others => to_ulong(0));
  props_out.nLEDs           <= to_ulong(0); --led'length);
  props_out.UUID            <= metadata_in.UUID;
  props_out.romData         <= metadata_in.romData;
  -- props_out.pciId          <= ushort_t(unsigned(pci_id));
  -- Settable properties - drive the leds that are not driven by hardware from the property
  -- led(6 downto 1)           <= std_logic_vector(props_in.leds(6 downto 1));
  -- led(led'left downto 8)    <= (others => '0');
  -- Drive metadata interface
  metadata_out.clk          <= clk;
  metadata_out.romAddr      <= props_in.romAddr;
  metadata_out.romEn        <= props_in.romData_read;
end rtl;
