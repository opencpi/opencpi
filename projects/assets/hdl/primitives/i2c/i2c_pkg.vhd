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

-- package for letting VHDL access the verilogs in this library
library ieee; use ieee.std_logic_1164.all, ieee.numeric_std.all;
package i2c is

  component i2c_master_bit_ctrl is
    port (clk     : in  std_logic;
          rst     : in  std_logic;
          nReset  : in  std_logic;
          ena     : in  std_logic;
          clk_cnt : in  std_logic_vector(15 downto 0);
          cmd     : in  std_logic_vector(3 downto 0);
          cmd_ack : out std_logic;
          busy    : out std_logic;
          al      : out std_logic;
          din     : in  std_logic;
          dout    : out std_logic;
          scl_i   : in  std_logic;
          scl_o   : out std_logic;
          scl_oen : out std_logic;
          sda_i   : in  std_logic;
          sda_o   : out std_logic;
          sda_oen : out std_logic
          );
  end component i2c_master_bit_ctrl;

  component i2c_master_byte_ctrl is
    port (clk      : in  std_logic;
          rst      : in  std_logic;
          nReset   : in  std_logic;
          ena      : in  std_logic;
          clk_cnt  : in  std_logic_vector(15 downto 0);
          start    : in  std_logic;
          stop     : in  std_logic;
          read     : in  std_logic;
          write    : in  std_logic;
          ack_in   : in  std_logic;
          din      : in  std_logic_vector(7 downto 0);
          cmd_ack  : out std_logic;
          ack_out  : out std_logic;
          i2c_busy : out std_logic;
          i2c_al   : out std_logic;
          dout     : out std_logic_vector(7 downto 0);
          scl_i    : in  std_logic;
          scl_o    : out std_logic;
          scl_oen  : out std_logic;
          sda_i    : in  std_logic;
          sda_o    : out std_logic;
          sda_oen  : out std_logic
          );
  end component i2c_master_byte_ctrl;

  component i2c_opencores_ctrl is
    generic (
      clk_cnt : unsigned(15 downto 0) := x"00C7");
    port (
      wci_clk         : in  std_logic;
      wci_reset       : in  std_logic;
      number_of_bytes : in  std_logic_vector(2 downto 0);
      is_read         : in  std_logic;
      is_write        : in  std_logic;
      addr            : in  std_logic_vector(7 downto 0);
      slave_addr      : in  std_logic_vector(6 downto 0);
      wdata           : in  std_logic_vector(31 downto 0);
      rdata           : out std_logic_vector(31 downto 0);
      done            : out std_logic;
      error           : out std_logic;
      scl_i           : in  std_logic;
      scl_o           : out std_logic;
      scl_oen         : out std_logic;
      sda_i           : in  std_logic;
      sda_o           : out std_logic;
      sda_oen         : out std_logic
      );
  end component i2c_opencores_ctrl;

end package i2c;
