
--
-- The readback multiplexer for those properties that have readback
--
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all, ocpi.wci.all, ocpi.util.all;
entity readback is
  generic (properties : properties_t;
           ocpi_debug : bool_t);
  port (
      read_index  : in unsigned(width_for_max(properties'right)-1 downto 0);
      data_inputs : in data_a_t(properties'range);
      data_output : out std_logic_vector(31 downto 0)
      );
end entity readback;

architecture rtl of readback is
begin
  data_output <= data_inputs(to_integer(read_index));
end rtl;
