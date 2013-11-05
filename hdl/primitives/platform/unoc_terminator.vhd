library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
library work; use work.platform_pkg.all;

entity unoc_terminator is
  port(
    up_in      : in  unoc_master_out_t;
    up_out     : out unoc_master_in_t;
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
  process (up_in.clk) is
  begin
    if rising_edge(up_in.clk) then
      if up_in.reset_n = '0' then
        count <= (others => '0');
      elsif up_in.valid = '1' and count /= 255 then
        count <= count + 1;
      end if;
    end if;
  end process;
end architecture rtl;
