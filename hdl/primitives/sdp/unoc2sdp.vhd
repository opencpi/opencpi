library ieee, ocpi, platform;
use IEEE.std_logic_1164.all, IEEE.numeric_std.all,
  ocpi.all, ocpi.types.all, ocpi.util.all,
  platform.all, work.sdp.all;
entity unoc2sdp is
  generic(ocpi_debug   :     bool_t;
          sdp_width    :     natural);
  port (  unoc_in      : in  platform_pkg.unoc_master_out_t;
          unoc_out     : out platform_pkg.unoc_master_in_t;
          sdp_in       : in s2m_t;
          sdp_out      : out m2s_t;
          sdp_in_data  : in dword_array_t(0 to sdp_width-1);
          sdp_out_data : out dword_array_t(0 to sdp_width-1));
end entity;
architecture rtl of unoc2sdp is
begin
end rtl;
