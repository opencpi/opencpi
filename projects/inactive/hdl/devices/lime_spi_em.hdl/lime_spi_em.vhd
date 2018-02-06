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

-- THIS FILE WAS ORIGINALLY GENERATED ON Mon May 11 19:59:20 2015 EDT
-- BASED ON THE FILE: lime_spi_em.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: lime_spi_em
library ieee, ocpi, bsv;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ocpi.types.all;
architecture rtl of lime_spi_em_worker is
  signal spi_clk_async_r : std_logic; -- cross clock domain
  signal spi_clk_r       : std_logic; -- for edge detect
  signal spi_clk_post_r  : std_logic; -- for edge detection
  signal spi_en_async_r  : std_logic; -- cross clock domain
  signal spi_en_clk_r    : std_logic; -- cross clock domain
  signal spi_en_post_r   : std_logic; -- for edge detection
  signal wdata_r         : uchar_t;
  signal addr_r          : unsigned(6 downto 0);
  signal active_r        : bool_t;
  signal reading_r       : bool_t;
  signal spi_clk_en      : std_logic;
  signal bit_count_r     : unsigned(3 downto 0);
  signal data_r          : uchar_array_t(0 to 127);
  signal give_sig        : std_logic;
  signal num_write_ctr   : unsigned(31 downto 0);
begin
  spi_clk_en <= sclk and not sen and reset;
  sdo <=
    data_r(to_integer(addr_r))(15 - to_integer(bit_count_r)) when bit_count_r >= 8 else '0';
  cp : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        spi_clk_async_r <= '0';
        spi_clk_r       <= '0';
        spi_clk_post_r  <= '0';
        active_r        <= '0';
        reading_r       <= '0';
        bit_count_r     <= (others => '0');
        addr_r          <= (others => '0');
      else
        spi_clk_async_r <= spi_clk_en;
        spi_clk_r       <= spi_clk_async_r;
        spi_clk_post_r  <= spi_clk_r;
        if spi_clk_r = '1' and spi_clk_post_r = '0' then
          if bit_count_r = 15 then
            bit_count_r <= (others => '0');
            if not reading_r then
              data_r(to_integer(addr_r)) <= wdata_r;
            end if;
          else
            bit_count_r <= bit_count_r + 1;
          end if;
          if bit_count_r = 0 then
            reading_r <= to_bool(sdio = '0');
          elsif bit_count_r <= addr_r'length then
            addr_r(addr_r'length - to_integer(bit_count_r)) <= sdio;
          elsif not reading_r then
            wdata_r(15 - to_integer(bit_count_r)) <= sdio;
          end if;
        end if; -- rising edge of enabled spi clk
      end if; -- not reset
    end if; -- rising edge
  end process;

  props_out.numWrites <= num_write_ctr;
  write_ctr : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        num_write_ctr <= (others => '0');
      elsif its(give_sig) then
        num_write_ctr <= num_write_ctr+1;
      end if; -- not reset
    end if; -- rising edge
  end process;


  give_sig <= ctl_in.is_operating and out_in.ready and to_bool(bit_count_r = 15) and spi_clk_r and not spi_clk_post_r and not reading_r;
  out_out.give <= give_sig;
  out_out.valid <= give_sig when num_write_ctr <= props_in.maxWrites else '0';
  out_out.som <= give_sig;
  out_out.eom <= give_sig;
  out_out.data <= x"00" & '1' & std_logic_vector(addr_r) & x"00" & std_logic_vector(wdata_r);
  out_out.byte_enable <= (others => '1') when num_write_ctr <= props_in.maxWrites else (others => '0');
  out_out.opcode <= (others => '0');
end rtl;
