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
---------------------------------------------------------------------------------
--
-- Single Bit - Pulse (Level to Pulse) Synchronizer
--
-- Description:
--  Pulse (Level to Pulse) Synchronizer
--  Open-loop solution (i.e. without Feedback Acknowledgement)
--
--  This is a 'shadow' wrapper for the Xilinx xpm_cdc_pulse macro. Reference
--  its documentation for functionality and usage.
--
--  Detects rising edge of input upon assertion and produces a pulse on the
--  output upon deassertion of the input. 
--
-- Generics:
--  N     : Number of synchronizing FFs. Valid range 2 to 10 (default = 2)
--  OREG  : Optional output register: 0=Disable (default), 1=Enable 
--
-- Latency:
--  Output: x src_clk and x dest_clk delays
--
-- Background:
--    Xilinx XPM : xpm_cdc_pulse
--
---------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity sync_edge_pulse is
  generic (
    N    : natural   := 2;
    OREG : std_logic := '0');
  port (
    src_clk    : in  std_logic;
    src_rst    : in  std_logic;
    src_pulse  : in  std_logic;
    dest_clk   : in  std_logic;
    dest_rst   : in  std_logic;
    dest_pulse : out std_logic);
end entity sync_edge_pulse;

architecture rtl of sync_edge_pulse is
begin
  dest_pulse <= src_pulse;
end architecture rtl;
