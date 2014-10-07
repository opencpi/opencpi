-- THIS FILE WAS ORIGINALLY GENERATED ON Tue Aug 12 17:24:09 2014 EDT
-- BASED ON THE FILE: lime_spi.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: lime_spi

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all, ocpi.util.all; -- remove this to avoid all ocpi name collisions
architecture rtl of lime_spi_worker is
  -- Registers/state
  signal who_has_it : std_logic;
  -- Internal signals
  signal done       : bool_t;
  signal busy       : bool_t;
  signal renable    : bool_t;
  signal wenable    : bool_t;
  signal rdata      : std_logic_vector(7 downto 0);
  signal wdata      : std_logic_vector(7 downto 0);
  signal addr       : unsigned(6 downto 0);
  -- Convenience
  signal idx        : natural range 0 to 1;
  signal lsb        : natural range 0 to 31;
begin
  -- Assert chip reset if system is reset or both clients are reset
  RESET   <= wci_Reset or
             ((not props_in(0).present or props_in(0).reset) and
              (not props_in(1).present or props_in(1).reset));
  idx     <= 0 when who_has_it = '1' else 0;
  lsb     <= 8 * to_integer(props_in(idx).addr(1 downto 0));
  -- These are temp signals because Xilinx isim can't put them as actuals
  wdata   <= props_in(idx).data(lsb + 7 downto lsb);
  addr    <= props_in(idx).addr(6 downto 0);
  renable <= props_in(idx).renable;
  wenable <= props_in(idx).wenable;
  g0: for i in 0 to 1 generate
    props_out(i).data <= rdata & rdata & rdata & rdata; -- replicate read data everywhere
    props_out(i).present(2 to props_out(i).present'right) <= (others => '0');
    props_out(i).done <= to_bool(idx = i and done);
    g1: for k in 0 to 1 generate
      props_out(i).present(k) <= props_in(k).present; -- replicate presence bits
    end generate g1;
    dev_out(i).rx_clk_in  <= rx_clk_in;
    dev_out(i).tx_clk_in  <= tx_clk_in;
    dev_out(i).rx_present <= props_in(0).present;
    dev_out(i).tx_present <= props_in(1).present;
  end generate g0;

  spi : entity work.spi
    generic map(
      nusers => 2,
      data_width => 8,
      addr_width => 7,
      clock_divisor => 16
      )
    port map(
      clk     => wci_clk,
      reset   => wci_reset,
      renable => renable,
      wenable => wenable,
      addr    => addr,
      wdata   => wdata,
      rdata   => rdata,
      done    => done,
      busy    => busy,
      sdo     => sdo,
      sclk    => sclk,
      sen     => sen,
      sdio    => sdio
      );

  arb : process(wci_Clk) is begin
    if rising_edge(wci_Clk) then
      if wci_Reset = '1' then
        who_has_it <= props_in(1).present;
      elsif not its(busy) and props_in(0).present and props_in(1).present then
        who_has_it <= not who_has_it;
      end if;
    end if;
  end process arb;
end rtl;
