library ieee, ocpi, platform;
use IEEE.std_logic_1164.all, IEEE.numeric_std.all,
  ocpi.all, ocpi.types.all, ocpi.util.all,
  platform.all, work.sdp.all;
entity sdp_receive is
  generic(ocpi_debug    :     bool_t;
          sdp_width     :     natural;
          data_width    :     natural; -- in bits
          byte_width    :     natural);
  port(   sdp_in        : in  m2s_t;
          sdp_out       : out s2m_t;
          sdp_in_data   : in  dword_array_t(0 to sdp_width-1);
          sdp_out_data  : out dword_array_t(0 to sdp_width-1);
          wsi_in        : in  wsi.s2m_t;
          wsi_out       : out wsi.m2s_t;
          wsi_data_out  : out  std_logic_vector(data_width-1 downto 0);
          wsi_byteen_out: out  std_logic_vector(data_width/byte_width-1 downto 0));
end entity;
architecture rtl of sdp_receive is
begin
end rtl;
