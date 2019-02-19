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

-------------------------------------------------------------------------------
-- MFSK Mapper
-------------------------------------------------------------------------------
--
-- Description:
-- The MFSK Mapper worker inputs bits and converts them to symbols
--
-------------------------------------------------------------------------------

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all, ocpi.util.all;
use ieee.math_real.all;

architecture rtl of worker is
  constant BITS_PER_SYMBOL   : natural := natural(log2(real(to_integer(M_p))));
  constant SYMBOLS_PER_INPUT : natural := to_integer(DIN_WIDTH_p)/BITS_PER_SYMBOL;
  constant SYMBOLS_PER_BYTE  : natural := 8/BITS_PER_SYMBOL;
  constant COUNT_WIDTH       : natural := width_for_max(SYMBOLS_PER_INPUT-1);
  constant FIRST_SYMBOL      : unsigned(COUNT_WIDTH-1 downto 0)
                               := to_unsigned(SYMBOLS_PER_INPUT-1, COUNT_WIDTH);
  signal message_wcnt_r      : unsigned(FIRST_SYMBOL'range) := (others => '0');
  signal doit                : bool_t;
begin
  -- WSI input interface outputs
  doit        <= in_in.valid and out_in.ready;
  in_out.take <= to_bool(doit and message_wcnt_r = 0);

  -- This counter is not counting messages but is counting symbols
  MessageWordCount : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        message_wcnt_r <= FIRST_SYMBOL;
      elsif its(doit) then
        if message_wcnt_r = 0 then
          message_wcnt_r <= FIRST_SYMBOL;
        else
          message_wcnt_r <= message_wcnt_r-1;
        end if;
      end if;
    end if;
  end process;

  out_out.data <= std_logic_vector(props_in.symbols(to_integer(unsigned(in_in.data(to_integer(message_wcnt_r)*BITS_PER_SYMBOL+BITS_PER_SYMBOL-1 downto to_integer(message_wcnt_r)*BITS_PER_SYMBOL)))));

  -- WSI output interface outputs - suppress MSBs that are not byte-enabled
  out_out.valid         <= doit and in_in.byte_enable(to_integer(message_wcnt_r / SYMBOLS_PER_BYTE));

end rtl;
