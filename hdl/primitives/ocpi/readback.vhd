
--
-- The readback multiplexer for those properties that have readback
--
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wci.all;
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
  subtype index_t is natural range properties'range;
  function first_true(bools : bool_array_t(properties'range)) return index_t is
  begin
    for i in 0 to properties'right loop
      if (its(ocpi_debug) or not properties(i).debug) and its(bools(i)) then
        return i;
      end if;
    end loop;
    return 0;
  end first_true;
begin
  data_output <= data_inputs(first_true(read_enables));  
end rtl;
