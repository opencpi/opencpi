-- THIS FILE WAS ORIGINALLY GENERATED ON Wed Jun 19 12:13:07 2013 EDT
-- BASED ON THE FILE: (null)
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: ml605t

-- This is the ml605 platform worker.
-- It provides properties for the platform as a whole, and
-- it bootstraps the entire platform by providing clocks and control plane access.
-- Like all workers, it has a WCI which only works after this worker itself
-- deasserts its output reset associated with the control plane clock it produces.

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library unisim; use unisim.vcomponents.all;
library platform; use platform.platform_pkg.all;
library bsv;
architecture rtl of ml605_worker is
  signal ctl_clk                : std_logic;        -- clock we produce and use for the control plane
  signal ctl_rst_n              : std_logic;        -- reset associated with control plane clock
  signal sys0_clk               : std_logic;        -- timebase clock we produce and use
  signal sys0_rst_n             : std_logic;        -- reset for timebase
  signal sys1_clk_in            : std_logic;        -- received sys1 clk before bufg
  signal sys1_clk               : std_logic;        -- GBE clock we have produced for GMII
  signal sys1_rst_n             : std_logic;        -- reset for GBE clock
  signal EN_server_response_get : std_logic;
  signal pci_device             : std_logic_vector(15 downto 0);
  -- unoc connections
  signal pci2unoc, unoc2pci, unoc2term, term2unoc, cp2unoc, unoc2cp : unoc_link_t;
begin
  -- Provide the highest available quality clock and reset used for the time server,
  -- without regard for it being in any particular time domain.

  -- Receive the differential clock
  sys0_ibufds : IBUFDS port map(I  => sys0_clkp,
                                IB => sys0_clkn,
                                O  => sys0_clk);
  

  -- Create a reset sync'd with sys0_clk, based on our control reset
  sys0_sr : bsv.bsv.SyncResetA
    generic map(RSTDELAY => 0)
    port    map(CLK      => sys0_clk,
                IN_RST   => ctl_rst_n,
                OUT_RST  => sys0_rst_n);

  
  -- Establish the sys1 clock which drives the SGMII transceiver

  -- receive the differential clock
  sys1_ibufds : IBUFDS_GTXE1 port map (I     => sys1_clkp,
                                       IB    => sys1_clkn,
                                       CEB   => '0',
                                       O     => sys1_clk_in,
                                       ODIV2 => open);

  -- drive the clock through a clock buffer
  sys1_bufg : BUFG port map (I => sys1_clk_in,
                             O => sys1_clk);

  -- Instantiate the PCI core, which will also provide back to us a 125MHz clock
  -- based on the incoming 250Mhz PCI clock (based on the backplane 100Mhz PCI clock).
  -- We will use that 125MHz clock as our control plane clock since that avoids
  -- clock-domain crossing for lots of logic (control plane and data plane)

  -- Convert output to property type: ushort_t
  props_out.pciDevice <= ushort_t(unsigned(pci_device));
  pcie : work.ml605_pkg.pci_ml605
    port map(pci0_clkp   => pci0_clkp,
             pci0_clkn   => pci0_clkn,
             pci0_rstn   => pci0_reset_n,
             pcie_rxp_i  => pcie_rxp,
             pcie_rxn_i  => pcie_rxn,
             pcie_txp    => pcie_txp,
             pcie_txn    => pcie_txn,
             pci_blink   => led(7),
             pci_link_up => led(0),
             -- PCI signals facing into the rest of the platform
             p125clk     => ctl_clk,  -- we use this output for our control
             p125rst     => ctl_rst_n,
             pci_device  => pci_device,
             -- requests coming in from PCIE
             server_request_put => pci2unoc.data,
             EN_server_request_put => pci2unoc.valid,
             RDY_server_request_put => unoc2pci.take,
             -- Responses going to PCIE
             server_response_get => unoc2pci.data,
             EN_server_response_get => EN_server_response_get,
             RDY_server_response_get => unoc2pci.valid);
  -- This assignment basically converts from the native BSV client server to our unoc link
  pci2unoc.take <= EN_server_response_get and unoc2pci.valid;
  -- Here is a uNOC consisting of a node for the control plane and a terminator
  
  cp_unoc : unoc_node
    generic map(control => true,
                up_producer_hack => true)
    port    map(CLK        => ctl_clk,
                RST_N      => ctl_rst_n,
                up_in      => pci2unoc,
                up_out     => unoc2pci,
                client_in  => cp2unoc,
                client_out => unoc2cp,
                down_in    => term2unoc,
                down_out   => unoc2term);

  term_unoc : unoc_terminator
    port    map(CLK        => ctl_clk,
                RST_N      => ctl_rst_n,
                up_in      => unoc2term,
                up_out     => term2unoc,
                drop_count => props_out.unocDropCount);


  -- Here we need to adapt the unoc protocol to the occp protocol

  cp_adapt : unoc_cp_adapter
    port    map(CLK        => ctl_clk,
                RST_N      => ctl_rst_n,
                pciDevice  => pci_device,
                client_in  => unoc2cp,
                client_out => cp2unoc,
                occp_in    => cp_in,
                occp_out   => cp_out);

  -- This piece of generic infrastructure in is instantiated here because
  -- it localizes all these signals here in the platform worker, and thus
  -- the platform worker simply produces clock, reset, and time, all in the
  -- clock domain of the timekeepping clock.
  ts : time_server
    port map(
      CLK            => ctl_clk,
      RST_N          => ctl_rst_n,
      timeCLK        => sys0_clk,
      timeRST_N      => sys0_rst_n,

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
      ppsIn               => ppsExtIn,
      ppsOut              => ppsOut,

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
  props_out.nLEDs           <= to_ulong(led'length);      -- not including the gmii led
  props_out.UUID            <= metadata_in.UUID;
  props_out.romData         <= metadata_in.romData;
  metadata_out.romAddr      <= props_in.romAddr;
  metadata_out.romEn        <= props_in.romData_read;
  -- Settable properties - drive the leds that are not driven by hardware
  led(6 downto 1)          <= std_logic_vector(props_in.leds(6 downto 1));
  led(led'left downto 8)  <= (others => '0');
end rtl;
