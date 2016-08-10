library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library sdp; use sdp.sdp.all;
architecture rtl of sdp_term_rv is
  signal count_r : uchar_t;
begin
  up_out_data         <= (others => (others => '0'));
  up_out.sdp.valid    <= bfalse;
  up_out.sdp.eop      <= bfalse;
  up_out.sdp.ready    <= up_in.sdp.valid;
  up_out.sdp.header   <= dws2header((others => (others => '0')));
  up_out.dropCount    <= count_r;
  process (up_in.clk) is
  begin
    if rising_edge(up_in.clk) then
      if its(up_in.reset) then
        count_r <= (others => '0');
      elsif up_in.sdp.valid = '1' and count_r /= uchar_max then
        count_r <= count_r + 1;
      end if;
    end if;
  end process;
end rtl;
