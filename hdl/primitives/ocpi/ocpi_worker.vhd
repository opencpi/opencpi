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
