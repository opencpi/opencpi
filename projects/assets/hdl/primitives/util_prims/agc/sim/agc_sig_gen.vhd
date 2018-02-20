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
-- Simulation (Non-Synthesizable) Signal Generator
-------------------------------------------------------------------------------
--
-- File: agc_sig_gen.vhd
--
-- Description:
--
-- This module gets stimulus data from a file then send them to the UUT on every clock
-- cycle until the end-of-file
--
-------------------------------------------------------------------------------
-- Revision Log:
-------------------------------------------------------------------------------
-- 03/01/16:
-- File Created
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.textio.all;
use ieee.std_logic_textio.all;

entity agc_sig_gen is
  generic (
    FILE_NAME  : string  := "tb_in.txt";
    DATA_WIDTH : integer := 16
  );
  port (
    CLK      : in  std_logic;
    RST      : in std_logic;
    GO       : in  std_logic;
    DONE     : out std_logic;
    DOUT     : out std_logic_vector(DATA_WIDTH-1 downto 0);
    DOUT_VLD : out std_logic
  );
end agc_sig_gen;

architecture behavior of agc_sig_gen is
begin
  process
    file     sig_data_file : text;
    variable sig_data_line : line;
    variable sig_data      : std_logic_vector(DATA_WIDTH-1 downto 0);
  begin
    DONE     <= '0';
    DOUT     <= (others => '0');
    DOUT_VLD <= '0';

    file_open(sig_data_file,FILE_NAME,read_mode);
    wait until (RST='0') and (GO='1');
    while not endfile(sig_data_file) loop
      wait until rising_edge(clk);
      readline(sig_data_file,sig_data_line);
      hread(sig_data_line,sig_data);
      DOUT     <= sig_data;
      DOUT_VLD <= '1';
    end loop;
    wait until rising_edge(CLK);
    DONE     <= '1';
    DOUT     <= (others => '0');
    DOUT_VLD <= '0';
    file_close(sig_data_file);
    wait;
  end process;
end behavior;
