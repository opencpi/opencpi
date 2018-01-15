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

-- THIS FILE WAS ORIGINALLY GENERATED ON Fri Jun 30 11:37:07 2017 EDT
-- BASED ON THE FILE: ad9361_spi.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: ad9361_spi

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library util; use util.util.all; use util.types.all;
architecture rtl of ad9361_spi_worker is
  constant addr_width_c : natural := 15;
  -- Internal signals
  signal rdata      : std_logic_vector(7 downto 0) := (others => '0');
  signal wdata      : std_logic_vector(7 downto 0) := (others => '0');
  signal addr       : unsigned(addr_width_c-1 downto 0) := (others => '0');
  signal reset      : std_logic := '0';
  -- Convenience
  signal lsb        : natural range 0 to 31 := 0;
  signal myReset_r  : std_logic := '0'; -- stable output based on all clients
  signal lastAddr   : ushort_t := (others => '0');
  signal lastData   : uchar_t;
  signal active     : bool_t := bfalse;
  signal done       : bool_t := bfalse;
  -- signals interface
  signal SPI_DI_s     : std_logic := '0';
  signal SPI_CLK_s    : std_logic := '0';
  signal RESETB_s     : std_logic := '0';
  signal SPI_ENB_s    : std_logic := '0';
  signal SPI_DO_s     : std_logic := '0';
begin
  -- debug
  rprops_out.raw.done <= done;
  props_out.lastAddr   <= lastAddr;
  props_out.lastData   <= lastData;
  -- Assert chip reset if system is reset or client is reset, chip reset as asserted low
  props_out.resetting  <= myReset_r;
  reset        <= ctl_in.reset or myReset_r;
  RESETB_s     <= not reset; -- RESETB is active-low
  lsb          <= 8 * to_integer(rprops_in.raw.address(1 downto 0));
  -- These are temp signals because Xilinx isim can't put them as actuals
  wdata        <= rprops_in.raw.data(lsb + 7 downto lsb);
  addr         <= rprops_in.raw.address(addr'range);
  rprops_out.raw.data  <= rdata & rdata & rdata & rdata;

  -- have to drive these 2 signals when not using raw arbitrater
  rprops_out.raw.error <= bfalse;
  rprops_out.present <= (others => rprops_in.present);

  -- A registered reset output set when the present client is reset
  p : process(ctl_in.clk) is begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        myReset_r <= '1';
      else
        myReset_r <= (not rprops_in.present) or rprops_in.reset or
                     dev_force_reset_in.force_reset;
        if its(active) then
          if its(done) then
            lastData <= to_uchar(rdata);
            active   <= bfalse;
          end if;
        elsif its(rprops_in.raw.is_read) then
          active     <= btrue;
          lastAddr   <= resize(rprops_in.raw.address, ushort_t'length);
        end if;
      end if;
    end if;
  end process;

  -- Drive SPI from raw props
  spi : util.util.spi
    generic map(
      data_width    => 8,
      addr_width    => addr_width_c,
      clock_divisor => to_integer(to_unsigned(from_float(CP_CLK_FREQ_HZ_p)/
                                              from_float(SPI_CLK_FREQ_HZ_p),16)),
      capture_fall  => true)
    port map(
      clk     => ctl_in.clk,
      reset   => reset,
      renable => rprops_in.raw.is_read,
      wenable => rprops_in.raw.is_write,
      addr    => addr,
      wdata   => wdata,
      rdata   => rdata,
      done    => done,
      sdo     => SPI_DO_s,
      sclk    => SPI_CLK_s,
      sen     => SPI_ENB_s,
      sdio    => SPI_DI_s);

  signals : entity work.signals
    port map(
      w_SPI_DI     => SPI_DI_s,
      w_SPI_CLK    => SPI_CLK_s,
      w_RESETB     => RESETB_s,
      w_SPI_ENB    => SPI_ENB_s,
      w_SPI_DO     => SPI_DO_s,
      SPI_DI       => SPI_DI,
      SPI_CLK      => SPI_CLK,
      RESETB       => RESETB,
      SPI_ENB      => SPI_ENB,
      SPI_DO       => SPI_DO);

end rtl;
