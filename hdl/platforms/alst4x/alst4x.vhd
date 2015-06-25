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
library platform; use platform.platform_pkg.all;
library bsv;
library alst4x;
architecture rtl of alst4x_worker is
  signal ctl_clk                : std_logic;        -- clock we produce and use for the control plane
  signal ctl_rst_n              : std_logic;        -- reset associated with control plane clock
  signal pci_id                 : std_logic_vector(15 downto 0);
  -- unoc internal connections
  signal pci2unoc, unoc2cp : unoc_master_out_t;
  signal unoc2pci, cp2unoc : unoc_master_in_t;
  signal unoc_out_data     : std_logic_vector (152 downto 0);
  component pci_alst4 is
  port(
    sys0_clk                : in  std_logic;
    sys0_rstn               : in  std_logic;
    pcie_clk                : in  std_logic;
    pcie_rstn               : in  std_logic;
    pcie_rx                 : in  std_logic_vector (3 downto 0);
    pcie_tx                 : out std_logic_vector (3 downto 0);
    pci_blink               : out std_logic;
    pci_link_up             : out std_logic;
    -- PCI signals facing into the rest of the platform
    p125clk                 : out std_logic;
    p125rstn                : out std_logic;
    pci_device              : out std_logic_vector (15 downto 0);
    -- unoc_link from PCI
    unoc_out_data           : out std_logic_vector (152 downto 0);
    unoc_out_valid          : out std_logic;
    unoc_out_take           : out std_logic;
    -- unoc_link to PCI
    unoc_in_data            : in  std_logic_vector (152 downto 0);
    unoc_in_valid           : in  std_logic;
    unoc_in_take            : in  std_logic);
  end component pci_alst4;
begin
  timebase_out.clk   <= sys0_clk;
  timebase_out.reset <= not sys0_rstn;
  timebase_out.ppsIn <= ppsExtIn;
  ppsOut             <= timebase_in.ppsOut;
  -- Instantiate the PCI core, which will also provide back to us a 125MHz clock
  -- based on the incoming 250Mhz PCI clock (based on the backplane 100Mhz PCI clock).
  -- We will use that 125MHz clock as our control plane clock since that avoids
  -- clock-domain crossing for lots of logic (control plane and data plane)

  pcie : pci_alst4
    port map(sys0_clk       => sys0_clk,
             sys0_rstn      => sys0_rstn,
             pcie_clk       => pcie_clk,
             pcie_rstn      => pcie_rstn,
             pcie_rx        => pcie_rx,
             pcie_tx        => pcie_tx,
             pci_blink      => led(7),
             pci_link_up    => led(0),
             -- PCI signals facing into the rest of the platform
             p125clk        => ctl_clk,  -- we use this output for our control clock
             p125rstn       => ctl_rst_n,
             pci_device     => pci_id,
             -- unoc links
             unoc_out_data  => unoc_out_data,
             unoc_out_valid => pci2unoc.valid,
             unoc_out_take  => pci2unoc.take,
             unoc_in_data   => to_slv(unoc2pci.data),
             unoc_in_valid  => unoc2pci.valid,
             unoc_in_take   => unoc2pci.take);
  
  -- Complete the master unoc record
  pci2unoc.clk     <= ctl_clk;
  pci2unoc.reset_n <= ctl_rst_n;
  pci2unoc.id      <= pci_id;
  pci2unoc.data    <= to_unoc(unoc_out_data);

  cp_unoc : platform.unoc_node_defs.unoc_node_rv
    generic map(control    => btrue)
    port    map(up_in      => pci2unoc,
                up_out     => unoc2pci,
                client_in  => cp2unoc,
                client_out => unoc2cp,
                down_in    => pcie_in,
                down_out   => pcie_out);

  term_unoc : unoc_terminator
    port    map(up_in      => pcie_slave_in,
                up_out     => pcie_slave_out,
                drop_count => props_out.unocDropCount);


  -- Here we need to adapt the unoc protocol to the occp protocol

  cp_adapt : unoc_cp_adapter
    port    map(client_in  => unoc2cp,
                client_out => cp2unoc,
                cp_in    => cp_in,
                cp_out   => cp_out);

  -- Output/readable properties
  props_out.platform        <= to_string("alst4x", props_out.platform'length-1);
  props_out.dna             <= (others => '0');
  props_out.nSwitches       <= (others => '0');
  props_out.switches        <= (others => '0');
  props_out.memories_length <= to_ulong(1);
  props_out.memories        <= (others => to_ulong(0));
  props_out.nLEDs           <= to_ulong(led'length);      -- not including the gmii led
  props_out.UUID            <= metadata_in.UUID;
  props_out.romData         <= metadata_in.romData;
  props_out.pciId          <= ushort_t(unsigned(pci_id));
  -- Settable properties - drive the leds that are not driven by hardware from the property
  led(6 downto 1)           <= std_logic_vector(props_in.leds(6 downto 1));
  led(led'left downto 8)    <= (others => '0');
  -- Drive metadata interface
  metadata_out.clk          <= ctl_clk;
  metadata_out.romAddr      <= props_in.romAddr;
  metadata_out.romEn        <= props_in.romData_read;
end rtl;
