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

-- The control plane, wrapped for VHDL and no connecting logic
library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all, std.textio.all;
library ocpi; use ocpi.all, ocpi.types.all;
library platform; use platform.platform_pkg.all, platform.metadata_defs.all;
library bsv;
library util;
entity metadata_rv is
  generic(romwords : natural := 2048);
  port(
    wci_Clk      : in std_logic;
    wci_Reset_n  : in std_logic;
    metadata_in  : in  metadata_out_t;
    metadata_out : out metadata_in_t
    );
end entity metadata_rv;
architecture rtl of metadata_rv is
  component mkUUID is
    port (uuid : out std_logic_vector(511 downto 0));
  end component mkUUID;
  signal myUUID : std_logic_vector(511 downto 0);

  --type meta_t is array (0 to 1023) of std_logic_vector(31 downto 0);
  --impure function initmeta return meta_t is
  --  file metafile : text open read_mode is "metadatarom.dat";
  --  variable metamem : meta_t;
  --  variable metaline : line;
  --  variable metabits : bit_vector(31 downto 0);
  --begin
  --  for i in meta_t'range loop
  --    readline(metafile, metaline);
  --    read(metaline, metabits);
  --    metamem(i) := to_stdlogicvector(metabits);
  --  end loop;
  --  return metamem;
  --end function;

  --constant metamem : meta_t := initmeta;
  constant addr_width : natural := ocpi.util.width_for_max(romwords-1);
  signal dout : std_logic_vector(31 downto 0);
  signal addr : std_logic_vector(addr_width-1 downto 0);
begin
  -- Type conversions
  metadata_out.romData <= to_ulong(dout);
  addr <= std_logic_vector(metadata_in.romAddr(addr_width-1 downto 0));

  -- Instance the (generated) UUID module.
  uu : mkUUID
  port map(uuid => myUUID);
  gen0: for i in 0 to metadata_out.UUID'right generate
    metadata_out.UUID(i) <= to_ulong(myUUID(511 - i*32 downto 511 - i*32 - 31));
  end generate gen0;

  -- Instance the ROM filled from generated metadata file
  rom : component util.util.ROM
    generic map(WIDTH    => 32,
                SIZE     => romwords,
                INITFILE => "metadatarom.dat")
       port map(CLK      => wci_Clk,
                ADDR     => addr,
                DO       => dout);
  
end architecture rtl;
