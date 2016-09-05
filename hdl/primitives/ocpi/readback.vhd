
--
-- The readback multiplexer for those properties that have readback
--
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all, ocpi.wci.all, ocpi.util.all;
entity readback is
  generic (properties : properties_t;
           ocpi_debug : bool_t);
  port (
      read_enables : in bool_array_t(properties'range);
      data_inputs  : in data_a_t(properties'range);
      data_output  : out std_logic_vector(31 downto 0)
      );
end entity readback;

architecture rtl of readback is
  function some_true(re : bool_array_t(properties'range)) return natural is
    variable x : natural := 0;
  begin
   for i in 0 to properties'right loop
      if (ocpi_debug or not properties(i).debug) and properties(i).readable and re(i) then
        -- report "setting X to " & integer'image(i);
        x := i;
      end if;
    end loop;
    return x;
  end some_true;
begin
  data_output <= data_inputs(some_true(read_enables));
end rtl;
