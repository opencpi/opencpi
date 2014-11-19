-- SPI engine
library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all, ocpi.util.all; -- remove this to avoid all ocpi name collisions
entity spi is
  generic (
    nusers        : positive := 1;
    data_width    : positive := 8;
    addr_width    : positive := 7;
    clock_divisor : positive := 16
  );
  port (
    clk           : in  std_logic;
    reset         : in  bool_t;
    renable       : in  bool_t;
    wenable       : in  bool_t;
    addr          : in  unsigned(addr_width-1 downto 0);
    wdata         : in  std_logic_vector(data_width-1 downto 0);
    rdata         : out std_logic_vector(data_width-1 downto 0);
    busy          : out bool_t;
    done          : out bool_t;
    sdo           : in  std_logic;
    sclk          : out std_logic;
    sen           : out std_logic;
    sdio          : out std_logic
  );
end entity spi;
architecture rtl of spi is
  -- Registers/state
  signal busy_r    : bool_t;
  signal my_rdata  : std_logic_vector(data_width-1 downto 0);
  signal clk_count : unsigned(width_for_max(clock_divisor-1)-1 downto 0);
  signal bit_count : unsigned(width_for_max(addr_width + data_width + 1)-1 downto 0);
begin
  busy <= busy_r;
  done <= to_bool(bit_count = 15);
  rdata <= my_rdata;
  SEN  <= not busy_r;
  SCLK <= clk_count(3);
  SDIO <= wenable when bit_count = 0 else
          -- Select the right bit from the 7 bit address
          addr(addr_width - to_integer(bit_count)) when bit_count <= addr_width else
          -- Select the right bit from the 32 bit data (little endian)
          wdata((to_integer(unsigned(addr(1 downto 0))) + 1) * 8 - to_integer(bit_count - 8));
  arb : process(clk) is begin
    if rising_edge(clk) then
      if its(reset) then
        busy_r <= bfalse;
      elsif its(busy_r) then -- We are doing an access
        clk_count <= clk_count + 1;
        if clk_count = clock_divisor - 1 then
          bit_count <= bit_count + 1;
          if bit_count > addr_width + 1 then
            my_rdata(data_width - (to_integer(bit_count) - (addr_width + 1))) <= SDO;
          end if;
          if bit_count = data_width then
            busy_r <= '0';
          end if;
        end if;
      elsif renable or wenable then -- starting an access
        busy_r    <= '1';
        bit_count <= (others => '0');
        clk_count <= (others => '0');
      end if;
    end if;
  end process arb;
end rtl;
