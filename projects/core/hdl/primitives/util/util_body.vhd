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

library ieee; use IEEE.std_logic_1164.all, ieee.numeric_std.all;
library ocpi; use ocpi.types.all;

package body util is
-- A convenience function to join a CWD to a filename, returning a string
-- suitable for use in VHDL file_open
function cwd_join(cwd : string_t; name : string_t) return string is
  variable length : natural := name'right;
  variable n,i    : natural;
  variable result : string(1 to length);
begin
  report "CWD Join of cwd: '" & from_string(cwd) & "' name: '" & from_string(name) & "'";
  n := 1;
  if to_character(name(0)) /= '/' and cwd(0) /= 0 then
    -- if not absolute, copy cwd to the front of the result. we know it will fit
    while n <= cwd'length and cwd(n-1) /= 0 loop
      result(n) := to_character(cwd(n-1));
      n := n + 1;
    end loop;
    if n >= length then
      report "CWD is too long for pathname: " & result severity failure;
    end if;
    result(n) := '/';
    n := n + 1;
  end if;
  i := 0;
  while name(i) /= 0 loop
    if n > length then
      report "CWD is too long for pathname: " & result severity failure;
    end if;
    result(n) := to_character(name(i));
    i := i + 1;
    n := n + 1;
  end loop;
  if n <= length then
    result(n) := NUL;
  end if;
  return result;
end cwd_join;

procedure open_file(file thefile : char_file_t;
                         cwd     : string_t;
                         name    : string_t;
                         mode    : file_open_kind) is
  variable file_name : string(1 to name'right);
  variable status    : file_open_status;
  variable msg       : string(1 to 6);
begin
  if mode = write_mode then
    msg := "output";
  else
    msg := "input ";
  end if;
  file_name := cwd_join(cwd, name);
  file_open(status, thefile, file_name, mode);

  if status = open_ok then 
    -- If we get here far, file was successfully opened
    report "File opened successfully for " & msg & ": " & file_name;
  else
    -- Fail if the status indicates file read failure
    report "File could not be opened for " & msg & ": " & file_name severity failure;
  end if;

end open_file;

procedure close_file(file thefile : char_file_t; name : string_t) is
begin
  file_close(thefile);
  report "file closed: " & from_string(name);
end close_file;
end package body util;
