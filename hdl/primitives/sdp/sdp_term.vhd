library ieee, ocpi, platform;
use IEEE.std_logic_1164.all, IEEE.numeric_std.all,
  ocpi.all, ocpi.types.all, ocpi.util.all,
  platform.all, work.sdp.all;
entity sdp_term is
  generic(ocpi_debug      :     bool_t;
          sdp_width       :     uchar_t);
  port   (up_in           : in  m2s_t;
          up_out          : out s2m_t;
          up_in_data      : in  dword_array_t(0 to to_integer(sdp_width)-1);
          up_out_data     : out dword_array_t(0 to to_integer(sdp_width)-1);
          drop_count      : out uchar_t);
end entity;
architecture rtl of sdp_term is
  signal count_r : uchar_t;
begin
  up_out_data  <= (others => (others => '0'));
  up_out.sdp.valid  <= bfalse;
  up_out.sdp.eom    <= bfalse;
  up_out.sdp.ready  <= up_in.sdp.valid;
  up_out.sdp.header <= dws2header((others => (others => '0')));
  drop_count        <= count_r;
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
