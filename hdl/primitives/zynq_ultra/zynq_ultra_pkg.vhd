-- This package enables VHDL code to instantiate all entities and modules in this library
library ieee; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all;
library platform; use platform.platform_pkg.all;
library axi; use axi.axi_pkg.all;
package zynq_ultra_pkg is

-- Signals from the PS for use by the PL
type ps2pl_t is record
  FCLK         : std_logic_vector(3 downto 0);
  FCLKRESET_N  : std_logic;
end record ps2pl_t;
-- Signals from the PL for use by the PS
type pl2ps_t is record
  DEBUG        : std_logic_vector(31 downto 0); --     FTMT_F2P_DEBUG
end record pl2ps_t;

component zynq_ultra_ps_e is
  port(
    ps_in        : in    pl2ps_t;
    ps_out       : out   ps2pl_t;
    m_axi_hp_in  : in    m_axi_hp_in_array_t(0 to C_M_AXI_HP_COUNT-1);
    m_axi_hp_out : out   m_axi_hp_out_array_t(0 to C_M_AXI_HP_COUNT-1);
    s_axi_hp_in  : in    s_axi_hp_in_addr36_array_t(0 to C_S_AXI_HP_COUNT-1);
    s_axi_hp_out : out   s_axi_hp_out_array_t(0 to C_S_AXI_HP_COUNT-1)
    );
end component zynq_ultra_ps_e;
end package zynq_ultra_pkg;
