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
-- Simulation (Non-Synthesizable) Signal Capture
-------------------------------------------------------------------------------
--
-- File: agc_capture.vhd
--
-- Description:
--
-- This module capture ouput data from UUT on every clock cycle and save them in a file.
--
-------------------------------------------------------------------------------
-- Revision Log:
-------------------------------------------------------------------------------
-- 03/03/16:
-- File Created
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.textio.all;
use ieee.std_logic_textio.all;

entity agc_capture is
  generic (
    FILE_NAME  : string  := "tb_out.txt";
    DATA_WIDTH : integer := 16
  );
  port (
    CLK  : in std_logic;
    RST  : in std_logic;
    DONE : in std_logic;
    DIN  : in std_logic_vector(DATA_WIDTH-1 downto 0)
  );
end agc_capture;

architecture behavior of agc_capture is
begin
  process
    file     cap_data_file : text;
    variable cap_data_line : line;
    variable cap_data      : std_logic_vector(DATA_WIDTH-1 downto 0);
  begin
    file_open(cap_data_file,FILE_NAME,write_mode);
    wait until (RST='0') and (DONE='0');
    while (DONE/='1') loop
      wait until rising_edge(CLK);
      cap_data := DIN;
      hwrite(cap_data_line,cap_data,RIGHT,DATA_WIDTH/4);
      writeline(cap_data_file,cap_data_line);
    end loop;
    file_close(cap_data_file);
    wait;
  end process;
end behavior;
