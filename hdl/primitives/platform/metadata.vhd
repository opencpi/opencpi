-- The control plane, wrapped for VHDL and no connecting logic
library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
library platform; use platform.platform_pkg.all, platform.metadata_defs.all;
library bsv;
entity metadata_rv is
  port(
    metadata_out : out metadata_in_t;
    metadata_in  : in  metadata_out_t
    );
end entity metadata_rv;
architecture rtl of metadata_rv is
  component UUID is
    port (uuid : out std_logic_vector(511 downto 0));
  end component UUID;
  signal myUUID : std_logic_vector(511 downto 0);
begin
  -- BRAMLoad comes from the bsv library
  rom : component bsv.bsv.BRAM1Load
    generic map(
      FILENAME   => "metadatarom.dat",
      ADDR_WIDTH => 10,
      DATA_WIDTH => 32,
      MEMSIZE    => 1024
      )
  port map(
    CLK => metadata_in.clk,
    EN  => metadata_in.romEn,
    WE  => '0',
    ADDR => std_logic_vector(metadata_in.romAddr(9 downto 0)),
    DI  => (others => '0'),
    To_ulong(DO)  => metadata_out.romData
    );
  uu : UUID
  port map(
    uuid => myUUID
    );
  gen0: for i in 0 to metadata_out.UUID'right generate
    metadata_out.UUID(i) <= to_ulong(myUUID(511 - i*32 downto 511 - i*32 - 31));
  end generate gen0;
  
end architecture rtl;
