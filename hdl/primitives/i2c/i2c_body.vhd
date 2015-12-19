-- package for letting VHDL access the verilogs in this library
library ieee; use ieee.std_logic_1164.all, ieee.numeric_std.all;
package body i2c is

  function count_ones(slv : std_logic_vector) return natural is
    variable n_ones : natural := 0;
  begin
    for i in slv'range loop
      if slv(i) = '1' then
        n_ones := n_ones + 1;
      end if;
    end loop;
    return n_ones;
  end function count_ones;

end package body i2c;
