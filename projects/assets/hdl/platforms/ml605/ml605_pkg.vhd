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

-- VHDL component declarations for ml605 modules
library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
package ml605_pkg is
component pci_ml605 is
  port(
    pci0_clkp               : in  std_logic;
    pci0_clkn               : in  std_logic;
    pci0_rstn               : in  std_logic;
    pcie_rxp_i              : in  std_logic_vector (3 downto 0);
    pcie_rxn_i              : in  std_logic_vector (3 downto 0);
    pcie_txp                : out std_logic_vector (3 downto 0);
    pcie_txn                : out std_logic_vector (3 downto 0);
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
    ---- Requests coming in from PCIE
    --server_request_put      : out std_logic_vector (152 downto 0);
    --EN_server_request_put   : out std_logic;
    --RDY_server_request_put  : in  std_logic;
    ---- Responses going to PCIE
    --server_response_get     : in  std_logic_vector (152 downto 0);
    --EN_server_response_get  : out std_logic;
    --RDY_server_response_get : in  std_logic);
end component pci_ml605;
  
end package ml605_pkg;
