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

-------------------------------------------------------------------------------
-- Convergent Rounder
-------------------------------------------------------------------------------
--
-- Description: 
-- Test Bench for the Convegent Rounder.  Depending on the what the LSB and the 
-- value of the fractional part of the data in the circuit decides to either
-- round up or down.  In the test bench there are two cases that simulate the 
-- circuit rounding up and one rounding down.  
------------------------------------------------------------------------------- 


LIBRARY ieee;
USE ieee.std_logic_1164.ALL;
 

ENTITY round_convtb IS
END round_convtb;
 
ARCHITECTURE behavior OF round_convtb IS 
 
    -- Component Declaration for the Unit Under Test (UUT)
 
    COMPONENT round_conv
    PORT(
         CLK : IN  std_logic;
         RST : IN  std_logic;
         DIN : IN  std_logic_vector(7 downto 0);
         DIN_VLD : IN  std_logic;
         DOUT : OUT  std_logic_vector(6 downto 0);
         DOUT_VLD : OUT  std_logic
        );
    END COMPONENT;
    

   --Inputs
   signal CLK : std_logic := '0';
   signal RST : std_logic := '0';
   signal DIN : std_logic_vector(7 downto 0) := (others => '0');
   signal DIN_VLD : std_logic := '0';

 	--Outputs
   signal DOUT : std_logic_vector(6 downto 0);
   signal DOUT_VLD : std_logic;

   -- Clock period definitions
   constant CLK_period : time := 10 ns;
 
BEGIN
 
	-- Instantiate the Unit Under Test (UUT)
   uut: round_conv PORT MAP (
          CLK => CLK,
          RST => RST,
          DIN => DIN,
          DIN_VLD => DIN_VLD,
          DOUT => DOUT,
          DOUT_VLD => DOUT_VLD
        );

   -- Clock process definitions
   CLK_process :process
   begin
		CLK <= '0';
		wait for CLK_period/2;
		CLK <= '1';
		wait for CLK_period/2;
   end process;
 

   -- Stimulus process
   stim_proc: process
   begin		
      -- hold reset state for 100 ns.
      wait for 100 ns;	

      wait for CLK_period*10;

      -- insert stimulus here 
		DIN <= "10000011";		-- Case were the circuit will round up basd on the LSB
		DIN_VLD <= '0';
      wait for 100 ns;
		DIN <= "10000001";		-- Case were the circuit will round down
		wait;
   end process;

END;
