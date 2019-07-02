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

-- An I2C generic subdevice for the fmcomms2/3 cards

library IEEE, ocpi, i2c;
use IEEE.std_logic_1164.all, ieee.numeric_std.all;
use ocpi.all, ocpi.types.all; use ocpi.util.all;
use i2c.i2c.all;
architecture rtl of fmcomms_2_3_i2c_worker is
  constant addr_width_c    : natural := 7;
  signal   scl_padoen      : std_logic;
  signal   sda_padoen      : std_logic;
  -- Internal signals
  signal   rdata           : std_logic_vector(31 downto 0);
  signal   wdata           : std_logic_vector(31 downto 0);
  signal   addr            : std_logic_vector(addr_width_c downto 0);
  -- Convenience
  signal   raw_in          : wci.raw_prop_out_t;
  signal   raw_out         : wci.raw_prop_in_t;
  signal   number_of_bytes : std_logic_vector(2 downto 0);
  signal   index           : integer range 0 to 1;
  signal   raw_enable      : std_logic_vector(3 downto 0);
  signal   slave_addr      : std_logic_vector(6 downto 0);
  -- addressing, assuming GA0 and GA1 set to 0
  constant ad7291_addr     : std_logic_vector(6 downto 0) := "0101111"; -- Table 31 in datasheet
  signal   stm_mc24c02_addr     : std_logic_vector(6 downto 0);
  constant stm_mc24c02_addr_6_3 : std_logic_vector(3 downto 0) := "1010"; -- Table 2 in datasheet
  -- MC24C02 TSSOP package-specific addressing
  constant stm_mc24c02_addr_2   : std_logic := '0';     -- MC24C02 pin 3 (E2) is grounded on FMCOMMS2/3 boards
  signal   stm_mc24c02_addr_1   : std_logic := FMC_GA0; -- MC24C02 pin 2 (E1) is connected to FMC GA0
  signal   stm_mc24c02_addr_0   : std_logic := FMC_GA1; -- MC24C02 pin 1 (E0) is connected to FMC GA1
begin
  raw_enable      <= raw_in.raw.byte_enable;
  stm_mc24c02_addr <= stm_mc24c02_addr_6_3 & stm_mc24c02_addr_2 & stm_mc24c02_addr_1 & stm_mc24c02_addr_0;
  slave_addr      <= ad7291_addr when index = 0 else stm_mc24c02_addr;
  number_of_bytes <= std_logic_vector(to_unsigned(count_ones(raw_in.raw.byte_enable), number_of_bytes'length));
  addr            <= std_logic_vector(raw_in.raw.address(addr'range));

  --Input byte enable decode
  be_input : process (raw_in.raw.byte_enable, raw_in.raw.data)
  begin
    case raw_enable is
      when "0011" => wdata <= x"0000"&raw_in.raw.data(15 downto 0);
      when "0110" => wdata <= x"0000"&raw_in.raw.data(23 downto 8);
      when "1100" => wdata <= x"0000"&raw_in.raw.data(31 downto 16);
      when "0001" => wdata <= x"000000"&raw_in.raw.data(7 downto 0);
      when "0010" => wdata <= x"000000"&raw_in.raw.data(15 downto 8);
      when "0100" => wdata <= x"000000"&raw_in.raw.data(23 downto 16);
      when "1000" => wdata <= x"000000"&raw_in.raw.data(31 downto 24);
      when "0111" => wdata <= x"00"&raw_in.raw.data(23 downto 0);
      when "1111" => wdata <= raw_in.raw.data;
      when others => wdata <= (others => '0');
    end case;
  end process be_input;

  --Output byte enable encoding
  be_output : process (raw_in.raw.byte_enable, rdata)
  begin
    case raw_enable is
      when "0011" => raw_out.raw.data <= x"0000"&rdata(31 downto 16);
      when "0110" => raw_out.raw.data <= x"00"&rdata(31 downto 16)&x"00";
      when "1100" => raw_out.raw.data <= rdata(31 downto 16)&x"0000";
      when "0001" => raw_out.raw.data <= x"000000"&rdata(31 downto 24);
      when "0010" => raw_out.raw.data <= x"0000"&rdata(31 downto 24)&x"00";
      when "0100" => raw_out.raw.data <= x"00"&rdata(31 downto 24)&x"0000";
      when "1000" => raw_out.raw.data <= rdata(31 downto 24)&x"000000";
      when "0111" => raw_out.raw.data <= rdata(31 downto 8)&x"00";
      when "1111" => raw_out.raw.data <= rdata;
      when others => raw_out.raw.data <= (others => '0');
    end case;
  end process be_output;

  -- Use the generic raw property arbiter between the nusers inputs
  arb : wci.raw_arb
    generic map(nusers => 2)
    port map(
      CLK         => wci_clk,
      RESET       => wci_reset,
      FROM_USERS  => rprops_in,
      TO_USERS    => rprops_out,
      FROM_DEVICE => raw_out,
      TO_DEVICE   => raw_in,
      INDEX       => index);

  -- State machine to control OpenCores I2C module
  fsm_controller : i2c.i2c.i2c_opencores_ctrl
    generic map (
      CLK_CNT => to_unsigned(from_float(CP_CLK_FREQ_p)/from_float(I2C_CLK_FREQ_p),16) )
    port map(
      WCI_CLK         => wci_clk,
      WCI_RESET       => wci_reset,
      NUMBER_OF_BYTES => number_of_bytes,
      IS_READ         => raw_in.raw.is_read,
      IS_WRITE        => raw_in.raw.is_write,
      SLAVE_ADDR      => slave_addr,
      ADDR            => addr,
      WDATA           => wdata,
      RDATA           => rdata,
      DONE            => raw_out.raw.done,
      ERROR           => raw_out.raw.error,
      SCL_I           => SCL_I,
      SCL_O           => SCL_O,
      SCL_OEN         => scl_padoen,
      SDA_I           => SDA_I,
      SDA_O           => SDA_O,
      SDA_OEN         => sda_padoen);

  --Invert output enables for tri_states
  SCL_OE <= not scl_padoen;
  SDA_OE <= not sda_padoen;
end rtl;
