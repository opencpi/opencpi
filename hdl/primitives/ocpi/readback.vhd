
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
  impure function some_true return std_logic_vector is
    variable idx :
      unsigned(width_for_max(properties'right)-1 downto 0) := (others => '0');
  begin
   for i in 0 to properties'right loop
      if (ocpi_debug or not properties(i).debug) and properties(i).readable and
          read_enables(i) then
        idx := idx or to_unsigned(i, idx'length);
      end if;
    end loop;
    return data_inputs(to_integer(idx));
  end some_true;
begin
  data_output <= some_true;
end rtl;
