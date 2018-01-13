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

-- A module that must be written in verilog to write files with flushing
library ieee; use ieee.std_logic_1164.all, ieee.numeric_std.all;
library ocpi; use ocpi.types.all;
entity flush_writer is
  generic(nbytes       : natural;
          namelength   : natural);
  port   (clk          : in std_logic;
          reset        : in bool_t;
          name         : in string_t(0 to namelength);
          valid        : in bool_t;
          flush        : in bool_t;
          close        : in bool_t;
          data         : in std_logic_vector(nbytes*8-1 downto 0));
end entity flush_writer;
architecture rtl of flush_writer is
  signal name_internal : std_logic_vector(0 to namelength*8-1);
  -- prepare/convert a string_t name into one for verilog file open
  function string2slv(input : string_t) return std_logic_vector is
    variable result: std_logic_vector(0 to namelength*8-1); -- no null
    variable n: natural := 0;
    variable i: natural := 0;
  begin
    -- count good chars before NULL
    while n < namelength loop
      if unsigned(input(n)) = 0 then
        exit;
      end if;
      n := n + 1;
    end loop;
    -- put nulls at left
    while i < namelength - n loop
      result(i*8 to i*8+7) := (others => '0');
      i := i + 1;
    end loop;
    -- copy content to right end
    n := 0;
    while i < namelength loop          -- <= because result has extra room for the null
      if unsigned(input(n)) = 0 then
        exit;
      end if;
      result(i*8 to i*8+7) := from_char(input(n));
      i := i + 1;
      n := n + 1;
    end loop;
    return result;
  end string2slv;
begin
  flush_writer_i: component work.util.flush_writer_internal
    generic map(nbytes => nbytes,
                namelength => namelength)
    port    map(clk => clk,
                reset => reset,
                name => name_internal,
                valid => valid,
                flush => flush,
                close => close,
                data  => data);
  name_internal <= (others => '0') when its(reset) else string2slv(name);
end rtl;
