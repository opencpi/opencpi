library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
library work; use work.platform_pkg.all;

entity unoc_terminator is
  port(
    CLK, RST_N : in std_logic;
    up_in      : in  unoc_link_t;
    up_out     : out unoc_link_t;
    drop_count : out unsigned(7 downto 0)
    );
end entity unoc_terminator;
architecture rtl of unoc_terminator is
  signal count : unsigned(drop_count'range);
begin
  up_out.data  <= (others => '0');
  up_out.valid <= '0';
  up_out.take  <= up_in.valid;
  drop_count   <= count;
  process (clk) is
  begin
    if rising_edge(clk) then
      if rst_n = '0' then
        count <= (others => '0');
      elsif up_in.valid = '1' and count /= 255 then
        count <= count + 1;
      end if;
    end if;
  end process;
end architecture rtl;
