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

library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
library work; use work.platform_pkg.all; use work.time_client_defs.all; 
entity time_client_rv is
  port(
    wci_Clk     : in std_logic;
    wci_Reset_n : in std_logic;
    time_in     : in  time_service_t;
    wti_in      : in  wti_in_t;
    wti_out     : out wti_out_t
    );
end entity time_client_rv;
architecture rtl of time_client_rv is

  component SyncRegister is
    generic (
      width : positive;
      init  : natural);
    port (
      sCLK   : in  std_logic;
      dCLK   : in  std_logic;
      sRST   : in  std_logic;
      sD_IN  : in  std_logic_vector(width-1 downto 0);
      sEN    : in  std_logic;
      dD_OUT : out std_logic_vector(width-1 downto 0);
      sRDY   : out std_logic);
  end component;

begin

  -- We have no support yet for the time not being valid.
  -- Thus it is assumed that the time is valid before workers using time are out of reset
  -- If the infrastructure changed to support this, we would conditionally drive MCmd.
  -- When "allow unavailable" is supported, it would mean that the system could allow workers
  -- to be enabled (out of reset, or started) even if the time was not available.

  syncReg : SyncRegister
    generic map (
      width => 64,
      init  => 0)
    port map (
      sCLK   => time_in.clk,
      dCLK   => wti_in.Clk,
      sRST   => time_in.reset,
      sD_IN  => std_logic_vector(time_in.now),
      sEN    => '1',
      dD_OUT => wti_out.MData,
      sRDY   => open);

  wti_out.MCmd <= ocp.MCmd_WRITE; -- just for OCP compliance.
end architecture rtl;
