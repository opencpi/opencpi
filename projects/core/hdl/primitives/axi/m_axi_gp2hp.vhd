library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library platform; use platform.all;
library work; use work.axi_pkg.all;
entity m_axi_gp2hp is
  port(
    gp_in  : in  m_axi_gp_in_t;
    gp_out : out m_axi_gp_out_t;
    hp_in  : out m_axi_hp_in_t;
    hp_out : in  m_axi_hp_out_t
    );
end entity m_axi_gp2hp;
architecture rtl of m_axi_gp2hp is
begin
  hp_in.ACLK     <= gp_in.ACLK     ;
  hp_in.AR.READY <= gp_in.ARREADY  ;
  hp_in.AW.READY <= gp_in.AWREADY  ;
  hp_in.B.VALID  <= gp_in.BVALID   ;
  hp_in.R.LAST   <= gp_in.RLAST    ;
  hp_in.R.VALID  <= gp_in.RVALID   ;
  hp_in.W.READY  <= gp_in.WREADY   ;
  hp_in.B.ID     <= gp_in.BID      ;
  hp_in.R.ID     <= gp_in.RID      ;
  hp_in.B.RESP   <= gp_in.BRESP    ;
  hp_in.R.RESP   <= gp_in.RRESP    ;
  hp_in.R.DATA   <= gp_in.RDATA    ;
  hp_in.AW.COUNT <= (others => '0');
  hp_in.AR.COUNT <= (others => '0');
  hp_in.W.COUNT  <= (others => '0');
  hp_in.R.COUNT  <= (others => '0');

  gp_out.ARESETN <= hp_out.ARESETN ;
  gp_out.ARVALID <= hp_out.AR.VALID;
  gp_out.AWVALID <= hp_out.AW.VALID;
  gp_out.BREADY  <= hp_out.B.READY ;
  gp_out.RREADY  <= hp_out.R.READY ;
  gp_out.WLAST   <= hp_out.W.LAST  ;
  gp_out.WVALID  <= hp_out.W.VALID ;
  gp_out.ARID    <= hp_out.AR.ID   ;
  gp_out.AWID    <= hp_out.AW.ID   ;
  gp_out.WID     <= hp_out.W.ID    ;
  gp_out.ARBURST <= hp_out.AR.BURST;
  gp_out.ARLOCK  <= hp_out.AR.LOCK ;
  gp_out.ARSIZE  <= hp_out.AR.SIZE ;
  gp_out.AWBURST <= hp_out.AW.BURST;
  gp_out.AWLOCK  <= hp_out.AW.LOCK ;
  gp_out.AWSIZE  <= hp_out.AW.SIZE ;
  gp_out.ARPROT  <= hp_out.AR.PROT ;
  gp_out.AWPROT  <= hp_out.AW.PROT ;
  gp_out.ARADDR  <= hp_out.AR.ADDR ;
  gp_out.AWADDR  <= hp_out.AW.ADDR ;
  gp_out.WDATA   <= hp_out.W.DATA  ;
  gp_out.ARCACHE <= hp_out.AR.CACHE;
  gp_out.ARLEN   <= hp_out.AR.LEN  ;
  gp_out.ARQOS   <= hp_out.AR.QOS  ;
  gp_out.AWCACHE <= hp_out.AW.CACHE;
  gp_out.AWLEN   <= hp_out.AW.LEN  ;
  gp_out.AWQOS   <= hp_out.AW.QOS  ;
  gp_out.WSTRB   <= hp_out.W.STRB  ;
end rtl;
