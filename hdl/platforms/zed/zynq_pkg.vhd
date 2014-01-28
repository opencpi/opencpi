-- This file contains zynq-specific definitions that have nothing to do with the
-- particulars of the zed board.

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
package zynq_pkg is

-- MAKE REFERENCE TO PROCESSING_SYSTEM7 DATA SHEET
-- Signals into the Zynq PS core (not involved with the PL, but from pins)
--type ps_in_t is record
--  PS_SRSTB : std_logic;
--  PS_CLK   : std_logic;
--  PS_PORB  : std_logic;
--end record ps_in_t;

-- Signals out of the Zynq PS (not involved with the PL)
--type ps_out_t is record
--end record ps_out_t;

-- Signals inout of the Zynq PS (not involved with the PL)
--type ps_inout_t is record
--  MIO          : std_logic_vector(53 downto 0);
--  DDR_Clk      : std_logic;
--  DDR_Clk_n    : std_logic;
--  DDR_CKE      : std_logic;
--  DDR_CS_n     : std_logic;
--  DDR_RAS_n    : std_logic;
--  DDR_WEB      : std_logic;
--  DDR_CAS_n    : std_logic;
--  DDR_BankAddr : std_logic_vector(2 downto 0);
--  DDR_Addr     : std_logic_vector(14 downto 0);
--  DDR_ODT      : std_logic;
--  DDR_DRSTB    : std_logic;
--  DDR_DQ       : std_logic_vector(31 downto 0);
--  DDR_DM       : std_logic_vector(3 downto 0);
--  DDR_DQS      : std_logic_vector(3 downto 0);
--  DDR_DQS_n    : std_logic_vector(3 downto 0);
--  DDR_VRN      : std_logic;
--  DDR_VRP      : std_logic;
--end record ps_inout_t;

-- Signals from the PS for use by the PL
type ps2pl_t is record
  FCLK         : std_logic;
  FCLKRESET_N  : std_logic;
end record ps2pl_t;

-- Inputs to the m_axi_gp port of the PS
type axi_gp_in_t is record
  ACLK    : std_logic;
  ARREADY : std_logic;
  AWREADY : std_logic;
  BVALID  : std_logic;
  RLAST   : std_logic;
  RVALID  : std_logic;
  WREADY  : std_logic;
  BID     : std_logic_vector(11 downto 0);
  RID     : std_logic_vector(11 downto 0);
  BRESP   : std_logic_vector(1 downto 0);
  RRESP   : std_logic_vector(1 downto 0);
  RDATA   : std_logic_vector(31 downto 0);
end record axi_gp_in_t;

-- Outputs from the m_axk_gp port of the PS into the PL
type axi_gp_out_t is record
  ARESETN : std_logic;
  ARVALID : std_logic;
  AWVALID : std_logic;
  BREADY  : std_logic;
  RREADY  : std_logic;
  WLAST   : std_logic;
  WVALID  : std_logic;
  ARID    : std_logic_vector(11 downto 0);
  AWID    : std_logic_vector(11 downto 0);
  WID     : std_logic_vector(11 downto 0);
  ARBURST : std_logic_vector(1 downto 0);
  ARLOCK  : std_logic_vector(1 downto 0);
  ARSIZE  : std_logic_vector(2 downto 0);
  AWBURST : std_logic_vector(1 downto 0);
  AWLOCK  : std_logic_vector(1 downto 0);
  AWSIZE  : std_logic_vector(2 downto 0);
  ARPROT  : std_logic_vector(2 downto 0);
  AWPROT  : std_logic_vector(2 downto 0);
  ARADDR  : std_logic_vector(31 downto 0);
  AWADDR  : std_logic_vector(31 downto 0);
  WDATA   : std_logic_vector(31 downto 0);
  ARCACHE : std_logic_vector(3 downto 0);
  ARLEN   : std_logic_vector(3 downto 0);
  ARQOS   : std_logic_vector(3 downto 0);
  AWCACHE : std_logic_vector(3 downto 0);
  AWLEN   : std_logic_vector(3 downto 0);
  AWQOS   : std_logic_vector(3 downto 0);
  WSTRB   : std_logic_vector(3 downto 0);
end record axi_gp_out_t;

subtype  Resp_t IS std_logic_vector(1 downto 0);
constant Resp_OKAY   : Resp_t := "00";
constant Resp_EXOKAY : Resp_t := "01";
constant Resp_SLVERR : Resp_t := "10";
constant Resp_DECERR : Resp_t := "11";

end package zynq_pkg;
