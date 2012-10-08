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
