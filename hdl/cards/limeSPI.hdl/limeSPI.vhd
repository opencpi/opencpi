-- THIS FILE WAS ORIGINALLY GENERATED ON Sun Dec  8 17:47:10 2013 EST
-- BASED ON THE FILE: limeSPI.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: limeSPI

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of limeSPI_worker is
  signal rx_has_it     : std_logic; -- a register saying which user has it
  signal busy          : std_logic; -- a register saying whether we are performing an access
  signal my_read_data  : std_logic_vector(7 downto 0); -- a register accumulating read data
  -- Combinatorial conveniences.
  signal my_write_data : std_logic_vector(31 downto 0);
  signal my_addr       : std_logic_vector(6 downto 0);
  signal do_read       : std_logic;
  signal do_write      : std_logic;
  -- we'll divide the control clock by 16 to be sure it is slow enough
  signal clk_count     : unsigned(3 downto 0);
  -- the access cycle has 16 states
  signal bit_count     : unsigned(4 downto 0);
begin
  -- Assert chip reset if system is reset or both clients are reset
  RESET         <= not wci_Reset_n or
                   ((not rx_present or rx_reset) and (not tx_present or tx_reset));
  -- Mux the inputs from users
  my_addr       <= rx_addr when its(rx_has_it) else tx_addr;
  my_write_data <= rx_wdata when its(rx_has_it) else tx_wdata;
  -- Drive the accumulated read data to users
  rdata         <= my_read_data;
  do_read       <= rx_read when its(rx_has_it) else tx_read;
  do_write      <= rx_write when its(rx_has_it) else tx_write;
  SEN           <= not busy;
  SCLK          <= clk_count(3);
  SDIO          <= do_write when bit_count = 0 else
                   -- Select the right bit from the 7 bit address
                   my_addr(7 - to_integer(bit_count)) when bit_count <= 7 else
                   -- Select the right bit from the 32 bit data (little endian)
                   my_write_data(to_integer(unsigned(my_addr(1 downto 0))) * 8 - to_integer(bit_count - 8));
  rx_done       <= '1' when rx_has_it = '1' and bit_count = 15 else '0';
  tx_done       <= '1' when rx_has_it = '0' and bit_count = 15 else '0';

  arb : process(wci_Clk) is
  begin
    if rising_edge(wci_Clk) then
      if wci_Reset_n = '0' then
        rx_has_it <= rx_present;
        busy      <= '0';
      elsif busy = '1' then -- We doing an access
        clk_count <= clk_count + 1;
        if clk_count = 15 then
          bit_count <= bit_count + 1;
          if bit_count > 8 then
            my_read_data(16 - to_integer(bit_count)) <= SDO;
          end if;
          if bit_count = 16 then
            busy <= '0';
          end if;
        end if;
      elsif do_read or do_write then -- starting an access
        busy      <= '1';
        bit_count <= (others => '0');
        clk_count <= (others => '0');
      elsif rx_present and tx_present then -- just switching who goes next
        rx_has_it <= not rx_has_it;
      end if;
    end if;
  end process arb;
end rtl;
