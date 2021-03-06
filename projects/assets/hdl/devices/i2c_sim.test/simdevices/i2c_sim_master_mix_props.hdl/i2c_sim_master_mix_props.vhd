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

-- This worker has the register descriptions in its XML, but
-- delegates the control interface to an I2C subdevice.
architecture rtl of i2c_sim_master_mix_props_worker is
  signal wdata : std_logic_vector(31 downto 0);
  signal byte_enable : std_logic_vector(props_in.raw.byte_enable'range);  -- for modelsim
begin
  byte_enable <= props_in.raw.byte_enable;
  -- Control plane outputs.  Raw props routed to underlying I2C
  rawprops_out.present         <= '1';
  rawprops_out.reset           <= ctl_in.reset;
  rawprops_out.raw.address     <= props_in.raw.address srl 1;  --Need to shift to 16bit addressing
  rawprops_out.raw.byte_enable <= props_in.raw.byte_enable;
  rawprops_out.raw.is_read     <= props_in.raw.is_read;
  rawprops_out.raw.is_write    <= props_in.raw.is_write;
  rawprops_out.raw.data        <= wdata when props_in.raw.is_write = '1' else props_in.raw.data;
  props_out.raw                <= rawprops_in.raw;
  --Input byte enable decode (LSB then MSB then 0)
  be_input : process (props_in.raw.byte_enable, props_in.raw.data)
  begin
    case byte_enable is
      when "0001" => wdata <= x"000000"&props_in.raw.data(7 downto 0);
      when "0010" => wdata <= x"0000"&props_in.raw.data(7 downto 0)&x"00";
      when "0100" => wdata <= x"00"&props_in.raw.data(7 downto 0)&x"0000";
      when "1000" => wdata <= props_in.raw.data(7 downto 0)&x"000000";
      when "0011" => wdata <= x"0000"&props_in.raw.data(7 downto 0)&props_in.raw.data(15 downto 8);
      when "0110" => wdata <= x"00"&props_in.raw.data(15 downto 8)&props_in.raw.data(23 downto 16)&x"00";
      when "1100" => wdata <= props_in.raw.data(23 downto 16)&props_in.raw.data(31 downto 24)&x"0000";
      when others => wdata <= (others => '0');
    end case;
  end process be_input;
end rtl;
