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
library ml605;
architecture rtl of ml605_worker is
  signal ctl_clk                : std_logic;        -- clock we produce and use for the control plane
  signal ctl_rst_n              : std_logic;        -- reset associated with control plane clock
  signal ctl_rst                : std_logic;
  signal sys0_clkunbuf          : std_logic;        -- unbuffered 200mhz clock
  signal sys0_clk               : std_logic;        -- timebase clock we produce and use
  signal sys0_rst               : std_logic;        -- reset for timebase
  signal sys1_clk_in            : std_logic;        -- received sys1 clk before bufg
  signal sys1_clk               : std_logic;        -- GBE clock we have produced for GMII
  signal sys1_rst_n             : std_logic;        -- reset for GBE clock
  signal pci_id                 : std_logic_vector(15 downto 0);
  -- unoc internal connections
  signal unoc_out_data          : std_logic_vector(unoc_data_width-1 downto 0);
-- chipscope
  --signal control0               : std_logic_vector(35 downto 0);
  --signal ila_data               : std_logic_vector(31 downto 0);
  --signal ila_trigger            : std_logic_vector(7 downto 0);
begin
  ctl_rst            <= not ctl_rst_n;
  timebase_out.clk   <= sys0_clk;
  timebase_out.reset <= sys0_rst;
  timebase_out.ppsIn <= ppsExtIn;
  ppsOut             <= timebase_in.ppsOut;
  -- Provide the highest available quality clock and reset used for the time server,
  -- without regard for it being in any particular time domain.

  -- Receive the differential clock, then put it on a clock buffer
  sys0_ibufds : IBUFDS port map(I  => sys0_clkp,
                                IB => sys0_clkn,
                                O  => sys0_clkunbuf);
  
  sys0_bufg   : BUFG   port map(I => sys0_clkunbuf,
                                O => sys0_clk);

  -- Create a reset sync'd with sys0_clk, based on our control reset
  sys0_sr : bsv.bsv.SyncResetA
    generic map(RSTDELAY => 0)
    port    map(CLK      => sys0_clk,
                IN_RST   => ctl_rst,
                OUT_RST  => sys0_rst);

  
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

  pcie : work.ml605_pkg.pci_ml605
    port map(pci0_clkp      => pci0_clkp,
             pci0_clkn      => pci0_clkn,
             pci0_rstn      => pci0_reset_n,
             pcie_rxp_i     => pcie_rxp,
             pcie_rxn_i     => pcie_rxn,
             pcie_txp       => pcie_txp,
             pcie_txn       => pcie_txn,
             pci_blink      => led(7),
             pci_link_up    => led(0),
             -- PCI signals facing into the rest of the platform
             p125clk        => ctl_clk,  -- we use this output for our control clock
             p125rstn       => ctl_rst_n,
             pci_device     => pci_id,
             -- unoc links
             unoc_out_data  => unoc_out_data,
             unoc_out_valid => pcie_out.valid,
             unoc_out_take  => pcie_out.take,
             unoc_in_data   => to_slv(pcie_in.data),
             unoc_in_valid  => pcie_in.valid,
             unoc_in_take   => pcie_in.take);
  
  -- Complete the master unoc record
  pcie_out.data    <= to_unoc(unoc_out_data);
  pcie_out.clk     <= ctl_clk;
  pcie_out.reset_n <= ctl_rst_n;
  pcie_out.id      <= pci_id;

  -- Output/readable properties
  props_out.dna             <= (others => '0');
  props_out.nSwitches       <= (others => '0');
  props_out.switches        <= (others => '0');
  props_out.memories_length <= to_ulong(1);
  props_out.memories        <= (others => to_ulong(0));
  props_out.nLEDs           <= to_ulong(led'length);      -- not including the gmii led
  props_out.UUID            <= metadata_in.UUID;
  props_out.romData         <= metadata_in.romData;
  props_out.pciId           <= ushort_t(unsigned(pci_id));
  props_out.unocDropCount   <= pcie_in.dropCount;
  props_out.slotCardIsPresent <= (0 => not fmc_lpc_prsnt_m2c_l,
                                  1 => not fmc_hpc_prsnt_m2c_l,
                                  others => '0'); -- FIXME others not necessary when sequence
  -- Settable properties - drive the leds that are not driven by hardware from the property
  led(6 downto 1)           <= std_logic_vector(props_in.leds(6 downto 1));
  led(12 downto 8)          <= std_logic_vector(props_in.leds(12 downto 8));
  -- Drive metadata interface
  metadata_out.clk          <= ctl_clk;
  metadata_out.romAddr      <= props_in.romAddr;
  metadata_out.romEn        <= props_in.romData_read;
------------------------------------------------------------------------------------------
-- Chipscope
------------------------------------------------------------------------------------------
--  icon_i : ml605.chipscope_pkg.chipscope_icon
--    port map(control0 => control0);
--  ila_i  : ml605.chipscope_pkg.chipscope_ila
--    port map(control => control0,
--             clk     => ctl_clk,
--             data    => ila_data,
--             trig0   => ila_trigger);

--  ila_trigger(0) <= pci2unoc.valid;
--  ila_trigger(1) <= pci2unoc.take;
--  ila_trigger(2) <= unoc2pci.valid;
--  ila_trigger(3) <= unoc2pci.take;

--  ila_trigger(4) <= unoc2cp.valid;
--  ila_trigger(5) <= unoc2cp.take;
--  ila_trigger(6) <= cp2unoc.valid;
--  ila_trigger(7) <= cp2unoc.take;

--  ila_data(0) <= pci2unoc.valid;
--  ila_data(1) <= pci2unoc.take;
--  ila_data(2) <= unoc2pci.valid;
--  ila_data(3) <= unoc2pci.take;

--  ila_data(4) <= unoc2cp.valid;
--  ila_data(5) <= unoc2cp.take;
--  ila_data(6) <= cp2unoc.valid;
--  ila_data(7) <= cp2unoc.take;

--  ila_data(8) <= pcie_slave_in.valid;
--  ila_data(9) <= pcie_slave_in.take;
----  ila_data(10) <= pcie_slave_out.valid;
----  ila_data(11) <= pcie_slave_out.take;

--  ila_data(31 downto 12) <= pci2unoc.data(19 downto 0);     
end rtl;
