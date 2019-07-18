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
-- Simulation (Non-Synthesizable) Data Source
-------------------------------------------------------------------------------
--
-- File: sim_source.vhd
--
-- Description:
--
-- Implements a file I/O based signal generator for use with a behavioral
-- testbench. Output generated obeys an AXI-streaming interface protocol.
--
-------------------------------------------------------------------------------
-- Revision Log:
-------------------------------------------------------------------------------
-- 02/26/18:
-- File Created
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.textio.all;
use ieee.std_logic_textio.all;

entity sim_source is
  generic (
    FILE_NAME  : in string;
    DATA_WIDTH :    positive
    );
  port (
    CLK        : in  std_logic;
    RST        : in  std_logic;
    EN         : in  std_logic;
    DOUT_TDATA : out std_logic_vector(DATA_WIDTH-1 downto 0);
    DOUT_TRDY  : in  std_logic
    );
end entity sim_source;

architecture behavior of sim_source is

  signal get_new_data : std_logic;

begin

  get_new_data <= '1' when (EN = '1') and (DOUT_TRDY = '1') else '0';

  dproc : process

    file data_file     : text;
    variable data_line : line;
    variable data      : std_logic_vector(DATA_WIDTH-1 downto 0);

  begin

    wait until RST = '1';
    DOUT_TDATA <= (others => '0');
    wait until RST = '0';

    file_open(data_file, FILE_NAME, read_mode);
    file_loop : while not endfile(data_file) loop
      readline(data_file, data_line);
      hread(data_line, data);
      DOUT_TDATA <= data;
      wait until rising_edge(CLK) and (get_new_data = '1');
    end loop;
    file_close(data_file);

  end process;

end;
