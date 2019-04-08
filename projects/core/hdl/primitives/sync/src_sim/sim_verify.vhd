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
-- Simulation (Non-Synthesizable) Verification
-------------------------------------------------------------------------------
--
-- File: sim_verify.vhd
--
-- Description:
--
-- Implements a file I/O based signal verification engine for use with a
-- behavioral testbench. Input port obeys an AXI-streaming interface protocol.
-- Data is compared to a "golden" check file and results are logged.
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

entity sim_verify is

generic(
   CHK_FNAME   : string;
   ACT_FNAME   : string;
   LOG_FNAME   : string;
   TAG         : string;
   TOLERANCE   : integer;
   DATA_WIDTH  : positive);

port(
   CLK         : in std_logic;
   RST         : in std_logic;
   EN          : in std_logic;
   DONE        : out std_logic;
   DIN_TDATA   : in std_logic_vector(DATA_WIDTH-1 downto 0));

end entity sim_verify;

architecture behavior of sim_verify is

   signal new_din           : std_logic;

begin

   new_din <= '1' when (EN = '1') else '0';

   verify: process

      file     chk_file    : text;
      variable chk_line    : line;
      variable chk         : std_logic_vector(DATA_WIDTH-1 downto 0);
      file     log_file    : text;
      variable log_line    : line;
      file     act_file    : text;
      variable act_line    : line;
      variable err_cnt     : integer;
      variable dbg_line    : line;
      variable diff        : integer;

      procedure logwrite (
         str  : string) is
      begin
         write(log_line, string'(str));
         writeline(log_file, log_line);
      end procedure;

   begin

      DONE <= '0';

      wait until RST = '1';
      wait until RST = '0';

      err_cnt := 0;

      file_open(chk_file, CHK_FNAME, read_mode);
      file_open(act_file, ACT_FNAME, write_mode);
      file_open(log_file, LOG_FNAME, write_mode);

      logwrite("-------------------------------------------------------");
      logwrite("Testbench Results for : " & TAG);
      logwrite("-------------------------------------------------------");
      logwrite("Format:");
      logwrite("Expect:Actual (Time) <ERROR Notification>");
      logwrite("-------------------------------------------------------");

      file_loop: while not endfile(chk_file) loop

         -- Get next check value and wait for valid data

         readline(chk_file, chk_line);
         hread(chk_line, chk);
         wait until rising_edge(CLK) and ((new_din = '1') or (RST = '1'));
         if (RST = '1') then
            exit;
         end if;

         -- Output to actuals file

         hwrite(act_line, DIN_TDATA);
         writeline(act_file, act_line);

         -- Assemble log message

         hwrite(log_line, chk);
         write(log_line, string'(":"));
         hwrite(log_line, DIN_TDATA);
         write(log_line, string'(" ("));
         write(log_line, now, UNIT => ns);
         write(log_line, string'(")"));

         -- Verify data against check value and update log

         diff := to_integer( signed(DIN_TDATA) - signed(chk) );

         if ( abs(diff) > TOLERANCE ) then
            write(log_line, string'(" <=== ERROR"));
            report TAG & ": Data mismatch. See log at " & LOG_FNAME severity error;
            err_cnt := err_cnt + 1;
         end if;

         -- Update log

         writeline(log_file, log_line);

      end loop;

      if (RST = '0') then

         logwrite("-------------------------------------------------------");
         logwrite("Results Summary for : " & TAG);
         logwrite("-------------------------------------------------------");
         if (err_cnt = 0) then
            logwrite("TEST PASSED - No Errors Detected");
            report TAG & " TEST PASSED - No errors detected";
         else
            logwrite("TEST FAILED - Total Errors = " & integer'image(err_cnt));
            report TAG & " TEST FAILED - " & integer'image(err_cnt) & " errors detected";
         end if;
         logwrite("-------------------------------------------------------");


      end if;

      file_close(chk_file);
      file_close(act_file);
      file_close(log_file);

      wait until rising_edge(CLK);
      DONE <= '1';
      wait until rising_edge(CLK);
      DONE <= '0';

   end process;

end;
