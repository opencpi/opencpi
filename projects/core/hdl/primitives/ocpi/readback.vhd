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
