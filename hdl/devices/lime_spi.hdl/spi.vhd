-- SPI engine.  Essentially a raw-to-spi adapter, but with our parent module already
-- dealing with converting the raw bus to the specified addr and data width.
library IEEE, ocpi;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ocpi.types.all, ocpi.util.all;
entity spi is
  generic (
    data_width    : positive := 8;
    addr_width    : positive := 7;
    clock_divisor : positive := 16
  );
  port (
    -- The raw interface signals (with widths adjusted)
    clk           : in  std_logic;
    reset         : in  bool_t;
    renable       : in  bool_t;
    wenable       : in  bool_t;
    addr          : in  unsigned(addr_width-1 downto 0);
    wdata         : in  std_logic_vector(data_width-1 downto 0);
    rdata         : out std_logic_vector(data_width-1 downto 0);
    done          : out bool_t;
    -- The SPI signals
    sdo           : in  std_logic;
    sclk          : out std_logic;
    sen           : out std_logic;
    sdio          : out std_logic
  );
end entity spi;
architecture rtl of spi is
  -- Registers/state
  signal busy_r      : bool_t;
  signal done_r      : bool_t; -- for one pulse
  signal rdata_r     : std_logic_vector(data_width-1 downto 0) := (others => '0');
  signal clk_count_r : unsigned(width_for_max(clock_divisor-1)-1 downto 0) := (others => '0');
  signal bit_count_r : unsigned(width_for_max(addr_width + data_width + 1)-1 downto 0) := (others => '0');
begin
  done  <= done_r;
  rdata <= rdata_r;
  SEN   <= not busy_r;                       -- enable is asserted low here
  SDIO  <= wenable when bit_count_r = 0 else
           -- Select the right bit from the 7 bit address
           addr(addr_width - to_integer(bit_count_r)) when bit_count_r <= addr_width else
           -- Select the right bit from the 32 bit data (little endian)
           wdata(data_width - to_integer(bit_count_r - addr_width));
  p : process(clk) is begin
    if rising_edge(clk) then
      if its(reset) then
        busy_r      <= bfalse;
        done_r      <= bfalse;
        sclk        <= '0';
        bit_count_r <= (others => '0');
        clk_count_r <= (others => '0');
        -- no need to set counters since they get initialized when a cycle starts
      elsif its(busy_r) then -- We are doing an access
        clk_count_r <= clk_count_r + 1;
        if clk_count_r = clock_divisor/2 - 1 then
          sclk <= '1';
        elsif clk_count_r = clock_divisor - 1 then
          clk_count_r <= (others => '0');
          sclk        <= '0';
          if bit_count_r = (data_width + addr_width) then
            busy_r <= bfalse;
            done_r <= btrue;
            bit_count_r <= (others => '0');
          else
            if bit_count_r > addr_width then
              rdata_r(data_width - to_integer(bit_count_r - addr_width)) <= SDO;
            end if;
            bit_count_r <= bit_count_r + 1;
          end if;
        end if;
      elsif its(done_r) then
        done_r <= bfalse;
      elsif renable or wenable then
        -- starting an access after busy_r has been down for a cycle
        busy_r      <= btrue;
        sclk        <= '0';
      end if;
    end if;
  end process p;
end rtl;
