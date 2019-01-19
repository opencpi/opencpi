library IEEE; use IEEE.std_logic_1164.all, ieee.numeric_std.all;
library platform; use platform.platform_pkg.all;
library ocpi; use ocpi.types.all, ocpi.util.all;
library axi; use axi.axi_pkg.all;
library sdp; use sdp.sdp.all;

-- This is the sdp2axi entity with 32 bits of axi_in address space
entity sdp2axi is
  generic(
    ocpi_debug   : boolean;
    sdp_width    : natural;
    axi_width    : natural);
  port(
    clk          : in  std_logic;
    reset        : in  bool_t;
    sdp_in       : in  s2m_t;
    sdp_in_data  : in  dword_array_t(0 to sdp_width-1);
    sdp_out      : out m2s_t;
    sdp_out_data : out dword_array_t(0 to sdp_width-1);
    axi_in       : in  s_axi_hp_out_t;
    axi_out      : out s_axi_hp_in_t;
    axi_error    : out bool_t;
    dbg_state    : out ulonglong_t;
    dbg_state1   : out ulonglong_t;
    dbg_state2   : out ulonglong_t
    );
end entity sdp2axi;
