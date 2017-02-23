-- The subdevice for the ad9361. see xml file for comments.
library IEEE, ocpi, util;
use IEEE.std_logic_1164.all, ieee.numeric_std.all;
use ocpi.all, ocpi.types.all, ocpi.util.all;
architecture rtl of ad9361_sub_worker is
  constant rx_c         : natural := 0;         
  constant tx_c         : natural := 1;         
  constant adc_c        : natural := 2;
  constant dac_c        : natural := 3;         
  constant addr_width_c : natural := 15;
  -- Internal signals
  signal rdata      : std_logic_vector(7 downto 0);
  signal wdata      : std_logic_vector(7 downto 0);
  signal addr       : unsigned(addr_width_c-1 downto 0);
  -- Convenience
  signal lsb        : natural range 0 to 31;
  signal raw_in     : wci.raw_prop_out_t;
  signal raw_out    : wci.raw_prop_in_t;
  signal myReset_r  : std_logic; -- stable output based on all clients
  -- Debug
  signal lastClient : uchar_t;
  signal lastAddr   : ushort_t;
  signal lastData   : uchar_t;
  signal active     : bool_t;
  signal done       : bool_t;
  signal index      : integer range 0 to 4-1;
begin
  -- debug
  raw_out.raw.done <= done;
  props_out.lastClient <= lastClient;
  props_out.lastAddr   <= lastAddr;
  props_out.lastData   <= lastData;
  props_out.resetting  <= myReset_r;
  -- Assert chip reset if system is reset or both clients are reset, chip reset as asserted low
  resetb       <= not (ctl_in.reset or myReset_r);
  -- Other outputs (default value)
  enable       <= dev_in(rx_c).enable; -- rx enable in FDD mode
  txnrx        <= dev_in(tx_c).enable; -- tx enable in FDD mode
  sync_in      <= '0';
  lsb          <= 8 * to_integer(raw_in.raw.address(1 downto 0));
  -- These are temp signals because Xilinx isim can't put them as actuals
  wdata        <= raw_in.raw.data(lsb + 7 downto lsb);
  addr         <= raw_in.raw.address(addr'range);
  raw_out.raw.data  <= rdata & rdata & rdata & rdata;
  dev_out(adc_c).data_clk_p <= data_clk_p;
  dev_out(adc_c).data_clk_n <= data_clk_n;
  dev_out(dac_c).data_clk_p <= data_clk_p;
  dev_out(dac_c).data_clk_n <= data_clk_n;
  -- A registered reset output set when all present clients are reset
  p : process(ctl_in.clk) is begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        myReset_r <= '1';
      else
        myReset_r <=
          not ((rprops_in(rx_c).present and not rprops_in(rx_c).reset) or
               (rprops_in(tx_c).present and not rprops_in(tx_c).reset) or
               (rprops_in(adc_c).present and not rprops_in(adc_c).reset) or
               (rprops_in(dac_c).present and not rprops_in(dac_c).reset));
        if its(active) then
          if its(done) then
            lastData <= to_uchar(rdata);
            active   <= bfalse;
          end if;
        elsif its(raw_in.raw.is_read) then
          active     <= btrue;
          lastAddr   <= resize(raw_in.raw.address, ushort_t'length);
          lastClient <= to_uchar(index);
        end if;
      end if;
    end if;
  end process;
  -- Use the generic raw property arbiter among clients
  arb : wci.raw_arb
    generic map(nusers => 4)
    port map(
      clk         => ctl_in.clk,
      reset       => ctl_in.reset,
      from_users  => rprops_in,
      to_users    => rprops_out,
      from_device => raw_out,
      to_device   => raw_in,
      index       => index);
  -- Drive SPI from the arbitrated raw interface
  spi : util.util.spi
    generic map(
      data_width    => 8,
      addr_width    => addr_width_c,
      clock_divisor => 16,
      capture_fall  => true)
    port map(
      clk     => ctl_in.clk,
      reset   => ctl_in.reset,
      renable => raw_in.raw.is_read,
      wenable => raw_in.raw.is_write,
      addr    => addr,
      wdata   => wdata,
      rdata   => rdata,
      done    => done,
      sdo     => spi_do,
      sclk    => spi_clk,
      sen     => spi_enb,
      sdio    => spi_di);
end rtl;
