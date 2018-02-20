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
library platform; use platform.pci_pkg.all, platform.platform_pkg.all;
library bsv;
architecture rtl of alst4_worker is
  signal ctl_clk            : std_logic;        -- clock we produce and use for the control plane
  signal ctl_rst_n          : std_logic;        -- reset associated with control plane clock
  signal pci_id             : std_logic_vector(15 downto 0);
  signal unoc_out_data      : std_logic_vector (152 downto 0);
  signal props_in_leds_n    : std_logic_vector(led'range);
  signal led_n_7            : std_logic;
  signal led_n_0            : std_logic;
  component pci_alst4 is
  generic(VENDOR_ID, DEVICE_ID, CLASS_CODE : natural);
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
  timebase_out.reset <= not ctl_rst_n;
  timebase_out.ppsIn <= '0';

  -- Instantiate the PCI core, which will also provide back to us a 125MHz clock
  -- based on the incoming 250Mhz PCI clock (based on the backplane 100Mhz PCI clock).
  -- We will use that 125MHz clock as our control plane clock since that avoids
  -- clock-domain crossing for lots of logic (control plane and data plane)

  pcie : pci_alst4
    generic map(VENDOR_ID  => pci_vendor_id_c, -- the vendor ID for OpenCPI
                DEVICE_ID  => pci_device_id_t'pos(pci_device_id), -- the enum value from the OWD
                CLASS_CODE => pci_class_code_c) -- the pci class code for OpenCPI
    port map(sys0_clk       => sys0_clk,
             sys0_rstn      => sys0_rstn,
             pcie_clk       => pcie_clk,
             pcie_rstn      => pcie_rstn,
             pcie_rx        => pcie_rx,
             pcie_tx        => pcie_tx,
             pci_blink      => led_n_7,
             pci_link_up    => led_n_0,
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
  pcie_out.clk     <= ctl_clk;
  pcie_out.reset_n <= ctl_rst_n;
  pcie_out.id      <= pci_id;
  pcie_out.data    <= to_unoc(unoc_out_data);

  -- Output/readable properties
  props_out.dna             <= (others => '0');
  props_out.nSwitches       <= (others => '0');
  props_out.switches        <= (others => '0');
  props_out.memories_length <= to_ulong(1);
  props_out.memories        <= (others => to_ulong(0));
  props_out.nLEDs           <= to_ulong(led'length);      -- not including the gmii led
  props_out.UUID            <= metadata_in.UUID;
  props_out.romData         <= metadata_in.romData;
  props_out.pciId          <= ushort_t(unsigned(pci_id));
  -- Settable properties - drive the active-low led signals that are not driven by hardware from the property
  props_in_leds_n <= not std_logic_vector(props_in.leds(led'range));
  led(7)                    <= not led_n_7;
  led(6 downto 1)           <= props_in_leds_n(6 downto 1);
  led(0)                    <= not led_n_0;
  led(15 downto 8)          <= props_in_leds_n(15 downto 8);
  -- Drive metadata interface
  metadata_out.clk          <= ctl_clk;
  metadata_out.romAddr      <= props_in.romAddr;
  metadata_out.romEn        <= props_in.romData_read;
  -- Drive the card-present-in-slot booleans
  --props_out.slotCardIsPresent_length <= nSlots; -- TODO / FIXME comment back in once volatile sequence properties are fixed in codegen (which SHOULD result in this property being changed from an array to a sequence)
  props_out.slotCardIsPresent <= (0 => not hsmc_a_psntn, -- this coincides with index 0 of slotName property
                                  1 => not hsmc_b_psntn, -- this coincides with index 1 of slotName property
                                  others => '0'); -- TODO / FIXME remove this line once volatile sequence properties are fixed in codegen (which SHOULD result in this property being changed from an array to a sequence)
end rtl;
