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

-- this sub-package is the OpenCPI definitions from the OCP spec
-- the OCP signal names are left in their "native" case to make it clear
-- that they are for those specific signals
library ieee; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
package ocp is
  -- Address Space Operations
  subtype  MCmd_t IS std_logic_vector(2 DOWNTO 0);
  constant MCmd_IDLE  : MCmd_t := "000";
  constant MCmd_WRITE : MCmd_t := "001";
  constant MCmd_READ  : MCmd_t := "010";
        
  -- Address Space Operation Responses
  subtype  SResp_t IS std_logic_vector(1 DOWNTO 0);
  constant SResp_NULL : SResp_t := "00";
  constant SResp_DVA  : SResp_t := "01";
  constant SResp_FAIL : SResp_t := "10";
  constant SResp_ERR  : SResp_t := "11";

end package ocp;
