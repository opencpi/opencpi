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

library ieee; use ieee.std_logic_1164.all, ieee.numeric_std.all;
library ocpi; use ocpi.types.all;
package util is
function width_for_max(n : natural) return natural;
function roundup_2_power_of_2(n : natural) return natural;
function slv(b : std_logic) return std_logic_vector;
function slv(v : std_logic_vector) return std_logic_vector;
function slv(v : unsigned) return std_logic_vector;
function slv(v : signed) return std_logic_vector;
function slv(s : string_t) return std_logic_vector;
function slv(v : std_logic_vector; len : natural) return std_logic_vector;
function slv0(n : natural) return std_logic_vector;
function slv1(n : natural) return std_logic_vector;
function slvn(n, width : natural) return std_logic_vector;
function bit2unsigned(b : std_logic; len : natural := 1) return unsigned;
function swap(d : dword_t) return dword_t;
function min(l,r: unsigned) return unsigned;
function max(l,r: unsigned) return unsigned;
function min(l,r: natural) return natural;
function max(l,r: natural) return natural;
function min(l: unsigned; r: natural) return unsigned;
function max(l: unsigned; r: natural) return unsigned;
function count_ones(slv : std_logic_vector) return natural;
component message_bounds
  generic(width     : natural);
  port(Clk          : in  std_logic;
       RST          : in  bool_t;
       -- input side interface
       som_in       : in  bool_t;
       valid_in     : in  bool_t;
       eom_in       : in  bool_t;
       take_in      : out bool_t; -- input side is taking the input from the input port
       -- core facing signals
       core_out     : in  bool_t; -- output side has a valid value
       core_ready   : in  bool_t;
       core_enable  : out bool_t;
       -- output side interface
       ready_out    : in  bool_t; -- output side may produce
       som_out      : out bool_t;
       eom_out      : out bool_t;
       valid_out    : out bool_t;
       give_out     : out bool_t);
end component message_bounds;

component clock_limiter
  generic(width : natural);
  port(clk      : in std_logic;
       rst      : in bool_t;
       factor   : in unsigned(width-1 downto 0);
       enabled  : in bool_t;
       ready    : out bool_t);
end component clock_limiter;

end package util;
