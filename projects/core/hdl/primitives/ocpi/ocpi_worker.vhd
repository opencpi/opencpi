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

library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; use ocpi.wci.all;
package worker is
  type control_in_t is record
    clk                  : std_logic;
    reset                : bool_t;
    control_op           : control_op_t; -- what control op is in progress?
    state                : state_t; -- the current control state
    is_operating         : bool_t; -- convenience signal for "in operating state in"
    is_big_endian        : bool_t;
  end record control_in_t;
  type control_out_t is record
    done             : bool_t; -- worker indicates completion of control/config
    attention        : bool_t; -- worker indicates it needs attention
    abort_control_op : bool_t;
  end record control_out_t;
end package worker;
