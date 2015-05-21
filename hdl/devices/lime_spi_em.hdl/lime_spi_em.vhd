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
begin
  spi_clk_en <= sclk and not sen and reset;
  sdo <=
    data_r(to_integer(addr_r))(15 - to_integer(bit_count_r)) when bit_count_r >= 8 else '0';
  cp : process(wci_Clk)
  begin
    if rising_edge(wci_Clk) then
      if its(wci_reset) then
        spi_clk_async_r <= '0';
        spi_clk_r       <= '0';
        spi_clk_post_r  <= '0';
        active_r        <= '0';
        reading_r       <= '0';
        bit_count_r     <= (others => '0');
        addr_r          <= (others => '0');
      else
        spi_clk_async_r <= spi_clk_en;
        spi_clk_r      <= spi_clk_async_r;
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
end rtl;
