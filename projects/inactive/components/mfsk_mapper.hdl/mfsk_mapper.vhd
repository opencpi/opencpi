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
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
use ieee.math_real.all;

architecture rtl of mfsk_mapper_worker is
  constant BITS_PER_SYMBOL : natural := natural(log2(real(to_integer(M_p))));
  constant SYMBOLS_PER_INPUT : natural := to_integer(DIN_WIDTH_p)/BITS_PER_SYMBOL;
  constant SYMBOLS_PER_BYTE  : natural := 8/BITS_PER_SYMBOL;
  signal message_wcnt_r    : unsigned(15 downto 0) := to_unsigned(SYMBOLS_PER_INPUT,16)-1;
  signal doit              : bool_t;
  signal output_valid      : std_logic;
  signal output_som        : std_logic;
  signal output_eom        : std_logic;
  signal invalid_som_r     : std_logic;
  signal zlm_detected      : std_logic;  
  signal data_eom          : std_logic;

begin
  doit <= ctl_in.is_operating and in_in.ready and out_in.ready;

  -- WSI input interface outputs
  in_out.take <= doit and 
                 (data_eom or                                      --Take only after processing is done
                  (not in_in.valid and (in_in.som or in_in.eom))); --Leading SOM/Trailing EOM/ZLM

  data_eom <= to_bool(message_wcnt_r=0);                           --Send eom on last symbol

  InvalidSom : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if in_in.ready and in_in.valid then
        invalid_som_r <= '0';
      else
        if in_in.ready and in_in.som then
          invalid_som_r <= '1';
        end if;
      end if;
    end if;
  end process;

  zlm_detected <= in_in.ready and (in_in.som or invalid_som_r) and in_in.eom and not in_in.valid;

  MessageWordCount : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        message_wcnt_r <= to_unsigned(SYMBOLS_PER_INPUT,16)-1;
      elsif its(doit) then
        if its(data_eom) then --reset on last symbol
          message_wcnt_r <= to_unsigned(SYMBOLS_PER_INPUT,16)-1;
        elsif message_wcnt_r=to_unsigned(SYMBOLS_PER_INPUT,16)-1 then 
          if in_in.ready and in_in.valid then--only decrement for valid data
            message_wcnt_r <= message_wcnt_r-1;
          end if;
        else
          message_wcnt_r <= message_wcnt_r-1;
        end if;
      end if;
    end if;
  end process;

  out_out.data <= std_logic_vector(props_in.symbols(to_integer(unsigned(in_in.data(to_integer(message_wcnt_r)*BITS_PER_SYMBOL+BITS_PER_SYMBOL-1 downto to_integer(message_wcnt_r)*BITS_PER_SYMBOL)))));

  -- WSI output interface outputs
  out_out.give          <= doit and (output_valid or output_som or output_eom);
  output_valid          <= in_in.valid and doit and
                           in_in.byte_enable(to_integer(message_wcnt_r / SYMBOLS_PER_BYTE));
  out_out.valid         <= output_valid;
  output_som            <= to_bool(message_wcnt_r=to_unsigned(SYMBOLS_PER_INPUT,16)-1)
                           when in_in.ready and in_in.valid else zlm_detected;
  output_eom            <= data_eom when its(output_valid) else zlm_detected;
  out_out.som           <= output_som;
  out_out.eom           <= output_eom;
  out_out.byte_enable   <= (others => '1');

end rtl;
