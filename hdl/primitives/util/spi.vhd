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

-- SPI engine.  Essentially a raw-to-spi adapter, but with our parent module already
-- dealing with converting the raw bus to the specified addr and data width.
library IEEE, ocpi;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ocpi.types.all, ocpi.util.all;
entity spi is
  generic(data_width    : positive := 8;
          addr_width    : positive := 7;
          clock_divisor : positive := 16;
          capture_fall  : boolean  := false);
  port   (clk           : in  std_logic;
          reset         : in  bool_t;
          renable       : in  bool_t;
          wenable       : in  bool_t;
          addr          : in  unsigned(addr_width-1 downto 0);
          wdata         : in  std_logic_vector(data_width-1 downto 0);
          rdata         : out std_logic_vector(data_width-1 downto 0);
          done          : out bool_t;
          -- The SPI signals - a 4 wire interface
          sdo           : in  std_logic;
          sclk          : out std_logic;
          sen           : out std_logic;
          sdio          : out std_logic);
end entity spi;
architecture rtl of spi is
  type   state_t is (idle_e, starting_e, busy_e, done_e);
  signal state_r      : state_t := idle_e;
  signal clk_count_r  : unsigned(width_for_max(clock_divisor-1)-1 downto 0) := (others => '0');
  signal bit_count_r  : unsigned(width_for_max(addr_width + data_width + 1)-1 downto 0);
begin
  done  <= to_bool(state_r = done_e);
  SEN   <= '0' when state_r = starting_e or state_r = busy_e else '1'; -- enable asserted low
  SDIO  <= '1' when wenable and state_r = busy_e and bit_count_r = 0 else
           '0' when state_r = busy_e and bit_count_r = 0 else
           -- Select the right bit from the 7 bit address
           addr(addr_width - to_integer(bit_count_r)) when bit_count_r <= addr_width else
           -- Select the right bit from the data
           wdata(data_width - to_integer(bit_count_r - addr_width));
  p : process(clk) is begin
    if rising_edge(clk) then
      if its(reset) then
        state_r <= idle_e;
        sclk    <= '0';
      else
        case state_r is
          when busy_e =>
            if clk_count_r = clock_divisor - 1 then -- end of bit cycle
              clk_count_r <= (others => '0');
              if bit_count_r = (data_width + addr_width) then -- end of access
                sclk        <= '0';
                state_r     <= done_e;
              else
                sclk        <= to_bool(capture_fall);
                bit_count_r <= bit_count_r + 1;
              end if;
            else
              clk_count_r <= clk_count_r + 1;
              if clk_count_r = clock_divisor/2 - 1 then -- mid-cycle, read data is captured
                sclk <= not to_bool(capture_fall);
                rdata(data_width - to_integer(bit_count_r - addr_width)) <= SDO;
              end if;
            end if;
          when done_e =>
            state_r <= idle_e;
          when starting_e =>
            state_r     <= busy_e;
            sclk        <= to_bool(capture_fall);
            bit_count_r <= (others => '0');
            clk_count_r <= (others => '0');
          when idle_e =>
            if renable or wenable then
              state_r <= starting_e;
            end if;
        end case;
      end if;
    end if;
  end process p;
end rtl;
