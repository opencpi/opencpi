-- This file is our wrapper around the generated PS wrapper which is the
-- result of running XPS, which is basically coregen on the processing_system7 IP.
-- There is no logic here, just providing a simpler interface for
-- the platform worker that uses it.

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library zed; use zed.all;
entity zed_ps is
  port(
    ps_in      : in    zynq_pkg.ps_in_t;
--    ps_out     : out   zynq_pkg.ps_out_t;
    ps_inout   : inout zynq_pkg.ps_inout_t;

    pl_out     : out   zynq_pkg.ps2pl_t;
    axi_gp_in  : in    zynq_pkg.axi_gp_in_t;
    axi_gp_out : out   zynq_pkg.axi_gp_out_t
    );
end entity zed_ps;

architecture rtl of zed_ps is
  -- from EDK/hw/XilinxProcessorIPLib/pcores/processing_system7_v4_01_a/hdl/verilog/processing_system7.v
  -- using ds871_processing_system7.pdf
  component processing_system7 is
    generic (
      C_USE_DEFAULT_ACP_USER_VAL      : integer := 1;
      C_S_AXI_ACP_ARUSER_VAL          : integer := 31;
      C_S_AXI_ACP_AWUSER_VAL          : integer := 31;
      C_M_AXI_GP0_THREAD_ID_WIDTH     : integer := 12;
      C_M_AXI_GP1_THREAD_ID_WIDTH     : integer := 12;
      C_M_AXI_GP0_ENABLE_STATIC_REMAP : integer := 1;
      C_M_AXI_GP1_ENABLE_STATIC_REMAP : integer := 1;
      C_M_AXI_GP0_ID_WIDTH            : integer := 12;
      C_M_AXI_GP1_ID_WIDTH            : integer := 12;
      C_S_AXI_GP0_ID_WIDTH            : integer := 6;
      C_S_AXI_GP1_ID_WIDTH            : integer := 6;
      C_S_AXI_HP0_ID_WIDTH            : integer := 6;
      C_S_AXI_HP1_ID_WIDTH            : integer := 6;
      C_S_AXI_HP2_ID_WIDTH            : integer := 6;
      C_S_AXI_HP3_ID_WIDTH            : integer := 6;
      C_S_AXI_ACP_ID_WIDTH            : integer := 3;
      C_S_AXI_HP0_DATA_WIDTH          : integer := 64;
      C_S_AXI_HP1_DATA_WIDTH          : integer := 64;
      C_S_AXI_HP2_DATA_WIDTH          : integer := 64;
      C_S_AXI_HP3_DATA_WIDTH          : integer := 64;
      C_INCLUDE_ACP_TRANS_CHECK       : integer := 0;
      C_NUM_F2P_INTR_INPUTS           : integer := 2;
      C_FCLK_CLK0_BUF                 : string  := "TRUE";
      C_FCLK_CLK1_BUF                 : string  := "TRUE";
      C_FCLK_CLK2_BUF                 : string  := "TRUE";
      C_FCLK_CLK3_BUF                 : string  := "TRUE";
      C_EMIO_GPIO_WIDTH               : integer := 64;
      C_INCLUDE_TRACE_BUFFER          : integer := 0;
      C_TRACE_BUFFER_FIFO_SIZE        : integer := 128;
      C_TRACE_BUFFER_CLOCK_DELAY      : integer := 12;
      USE_TRACE_DATA_EDGE_DETECTOR    : integer := 0;
      C_PS7_SI_REV                    : string  := "PRODUCTION";
      C_EN_EMIO_ENET0                 : integer := 0;
      C_EN_EMIO_ENET1                 : integer := 0;
      C_EN_EMIO_TRACE                 : integer := 0
      );
    port (
      CAN0_PHY_TX : out std_logic;
      CAN0_PHY_RX : in std_logic;
      CAN1_PHY_TX : out std_logic;
      CAN1_PHY_RX : in std_logic;
      ENET0_GMII_TX_EN : out std_logic;
      ENET0_GMII_TX_ER : out std_logic;
      ENET0_MDIO_MDC : out std_logic;
      ENET0_MDIO_O : out std_logic;
      ENET0_MDIO_T : out std_logic;
      ENET0_PTP_DELAY_REQ_RX : out std_logic;
      ENET0_PTP_DELAY_REQ_TX : out std_logic;
      ENET0_PTP_PDELAY_REQ_RX : out std_logic;
      ENET0_PTP_PDELAY_REQ_TX : out std_logic;
      ENET0_PTP_PDELAY_RESP_RX : out std_logic;
      ENET0_PTP_PDELAY_RESP_TX : out std_logic;
      ENET0_PTP_SYNC_FRAME_RX : out std_logic;
      ENET0_PTP_SYNC_FRAME_TX : out std_logic;
      ENET0_SOF_RX : out std_logic;
      ENET0_SOF_TX : out std_logic;
      ENET0_GMII_TXD : out std_logic_vector(7 downto 0);
      ENET0_GMII_COL : in std_logic;
      ENET0_GMII_CRS : in std_logic;
      ENET0_EXT_INTIN : in std_logic;
      ENET0_GMII_RX_CLK : in std_logic;
      ENET0_GMII_RX_DV : in std_logic;
      ENET0_GMII_RX_ER : in std_logic;
      ENET0_GMII_TX_CLK : in std_logic;
      ENET0_MDIO_I : in std_logic;
      ENET0_GMII_RXD : in std_logic_vector(7 downto 0);
      ENET1_GMII_TX_EN : out std_logic;
      ENET1_GMII_TX_ER : out std_logic;
      ENET1_MDIO_MDC : out std_logic;
      ENET1_MDIO_O : out std_logic;
      ENET1_MDIO_T : out std_logic;
      ENET1_PTP_DELAY_REQ_RX : out std_logic;
      ENET1_PTP_DELAY_REQ_TX : out std_logic;
      ENET1_PTP_PDELAY_REQ_RX : out std_logic;
      ENET1_PTP_PDELAY_REQ_TX : out std_logic;
      ENET1_PTP_PDELAY_RESP_RX : out std_logic;
      ENET1_PTP_PDELAY_RESP_TX : out std_logic;
      ENET1_PTP_SYNC_FRAME_RX : out std_logic;
      ENET1_PTP_SYNC_FRAME_TX : out std_logic;
      ENET1_SOF_RX : out std_logic;
      ENET1_SOF_TX : out std_logic;
      ENET1_GMII_TXD : out std_logic_vector(7 downto 0);
      ENET1_GMII_COL : in std_logic;
      ENET1_GMII_CRS : in std_logic;
      ENET1_EXT_INTIN : in std_logic;
      ENET1_GMII_RX_CLK : in std_logic;
      ENET1_GMII_RX_DV : in std_logic;
      ENET1_GMII_RX_ER : in std_logic;
      ENET1_GMII_TX_CLK : in std_logic;
      ENET1_MDIO_I : in std_logic;
      ENET1_GMII_RXD : in std_logic_vector(7 downto 0);
      GPIO_I : in std_logic_vector(63 downto 0);
      GPIO_O : out std_logic_vector(63 downto 0);
      GPIO_T : out std_logic_vector(63 downto 0);
      I2C0_SDA_I : in std_logic;
      I2C0_SDA_O : out std_logic;
      I2C0_SDA_T : out std_logic;
      I2C0_SCL_I : in std_logic;
      I2C0_SCL_O : out std_logic;
      I2C0_SCL_T : out std_logic;
      I2C1_SDA_I : in std_logic;
      I2C1_SDA_O : out std_logic;
      I2C1_SDA_T : out std_logic;
      I2C1_SCL_I : in std_logic;
      I2C1_SCL_O : out std_logic;
      I2C1_SCL_T : out std_logic;
      PJTAG_TCK : in std_logic;
      PJTAG_TMS : in std_logic;
      PJTAG_TD_I : in std_logic;
      PJTAG_TD_T : out std_logic;
      PJTAG_TD_O : out std_logic;
      SDIO0_CLK : out std_logic;
      SDIO0_CLK_FB : in std_logic;
      SDIO0_CMD_O : out std_logic;
      SDIO0_CMD_I : in std_logic;
      SDIO0_CMD_T : out std_logic;
      SDIO0_DATA_I : in std_logic_vector(3 downto 0);
      SDIO0_DATA_O : out std_logic_vector(3 downto 0);
      SDIO0_DATA_T : out std_logic_vector(3 downto 0);
      SDIO0_LED : out std_logic;
      SDIO0_CDN : in std_logic;
      SDIO0_WP : in std_logic;
      SDIO0_BUSPOW : out std_logic;
      SDIO0_BUSVOLT : out std_logic_vector(2 downto 0);
      SDIO1_CLK : out std_logic;
      SDIO1_CLK_FB : in std_logic;
      SDIO1_CMD_O : out std_logic;
      SDIO1_CMD_I : in std_logic;
      SDIO1_CMD_T : out std_logic;
      SDIO1_DATA_I : in std_logic_vector(3 downto 0);
      SDIO1_DATA_O : out std_logic_vector(3 downto 0);
      SDIO1_DATA_T : out std_logic_vector(3 downto 0);
      SDIO1_LED : out std_logic;
      SDIO1_CDN : in std_logic;
      SDIO1_WP : in std_logic;
      SDIO1_BUSPOW : out std_logic;
      SDIO1_BUSVOLT : out std_logic_vector(2 downto 0);
      SPI0_SCLK_I : in std_logic;
      SPI0_SCLK_O : out std_logic;
      SPI0_SCLK_T : out std_logic;
      SPI0_MOSI_I : in std_logic;
      SPI0_MOSI_O : out std_logic;
      SPI0_MOSI_T : out std_logic;
      SPI0_MISO_I : in std_logic;
      SPI0_MISO_O : out std_logic;
      SPI0_MISO_T : out std_logic;
      SPI0_SS_I : in std_logic;
      SPI0_SS_O : out std_logic;
      SPI0_SS1_O : out std_logic;
      SPI0_SS2_O : out std_logic;
      SPI0_SS_T : out std_logic;
      SPI1_SCLK_I : in std_logic;
      SPI1_SCLK_O : out std_logic;
      SPI1_SCLK_T : out std_logic;
      SPI1_MOSI_I : in std_logic;
      SPI1_MOSI_O : out std_logic;
      SPI1_MOSI_T : out std_logic;
      SPI1_MISO_I : in std_logic;
      SPI1_MISO_O : out std_logic;
      SPI1_MISO_T : out std_logic;
      SPI1_SS_I : in std_logic;
      SPI1_SS_O : out std_logic;
      SPI1_SS1_O : out std_logic;
      SPI1_SS2_O : out std_logic;
      SPI1_SS_T : out std_logic;
      UART0_DTRN : out std_logic;
      UART0_RTSN : out std_logic;
      UART0_TX : out std_logic;
      UART0_CTSN : in std_logic;
      UART0_DCDN : in std_logic;
      UART0_DSRN : in std_logic;
      UART0_RIN : in std_logic;
      UART0_RX : in std_logic;
      UART1_DTRN : out std_logic;
      UART1_RTSN : out std_logic;
      UART1_TX : out std_logic;
      UART1_CTSN : in std_logic;
      UART1_DCDN : in std_logic;
      UART1_DSRN : in std_logic;
      UART1_RIN : in std_logic;
      UART1_RX : in std_logic;
      TTC0_WAVE0_OUT : out std_logic;
      TTC0_WAVE1_OUT : out std_logic;
      TTC0_WAVE2_OUT : out std_logic;
      TTC0_CLK0_IN : in std_logic;
      TTC0_CLK1_IN : in std_logic;
      TTC0_CLK2_IN : in std_logic;
      TTC1_WAVE0_OUT : out std_logic;
      TTC1_WAVE1_OUT : out std_logic;
      TTC1_WAVE2_OUT : out std_logic;
      TTC1_CLK0_IN : in std_logic;
      TTC1_CLK1_IN : in std_logic;
      TTC1_CLK2_IN : in std_logic;
      WDT_CLK_IN : in std_logic;
      WDT_RST_OUT : out std_logic;
      TRACE_CLK : in std_logic;
      TRACE_CTL : out std_logic;
      TRACE_DATA : out std_logic_vector(31 downto 0);
      USB0_PORT_INDCTL : out std_logic_vector(1 downto 0);
      USB1_PORT_INDCTL : out std_logic_vector(1 downto 0);
      USB0_VBUS_PWRSELECT : out std_logic;
      USB1_VBUS_PWRSELECT : out std_logic;
      USB0_VBUS_PWRFAULT : in std_logic;
      USB1_VBUS_PWRFAULT : in std_logic;
      SRAM_INTIN : in std_logic;
      M_AXI_GP0_ARESETN : out std_logic;
      M_AXI_GP0_ARVALID : out std_logic;
      M_AXI_GP0_AWVALID : out std_logic;
      M_AXI_GP0_BREADY : out std_logic;
      M_AXI_GP0_RREADY : out std_logic;
      M_AXI_GP0_WLAST : out std_logic;
      M_AXI_GP0_WVALID : out std_logic;
      M_AXI_GP0_ARID : out std_logic_vector(11 downto 0);
      M_AXI_GP0_AWID : out std_logic_vector(11 downto 0);
      M_AXI_GP0_WID : out std_logic_vector(11 downto 0);
      M_AXI_GP0_ARBURST : out std_logic_vector(1 downto 0);
      M_AXI_GP0_ARLOCK : out std_logic_vector(1 downto 0);
      M_AXI_GP0_ARSIZE : out std_logic_vector(2 downto 0);
      M_AXI_GP0_AWBURST : out std_logic_vector(1 downto 0);
      M_AXI_GP0_AWLOCK : out std_logic_vector(1 downto 0);
      M_AXI_GP0_AWSIZE : out std_logic_vector(2 downto 0);
      M_AXI_GP0_ARPROT : out std_logic_vector(2 downto 0);
      M_AXI_GP0_AWPROT : out std_logic_vector(2 downto 0);
      M_AXI_GP0_ARADDR : out std_logic_vector(31 downto 0);
      M_AXI_GP0_AWADDR : out std_logic_vector(31 downto 0);
      M_AXI_GP0_WDATA : out std_logic_vector(31 downto 0);
      M_AXI_GP0_ARCACHE : out std_logic_vector(3 downto 0);
      M_AXI_GP0_ARLEN : out std_logic_vector(3 downto 0);
      M_AXI_GP0_ARQOS : out std_logic_vector(3 downto 0);
      M_AXI_GP0_AWCACHE : out std_logic_vector(3 downto 0);
      M_AXI_GP0_AWLEN : out std_logic_vector(3 downto 0);
      M_AXI_GP0_AWQOS : out std_logic_vector(3 downto 0);
      M_AXI_GP0_WSTRB : out std_logic_vector(3 downto 0);
      M_AXI_GP0_ACLK : in std_logic;
      M_AXI_GP0_ARREADY : in std_logic;
      M_AXI_GP0_AWREADY : in std_logic;
      M_AXI_GP0_BVALID : in std_logic;
      M_AXI_GP0_RLAST : in std_logic;
      M_AXI_GP0_RVALID : in std_logic;
      M_AXI_GP0_WREADY : in std_logic;
      M_AXI_GP0_BID : in std_logic_vector(11 downto 0);
      M_AXI_GP0_RID : in std_logic_vector(11 downto 0);
      M_AXI_GP0_BRESP : in std_logic_vector(1 downto 0);
      M_AXI_GP0_RRESP : in std_logic_vector(1 downto 0);
      M_AXI_GP0_RDATA : in std_logic_vector(31 downto 0);
      M_AXI_GP1_ARESETN : out std_logic;
      M_AXI_GP1_ARVALID : out std_logic;
      M_AXI_GP1_AWVALID : out std_logic;
      M_AXI_GP1_BREADY : out std_logic;
      M_AXI_GP1_RREADY : out std_logic;
      M_AXI_GP1_WLAST : out std_logic;
      M_AXI_GP1_WVALID : out std_logic;
      M_AXI_GP1_ARID : out std_logic_vector(11 downto 0);
      M_AXI_GP1_AWID : out std_logic_vector(11 downto 0);
      M_AXI_GP1_WID : out std_logic_vector(11 downto 0);
      M_AXI_GP1_ARBURST : out std_logic_vector(1 downto 0);
      M_AXI_GP1_ARLOCK : out std_logic_vector(1 downto 0);
      M_AXI_GP1_ARSIZE : out std_logic_vector(2 downto 0);
      M_AXI_GP1_AWBURST : out std_logic_vector(1 downto 0);
      M_AXI_GP1_AWLOCK : out std_logic_vector(1 downto 0);
      M_AXI_GP1_AWSIZE : out std_logic_vector(2 downto 0);
      M_AXI_GP1_ARPROT : out std_logic_vector(2 downto 0);
      M_AXI_GP1_AWPROT : out std_logic_vector(2 downto 0);
      M_AXI_GP1_ARADDR : out std_logic_vector(31 downto 0);
      M_AXI_GP1_AWADDR : out std_logic_vector(31 downto 0);
      M_AXI_GP1_WDATA : out std_logic_vector(31 downto 0);
      M_AXI_GP1_ARCACHE : out std_logic_vector(3 downto 0);
      M_AXI_GP1_ARLEN : out std_logic_vector(3 downto 0);
      M_AXI_GP1_ARQOS : out std_logic_vector(3 downto 0);
      M_AXI_GP1_AWCACHE : out std_logic_vector(3 downto 0);
      M_AXI_GP1_AWLEN : out std_logic_vector(3 downto 0);
      M_AXI_GP1_AWQOS : out std_logic_vector(3 downto 0);
      M_AXI_GP1_WSTRB : out std_logic_vector(3 downto 0);
      M_AXI_GP1_ACLK : in std_logic;
      M_AXI_GP1_ARREADY : in std_logic;
      M_AXI_GP1_AWREADY : in std_logic;
      M_AXI_GP1_BVALID : in std_logic;
      M_AXI_GP1_RLAST : in std_logic;
      M_AXI_GP1_RVALID : in std_logic;
      M_AXI_GP1_WREADY : in std_logic;
      M_AXI_GP1_BID : in std_logic_vector(11 downto 0);
      M_AXI_GP1_RID : in std_logic_vector(11 downto 0);
      M_AXI_GP1_BRESP : in std_logic_vector(1 downto 0);
      M_AXI_GP1_RRESP : in std_logic_vector(1 downto 0);
      M_AXI_GP1_RDATA : in std_logic_vector(31 downto 0);
      S_AXI_GP0_ARESETN : out std_logic;
      S_AXI_GP0_ARREADY : out std_logic;
      S_AXI_GP0_AWREADY : out std_logic;
      S_AXI_GP0_BVALID : out std_logic;
      S_AXI_GP0_RLAST : out std_logic;
      S_AXI_GP0_RVALID : out std_logic;
      S_AXI_GP0_WREADY : out std_logic;
      S_AXI_GP0_BRESP : out std_logic_vector(1 downto 0);
      S_AXI_GP0_RRESP : out std_logic_vector(1 downto 0);
      S_AXI_GP0_RDATA : out std_logic_vector(31 downto 0);
      S_AXI_GP0_BID : out std_logic_vector(5 downto 0);
      S_AXI_GP0_RID : out std_logic_vector(5 downto 0);
      S_AXI_GP0_ACLK : in std_logic;
      S_AXI_GP0_ARVALID : in std_logic;
      S_AXI_GP0_AWVALID : in std_logic;
      S_AXI_GP0_BREADY : in std_logic;
      S_AXI_GP0_RREADY : in std_logic;
      S_AXI_GP0_WLAST : in std_logic;
      S_AXI_GP0_WVALID : in std_logic;
      S_AXI_GP0_ARBURST : in std_logic_vector(1 downto 0);
      S_AXI_GP0_ARLOCK : in std_logic_vector(1 downto 0);
      S_AXI_GP0_ARSIZE : in std_logic_vector(2 downto 0);
      S_AXI_GP0_AWBURST : in std_logic_vector(1 downto 0);
      S_AXI_GP0_AWLOCK : in std_logic_vector(1 downto 0);
      S_AXI_GP0_AWSIZE : in std_logic_vector(2 downto 0);
      S_AXI_GP0_ARPROT : in std_logic_vector(2 downto 0);
      S_AXI_GP0_AWPROT : in std_logic_vector(2 downto 0);
      S_AXI_GP0_ARADDR : in std_logic_vector(31 downto 0);
      S_AXI_GP0_AWADDR : in std_logic_vector(31 downto 0);
      S_AXI_GP0_WDATA : in std_logic_vector(31 downto 0);
      S_AXI_GP0_ARCACHE : in std_logic_vector(3 downto 0);
      S_AXI_GP0_ARLEN : in std_logic_vector(3 downto 0);
      S_AXI_GP0_ARQOS : in std_logic_vector(3 downto 0);
      S_AXI_GP0_AWCACHE : in std_logic_vector(3 downto 0);
      S_AXI_GP0_AWLEN : in std_logic_vector(3 downto 0);
      S_AXI_GP0_AWQOS : in std_logic_vector(3 downto 0);
      S_AXI_GP0_WSTRB : in std_logic_vector(3 downto 0);
      S_AXI_GP0_ARID : in std_logic_vector(5 downto 0);
      S_AXI_GP0_AWID : in std_logic_vector(5 downto 0);
      S_AXI_GP0_WID : in std_logic_vector(5 downto 0);
      S_AXI_GP1_ARESETN : out std_logic;
      S_AXI_GP1_ARREADY : out std_logic;
      S_AXI_GP1_AWREADY : out std_logic;
      S_AXI_GP1_BVALID : out std_logic;
      S_AXI_GP1_RLAST : out std_logic;
      S_AXI_GP1_RVALID : out std_logic;
      S_AXI_GP1_WREADY : out std_logic;
      S_AXI_GP1_BRESP : out std_logic_vector(1 downto 0);
      S_AXI_GP1_RRESP : out std_logic_vector(1 downto 0);
      S_AXI_GP1_RDATA : out std_logic_vector(31 downto 0);
      S_AXI_GP1_BID : out std_logic_vector(5 downto 0);
      S_AXI_GP1_RID : out std_logic_vector(5 downto 0);
      S_AXI_GP1_ACLK : in std_logic;
      S_AXI_GP1_ARVALID : in std_logic;
      S_AXI_GP1_AWVALID : in std_logic;
      S_AXI_GP1_BREADY : in std_logic;
      S_AXI_GP1_RREADY : in std_logic;
      S_AXI_GP1_WLAST : in std_logic;
      S_AXI_GP1_WVALID : in std_logic;
      S_AXI_GP1_ARBURST : in std_logic_vector(1 downto 0);
      S_AXI_GP1_ARLOCK : in std_logic_vector(1 downto 0);
      S_AXI_GP1_ARSIZE : in std_logic_vector(2 downto 0);
      S_AXI_GP1_AWBURST : in std_logic_vector(1 downto 0);
      S_AXI_GP1_AWLOCK : in std_logic_vector(1 downto 0);
      S_AXI_GP1_AWSIZE : in std_logic_vector(2 downto 0);
      S_AXI_GP1_ARPROT : in std_logic_vector(2 downto 0);
      S_AXI_GP1_AWPROT : in std_logic_vector(2 downto 0);
      S_AXI_GP1_ARADDR : in std_logic_vector(31 downto 0);
      S_AXI_GP1_AWADDR : in std_logic_vector(31 downto 0);
      S_AXI_GP1_WDATA : in std_logic_vector(31 downto 0);
      S_AXI_GP1_ARCACHE : in std_logic_vector(3 downto 0);
      S_AXI_GP1_ARLEN : in std_logic_vector(3 downto 0);
      S_AXI_GP1_ARQOS : in std_logic_vector(3 downto 0);
      S_AXI_GP1_AWCACHE : in std_logic_vector(3 downto 0);
      S_AXI_GP1_AWLEN : in std_logic_vector(3 downto 0);
      S_AXI_GP1_AWQOS : in std_logic_vector(3 downto 0);
      S_AXI_GP1_WSTRB : in std_logic_vector(3 downto 0);
      S_AXI_GP1_ARID : in std_logic_vector(5 downto 0);
      S_AXI_GP1_AWID : in std_logic_vector(5 downto 0);
      S_AXI_GP1_WID : in std_logic_vector(5 downto 0);
      S_AXI_ACP_ARESETN : out std_logic;
      S_AXI_ACP_AWREADY : out std_logic;
      S_AXI_ACP_ARREADY : out std_logic;
      S_AXI_ACP_BVALID : out std_logic;
      S_AXI_ACP_RLAST : out std_logic;
      S_AXI_ACP_RVALID : out std_logic;
      S_AXI_ACP_WREADY : out std_logic;
      S_AXI_ACP_BRESP : out std_logic_vector(1 downto 0);
      S_AXI_ACP_RRESP : out std_logic_vector(1 downto 0);
      S_AXI_ACP_BID : out std_logic_vector(2 downto 0);
      S_AXI_ACP_RID : out std_logic_vector(2 downto 0);
      S_AXI_ACP_RDATA : out std_logic_vector(63 downto 0);
      S_AXI_ACP_ACLK : in std_logic;
      S_AXI_ACP_ARVALID : in std_logic;
      S_AXI_ACP_AWVALID : in std_logic;
      S_AXI_ACP_BREADY : in std_logic;
      S_AXI_ACP_RREADY : in std_logic;
      S_AXI_ACP_WLAST : in std_logic;
      S_AXI_ACP_WVALID : in std_logic;
      S_AXI_ACP_ARID : in std_logic_vector(2 downto 0);
      S_AXI_ACP_ARPROT : in std_logic_vector(2 downto 0);
      S_AXI_ACP_AWID : in std_logic_vector(2 downto 0);
      S_AXI_ACP_AWPROT : in std_logic_vector(2 downto 0);
      S_AXI_ACP_WID : in std_logic_vector(2 downto 0);
      S_AXI_ACP_ARADDR : in std_logic_vector(31 downto 0);
      S_AXI_ACP_AWADDR : in std_logic_vector(31 downto 0);
      S_AXI_ACP_ARCACHE : in std_logic_vector(3 downto 0);
      S_AXI_ACP_ARLEN : in std_logic_vector(3 downto 0);
      S_AXI_ACP_ARQOS : in std_logic_vector(3 downto 0);
      S_AXI_ACP_AWCACHE : in std_logic_vector(3 downto 0);
      S_AXI_ACP_AWLEN : in std_logic_vector(3 downto 0);
      S_AXI_ACP_AWQOS : in std_logic_vector(3 downto 0);
      S_AXI_ACP_ARBURST : in std_logic_vector(1 downto 0);
      S_AXI_ACP_ARLOCK : in std_logic_vector(1 downto 0);
      S_AXI_ACP_ARSIZE : in std_logic_vector(2 downto 0);
      S_AXI_ACP_AWBURST : in std_logic_vector(1 downto 0);
      S_AXI_ACP_AWLOCK : in std_logic_vector(1 downto 0);
      S_AXI_ACP_AWSIZE : in std_logic_vector(2 downto 0);
      S_AXI_ACP_ARUSER : in std_logic_vector(4 downto 0);
      S_AXI_ACP_AWUSER : in std_logic_vector(4 downto 0);
      S_AXI_ACP_WDATA : in std_logic_vector(63 downto 0);
      S_AXI_ACP_WSTRB : in std_logic_vector(7 downto 0);
      S_AXI_HP0_ARESETN : out std_logic;
      S_AXI_HP0_ARREADY : out std_logic;
      S_AXI_HP0_AWREADY : out std_logic;
      S_AXI_HP0_BVALID : out std_logic;
      S_AXI_HP0_RLAST : out std_logic;
      S_AXI_HP0_RVALID : out std_logic;
      S_AXI_HP0_WREADY : out std_logic;
      S_AXI_HP0_BRESP : out std_logic_vector(1 downto 0);
      S_AXI_HP0_RRESP : out std_logic_vector(1 downto 0);
      S_AXI_HP0_BID : out std_logic_vector(0 to 0);
      S_AXI_HP0_RID : out std_logic_vector(0 to 0);
      S_AXI_HP0_RDATA : out std_logic_vector(63 downto 0);
      S_AXI_HP0_RCOUNT : out std_logic_vector(7 downto 0);
      S_AXI_HP0_WCOUNT : out std_logic_vector(7 downto 0);
      S_AXI_HP0_RACOUNT : out std_logic_vector(2 downto 0);
      S_AXI_HP0_WACOUNT : out std_logic_vector(5 downto 0);
      S_AXI_HP0_ACLK : in std_logic;
      S_AXI_HP0_ARVALID : in std_logic;
      S_AXI_HP0_AWVALID : in std_logic;
      S_AXI_HP0_BREADY : in std_logic;
      S_AXI_HP0_RDISSUECAP1_EN : in std_logic;
      S_AXI_HP0_RREADY : in std_logic;
      S_AXI_HP0_WLAST : in std_logic;
      S_AXI_HP0_WRISSUECAP1_EN : in std_logic;
      S_AXI_HP0_WVALID : in std_logic;
      S_AXI_HP0_ARBURST : in std_logic_vector(1 downto 0);
      S_AXI_HP0_ARLOCK : in std_logic_vector(1 downto 0);
      S_AXI_HP0_ARSIZE : in std_logic_vector(2 downto 0);
      S_AXI_HP0_AWBURST : in std_logic_vector(1 downto 0);
      S_AXI_HP0_AWLOCK : in std_logic_vector(1 downto 0);
      S_AXI_HP0_AWSIZE : in std_logic_vector(2 downto 0);
      S_AXI_HP0_ARPROT : in std_logic_vector(2 downto 0);
      S_AXI_HP0_AWPROT : in std_logic_vector(2 downto 0);
      S_AXI_HP0_ARADDR : in std_logic_vector(31 downto 0);
      S_AXI_HP0_AWADDR : in std_logic_vector(31 downto 0);
      S_AXI_HP0_ARCACHE : in std_logic_vector(3 downto 0);
      S_AXI_HP0_ARLEN : in std_logic_vector(3 downto 0);
      S_AXI_HP0_ARQOS : in std_logic_vector(3 downto 0);
      S_AXI_HP0_AWCACHE : in std_logic_vector(3 downto 0);
      S_AXI_HP0_AWLEN : in std_logic_vector(3 downto 0);
      S_AXI_HP0_AWQOS : in std_logic_vector(3 downto 0);
      S_AXI_HP0_ARID : in std_logic_vector(0 to 0);
      S_AXI_HP0_AWID : in std_logic_vector(0 to 0);
      S_AXI_HP0_WID : in std_logic_vector(0 to 0);
      S_AXI_HP0_WDATA : in std_logic_vector(63 downto 0);
      S_AXI_HP0_WSTRB : in std_logic_vector(7 downto 0);
      S_AXI_HP1_ARESETN : out std_logic;
      S_AXI_HP1_ARREADY : out std_logic;
      S_AXI_HP1_AWREADY : out std_logic;
      S_AXI_HP1_BVALID : out std_logic;
      S_AXI_HP1_RLAST : out std_logic;
      S_AXI_HP1_RVALID : out std_logic;
      S_AXI_HP1_WREADY : out std_logic;
      S_AXI_HP1_BRESP : out std_logic_vector(1 downto 0);
      S_AXI_HP1_RRESP : out std_logic_vector(1 downto 0);
      S_AXI_HP1_BID : out std_logic_vector(5 downto 0);
      S_AXI_HP1_RID : out std_logic_vector(5 downto 0);
      S_AXI_HP1_RDATA : out std_logic_vector(63 downto 0);
      S_AXI_HP1_RCOUNT : out std_logic_vector(7 downto 0);
      S_AXI_HP1_WCOUNT : out std_logic_vector(7 downto 0);
      S_AXI_HP1_RACOUNT : out std_logic_vector(2 downto 0);
      S_AXI_HP1_WACOUNT : out std_logic_vector(5 downto 0);
      S_AXI_HP1_ACLK : in std_logic;
      S_AXI_HP1_ARVALID : in std_logic;
      S_AXI_HP1_AWVALID : in std_logic;
      S_AXI_HP1_BREADY : in std_logic;
      S_AXI_HP1_RDISSUECAP1_EN : in std_logic;
      S_AXI_HP1_RREADY : in std_logic;
      S_AXI_HP1_WLAST : in std_logic;
      S_AXI_HP1_WRISSUECAP1_EN : in std_logic;
      S_AXI_HP1_WVALID : in std_logic;
      S_AXI_HP1_ARBURST : in std_logic_vector(1 downto 0);
      S_AXI_HP1_ARLOCK : in std_logic_vector(1 downto 0);
      S_AXI_HP1_ARSIZE : in std_logic_vector(2 downto 0);
      S_AXI_HP1_AWBURST : in std_logic_vector(1 downto 0);
      S_AXI_HP1_AWLOCK : in std_logic_vector(1 downto 0);
      S_AXI_HP1_AWSIZE : in std_logic_vector(2 downto 0);
      S_AXI_HP1_ARPROT : in std_logic_vector(2 downto 0);
      S_AXI_HP1_AWPROT : in std_logic_vector(2 downto 0);
      S_AXI_HP1_ARADDR : in std_logic_vector(31 downto 0);
      S_AXI_HP1_AWADDR : in std_logic_vector(31 downto 0);
      S_AXI_HP1_ARCACHE : in std_logic_vector(3 downto 0);
      S_AXI_HP1_ARLEN : in std_logic_vector(3 downto 0);
      S_AXI_HP1_ARQOS : in std_logic_vector(3 downto 0);
      S_AXI_HP1_AWCACHE : in std_logic_vector(3 downto 0);
      S_AXI_HP1_AWLEN : in std_logic_vector(3 downto 0);
      S_AXI_HP1_AWQOS : in std_logic_vector(3 downto 0);
      S_AXI_HP1_ARID : in std_logic_vector(5 downto 0);
      S_AXI_HP1_AWID : in std_logic_vector(5 downto 0);
      S_AXI_HP1_WID : in std_logic_vector(5 downto 0);
      S_AXI_HP1_WDATA : in std_logic_vector(63 downto 0);
      S_AXI_HP1_WSTRB : in std_logic_vector(7 downto 0);
      S_AXI_HP2_ARESETN : out std_logic;
      S_AXI_HP2_ARREADY : out std_logic;
      S_AXI_HP2_AWREADY : out std_logic;
      S_AXI_HP2_BVALID : out std_logic;
      S_AXI_HP2_RLAST : out std_logic;
      S_AXI_HP2_RVALID : out std_logic;
      S_AXI_HP2_WREADY : out std_logic;
      S_AXI_HP2_BRESP : out std_logic_vector(1 downto 0);
      S_AXI_HP2_RRESP : out std_logic_vector(1 downto 0);
      S_AXI_HP2_BID : out std_logic_vector(5 downto 0);
      S_AXI_HP2_RID : out std_logic_vector(5 downto 0);
      S_AXI_HP2_RDATA : out std_logic_vector(63 downto 0);
      S_AXI_HP2_RCOUNT : out std_logic_vector(7 downto 0);
      S_AXI_HP2_WCOUNT : out std_logic_vector(7 downto 0);
      S_AXI_HP2_RACOUNT : out std_logic_vector(2 downto 0);
      S_AXI_HP2_WACOUNT : out std_logic_vector(5 downto 0);
      S_AXI_HP2_ACLK : in std_logic;
      S_AXI_HP2_ARVALID : in std_logic;
      S_AXI_HP2_AWVALID : in std_logic;
      S_AXI_HP2_BREADY : in std_logic;
      S_AXI_HP2_RDISSUECAP1_EN : in std_logic;
      S_AXI_HP2_RREADY : in std_logic;
      S_AXI_HP2_WLAST : in std_logic;
      S_AXI_HP2_WRISSUECAP1_EN : in std_logic;
      S_AXI_HP2_WVALID : in std_logic;
      S_AXI_HP2_ARBURST : in std_logic_vector(1 downto 0);
      S_AXI_HP2_ARLOCK : in std_logic_vector(1 downto 0);
      S_AXI_HP2_ARSIZE : in std_logic_vector(2 downto 0);
      S_AXI_HP2_AWBURST : in std_logic_vector(1 downto 0);
      S_AXI_HP2_AWLOCK : in std_logic_vector(1 downto 0);
      S_AXI_HP2_AWSIZE : in std_logic_vector(2 downto 0);
      S_AXI_HP2_ARPROT : in std_logic_vector(2 downto 0);
      S_AXI_HP2_AWPROT : in std_logic_vector(2 downto 0);
      S_AXI_HP2_ARADDR : in std_logic_vector(31 downto 0);
      S_AXI_HP2_AWADDR : in std_logic_vector(31 downto 0);
      S_AXI_HP2_ARCACHE : in std_logic_vector(3 downto 0);
      S_AXI_HP2_ARLEN : in std_logic_vector(3 downto 0);
      S_AXI_HP2_ARQOS : in std_logic_vector(3 downto 0);
      S_AXI_HP2_AWCACHE : in std_logic_vector(3 downto 0);
      S_AXI_HP2_AWLEN : in std_logic_vector(3 downto 0);
      S_AXI_HP2_AWQOS : in std_logic_vector(3 downto 0);
      S_AXI_HP2_ARID : in std_logic_vector(5 downto 0);
      S_AXI_HP2_AWID : in std_logic_vector(5 downto 0);
      S_AXI_HP2_WID : in std_logic_vector(5 downto 0);
      S_AXI_HP2_WDATA : in std_logic_vector(63 downto 0);
      S_AXI_HP2_WSTRB : in std_logic_vector(7 downto 0);
      S_AXI_HP3_ARESETN : out std_logic;
      S_AXI_HP3_ARREADY : out std_logic;
      S_AXI_HP3_AWREADY : out std_logic;
      S_AXI_HP3_BVALID : out std_logic;
      S_AXI_HP3_RLAST : out std_logic;
      S_AXI_HP3_RVALID : out std_logic;
      S_AXI_HP3_WREADY : out std_logic;
      S_AXI_HP3_BRESP : out std_logic_vector(1 downto 0);
      S_AXI_HP3_RRESP : out std_logic_vector(1 downto 0);
      S_AXI_HP3_BID : out std_logic_vector(5 downto 0);
      S_AXI_HP3_RID : out std_logic_vector(5 downto 0);
      S_AXI_HP3_RDATA : out std_logic_vector(63 downto 0);
      S_AXI_HP3_RCOUNT : out std_logic_vector(7 downto 0);
      S_AXI_HP3_WCOUNT : out std_logic_vector(7 downto 0);
      S_AXI_HP3_RACOUNT : out std_logic_vector(2 downto 0);
      S_AXI_HP3_WACOUNT : out std_logic_vector(5 downto 0);
      S_AXI_HP3_ACLK : in std_logic;
      S_AXI_HP3_ARVALID : in std_logic;
      S_AXI_HP3_AWVALID : in std_logic;
      S_AXI_HP3_BREADY : in std_logic;
      S_AXI_HP3_RDISSUECAP1_EN : in std_logic;
      S_AXI_HP3_RREADY : in std_logic;
      S_AXI_HP3_WLAST : in std_logic;
      S_AXI_HP3_WRISSUECAP1_EN : in std_logic;
      S_AXI_HP3_WVALID : in std_logic;
      S_AXI_HP3_ARBURST : in std_logic_vector(1 downto 0);
      S_AXI_HP3_ARLOCK : in std_logic_vector(1 downto 0);
      S_AXI_HP3_ARSIZE : in std_logic_vector(2 downto 0);
      S_AXI_HP3_AWBURST : in std_logic_vector(1 downto 0);
      S_AXI_HP3_AWLOCK : in std_logic_vector(1 downto 0);
      S_AXI_HP3_AWSIZE : in std_logic_vector(2 downto 0);
      S_AXI_HP3_ARPROT : in std_logic_vector(2 downto 0);
      S_AXI_HP3_AWPROT : in std_logic_vector(2 downto 0);
      S_AXI_HP3_ARADDR : in std_logic_vector(31 downto 0);
      S_AXI_HP3_AWADDR : in std_logic_vector(31 downto 0);
      S_AXI_HP3_ARCACHE : in std_logic_vector(3 downto 0);
      S_AXI_HP3_ARLEN : in std_logic_vector(3 downto 0);
      S_AXI_HP3_ARQOS : in std_logic_vector(3 downto 0);
      S_AXI_HP3_AWCACHE : in std_logic_vector(3 downto 0);
      S_AXI_HP3_AWLEN : in std_logic_vector(3 downto 0);
      S_AXI_HP3_AWQOS : in std_logic_vector(3 downto 0);
      S_AXI_HP3_ARID : in std_logic_vector(5 downto 0);
      S_AXI_HP3_AWID : in std_logic_vector(5 downto 0);
      S_AXI_HP3_WID : in std_logic_vector(5 downto 0);
      S_AXI_HP3_WDATA : in std_logic_vector(63 downto 0);
      S_AXI_HP3_WSTRB : in std_logic_vector(7 downto 0);
      DMA0_DATYPE : out std_logic_vector(1 downto 0);
      DMA0_DAVALID : out std_logic;
      DMA0_DRREADY : out std_logic;
      DMA0_RSTN : out std_logic;
      DMA0_ACLK : in std_logic;
      DMA0_DAREADY : in std_logic;
      DMA0_DRLAST : in std_logic;
      DMA0_DRVALID : in std_logic;
      DMA0_DRTYPE : in std_logic_vector(1 downto 0);
      DMA1_DATYPE : out std_logic_vector(1 downto 0);
      DMA1_DAVALID : out std_logic;
      DMA1_DRREADY : out std_logic;
      DMA1_RSTN : out std_logic;
      DMA1_ACLK : in std_logic;
      DMA1_DAREADY : in std_logic;
      DMA1_DRLAST : in std_logic;
      DMA1_DRVALID : in std_logic;
      DMA1_DRTYPE : in std_logic_vector(1 downto 0);
      DMA2_DATYPE : out std_logic_vector(1 downto 0);
      DMA2_DAVALID : out std_logic;
      DMA2_DRREADY : out std_logic;
      DMA2_RSTN : out std_logic;
      DMA2_ACLK : in std_logic;
      DMA2_DAREADY : in std_logic;
      DMA2_DRLAST : in std_logic;
      DMA2_DRVALID : in std_logic;
      DMA3_DRVALID : in std_logic;
      DMA3_DATYPE : out std_logic_vector(1 downto 0);
      DMA3_DAVALID : out std_logic;
      DMA3_DRREADY : out std_logic;
      DMA3_RSTN : out std_logic;
      DMA3_ACLK : in std_logic;
      DMA3_DAREADY : in std_logic;
      DMA3_DRLAST : in std_logic;
      DMA2_DRTYPE : in std_logic_vector(1 downto 0);
      DMA3_DRTYPE : in std_logic_vector(1 downto 0);
      FTMD_TRACEIN_DATA : in std_logic_vector(31 downto 0);
      FTMD_TRACEIN_VALID : in std_logic;
      FTMD_TRACEIN_CLK : in std_logic;
      FTMD_TRACEIN_ATID : in std_logic_vector(3 downto 0);
      FTMT_F2P_TRIG : in std_logic_vector(3 downto 0);
      FTMT_F2P_TRIGACK : out std_logic_vector(3 downto 0);
      FTMT_F2P_DEBUG : in std_logic_vector(31 downto 0);
      FTMT_P2F_TRIGACK : in std_logic_vector(3 downto 0);
      FTMT_P2F_TRIG : out std_logic_vector(3 downto 0);
      FTMT_P2F_DEBUG : out std_logic_vector(31 downto 0);
      FCLK_CLK3 : out std_logic;
      FCLK_CLK2 : out std_logic;
      FCLK_CLK1 : out std_logic;
      FCLK_CLK0 : out std_logic;
      FCLK_CLKTRIG3_N : in std_logic;
      FCLK_CLKTRIG2_N : in std_logic;
      FCLK_CLKTRIG1_N : in std_logic;
      FCLK_CLKTRIG0_N : in std_logic;
      FCLK_RESET3_N : out std_logic;
      FCLK_RESET2_N : out std_logic;
      FCLK_RESET1_N : out std_logic;
      FCLK_RESET0_N : out std_logic;
      FPGA_IDLE_N : in std_logic;
      DDR_ARB : in std_logic_vector(3 downto 0);
      IRQ_F2P : in std_logic_vector(15 downto 0);
      Core0_nFIQ : in std_logic;
      Core0_nIRQ : in std_logic;
      Core1_nFIQ : in std_logic;
      Core1_nIRQ : in std_logic;
      EVENT_EVENTO : out std_logic;
      EVENT_STANDBYWFE : out std_logic_vector(1 downto 0);
      EVENT_STANDBYWFI : out std_logic_vector(1 downto 0);
      EVENT_EVENTI : in std_logic;
      MIO : inout std_logic_vector(53 downto 0);
      DDR_Clk : inout std_logic;
      DDR_Clk_n : inout std_logic;
      DDR_CKE : inout std_logic;
      DDR_CS_n : inout std_logic;
      DDR_RAS_n : inout std_logic;
      DDR_WEB : inout std_logic;
      DDR_CAS_n : inout std_logic;
      DDR_BankAddr : inout std_logic_vector(2 downto 0);
      DDR_Addr : inout std_logic_vector(14 downto 0);
      DDR_ODT : inout std_logic;
      DDR_DRSTB : inout std_logic;
      DDR_DQ : inout std_logic_vector(31 downto 0);
      DDR_DM : inout std_logic_vector(3 downto 0);
      DDR_DQS : inout std_logic_vector(3 downto 0);
      DDR_DQS_n : inout std_logic_vector(3 downto 0);
      DDR_VRN : inout std_logic;
      DDR_VRP : inout std_logic;
      PS_SRSTB : in std_logic;
      PS_CLK : in std_logic;
      PS_PORB : in std_logic;
      IRQ_P2F_DMAC_ABORT : out std_logic;
      IRQ_P2F_DMAC0 : out std_logic;
      IRQ_P2F_DMAC1 : out std_logic;
      IRQ_P2F_DMAC2 : out std_logic;
      IRQ_P2F_DMAC3 : out std_logic;
      IRQ_P2F_DMAC4 : out std_logic;
      IRQ_P2F_DMAC5 : out std_logic;
      IRQ_P2F_DMAC6 : out std_logic;
      IRQ_P2F_DMAC7 : out std_logic;
      IRQ_P2F_SMC : out std_logic;
      IRQ_P2F_QSPI : out std_logic;
      IRQ_P2F_CTI : out std_logic;
      IRQ_P2F_GPIO : out std_logic;
      IRQ_P2F_USB0 : out std_logic;
      IRQ_P2F_ENET0 : out std_logic;
      IRQ_P2F_ENET_WAKE0 : out std_logic;
      IRQ_P2F_SDIO0 : out std_logic;
      IRQ_P2F_I2C0 : out std_logic;
      IRQ_P2F_SPI0 : out std_logic;
      IRQ_P2F_UART0 : out std_logic;
      IRQ_P2F_CAN0 : out std_logic;
      IRQ_P2F_USB1 : out std_logic;
      IRQ_P2F_ENET1 : out std_logic;
      IRQ_P2F_ENET_WAKE1 : out std_logic;
      IRQ_P2F_SDIO1 : out std_logic;
      IRQ_P2F_I2C1 : out std_logic;
      IRQ_P2F_SPI1 : out std_logic;
      IRQ_P2F_UART1 : out std_logic;
      IRQ_P2F_CAN1 : out std_logic
    );
  end component;
begin
  ps : processing_system7
    generic map(
      C_EN_EMIO_ENET0 => 1,
      C_EN_EMIO_ENET1 => 0,
      C_EN_EMIO_TRACE => 0,
      C_INCLUDE_TRACE_BUFFER => 0,
      C_TRACE_BUFFER_FIFO_SIZE => 128,
      USE_TRACE_DATA_EDGE_DETECTOR => 0,
      C_TRACE_BUFFER_CLOCK_DELAY => 12,
      C_EMIO_GPIO_WIDTH => 64,
      C_INCLUDE_ACP_TRANS_CHECK => 0,
      C_USE_DEFAULT_ACP_USER_VAL => 0,
      C_S_AXI_ACP_ARUSER_VAL => 31,
      C_S_AXI_ACP_AWUSER_VAL => 31,
      C_PS7_SI_REV => "PRODUCTION",
      C_M_AXI_GP0_ID_WIDTH => 12,
      C_M_AXI_GP0_ENABLE_STATIC_REMAP => 0,
      C_M_AXI_GP1_ID_WIDTH => 12,
      C_M_AXI_GP1_ENABLE_STATIC_REMAP => 0,
      C_S_AXI_GP0_ID_WIDTH => 6,
      C_S_AXI_GP1_ID_WIDTH => 6,
      C_S_AXI_ACP_ID_WIDTH => 3,
      C_S_AXI_HP0_ID_WIDTH => 1,
      C_S_AXI_HP0_DATA_WIDTH => 64,
      C_S_AXI_HP1_ID_WIDTH => 6,
      C_S_AXI_HP1_DATA_WIDTH => 64,
      C_S_AXI_HP2_ID_WIDTH => 6,
      C_S_AXI_HP2_DATA_WIDTH => 64,
      C_S_AXI_HP3_ID_WIDTH => 6,
      C_S_AXI_HP3_DATA_WIDTH => 64,
      C_M_AXI_GP0_THREAD_ID_WIDTH => 12,
      C_M_AXI_GP1_THREAD_ID_WIDTH => 12,
      C_NUM_F2P_INTR_INPUTS => 2,
      C_FCLK_CLK0_BUF => "TRUE",
      C_FCLK_CLK1_BUF => "FALSE",
      C_FCLK_CLK2_BUF => "FALSE",
      C_FCLK_CLK3_BUF => "FALSE"
      )
    port map(
      CAN0_PHY_TX => open,
      CAN0_PHY_RX => '0',
      CAN1_PHY_TX => open,
      CAN1_PHY_RX => '0',
      ENET0_GMII_TX_EN => open,
      ENET0_GMII_TX_ER => open,
      ENET0_MDIO_MDC => open,
      ENET0_MDIO_O => open,
      ENET0_MDIO_T => open,
      ENET0_PTP_DELAY_REQ_RX => open,
      ENET0_PTP_DELAY_REQ_TX => open,
      ENET0_PTP_PDELAY_REQ_RX => open,
      ENET0_PTP_PDELAY_REQ_TX => open,
      ENET0_PTP_PDELAY_RESP_RX => open,
      ENET0_PTP_PDELAY_RESP_TX => open,
      ENET0_PTP_SYNC_FRAME_RX => open,
      ENET0_PTP_SYNC_FRAME_TX => open,
      ENET0_SOF_RX => open,
      ENET0_SOF_TX => open,
      ENET0_GMII_TXD => open,
      ENET0_GMII_COL => '0',
      ENET0_GMII_CRS => '0',
      ENET0_EXT_INTIN => '0',
      ENET0_GMII_RX_CLK => '0',
      ENET0_GMII_RX_DV => '0',
      ENET0_GMII_RX_ER => '0',
      ENET0_GMII_TX_CLK => '0',
      ENET0_MDIO_I => '0',
      ENET0_GMII_RXD => (others => '0'),
      ENET1_GMII_TX_EN => open,
      ENET1_GMII_TX_ER => open,
      ENET1_MDIO_MDC => open,
      ENET1_MDIO_O => open,
      ENET1_MDIO_T => open,
      ENET1_PTP_DELAY_REQ_RX => open,
      ENET1_PTP_DELAY_REQ_TX => open,
      ENET1_PTP_PDELAY_REQ_RX => open,
      ENET1_PTP_PDELAY_REQ_TX => open,
      ENET1_PTP_PDELAY_RESP_RX => open,
      ENET1_PTP_PDELAY_RESP_TX => open,
      ENET1_PTP_SYNC_FRAME_RX => open,
      ENET1_PTP_SYNC_FRAME_TX => open,
      ENET1_SOF_RX => open,
      ENET1_SOF_TX => open,
      ENET1_GMII_TXD => open,
      ENET1_GMII_COL => '0',
      ENET1_GMII_CRS => '0',
      ENET1_EXT_INTIN => '0',
      ENET1_GMII_RX_CLK => '0',
      ENET1_GMII_RX_DV => '0',
      ENET1_GMII_RX_ER => '0',
      ENET1_GMII_TX_CLK => '0',
      ENET1_MDIO_I => '0',
      ENET1_GMII_RXD => (others => '0'),
      GPIO_I => (others => '0'),
      GPIO_O => open,
      GPIO_T => open,
      I2C0_SDA_I => '0',
      I2C0_SDA_O => open,
      I2C0_SDA_T => open,
      I2C0_SCL_I => '0',
      I2C0_SCL_O => open,
      I2C0_SCL_T => open,
      I2C1_SDA_I => '0',
      I2C1_SDA_O => open,
      I2C1_SDA_T => open,
      I2C1_SCL_I => '0',
      I2C1_SCL_O => open,
      I2C1_SCL_T => open,
      PJTAG_TCK => '0',
      PJTAG_TMS => '0',
      PJTAG_TD_I => '0',
      PJTAG_TD_T => open,
      PJTAG_TD_O => open,
      SDIO0_CLK => open,
      SDIO0_CLK_FB => '0',
      SDIO0_CMD_O => open,
      SDIO0_CMD_I => '0',
      SDIO0_CMD_T => open,
      SDIO0_DATA_I => (others => '0'),
      SDIO0_DATA_O => open,
      SDIO0_DATA_T => open,
      SDIO0_LED => open,
      SDIO0_CDN => '0',
      SDIO0_WP => '0',
      SDIO0_BUSPOW => open,
      SDIO0_BUSVOLT => open,
      SDIO1_CLK => open,
      SDIO1_CLK_FB => '0',
      SDIO1_CMD_O => open,
      SDIO1_CMD_I => '0',
      SDIO1_CMD_T => open,
      SDIO1_DATA_I => (others => '0'),
      SDIO1_DATA_O => open,
      SDIO1_DATA_T => open,
      SDIO1_LED => open,
      SDIO1_CDN => '0',
      SDIO1_WP => '0',
      SDIO1_BUSPOW => open,
      SDIO1_BUSVOLT => open,
      SPI0_SCLK_I => '0',
      SPI0_SCLK_O => open,
      SPI0_SCLK_T => open,
      SPI0_MOSI_I => '0',
      SPI0_MOSI_O => open,
      SPI0_MOSI_T => open,
      SPI0_MISO_I => '0',
      SPI0_MISO_O => open,
      SPI0_MISO_T => open,
      SPI0_SS_I => '0',
      SPI0_SS_O => open,
      SPI0_SS1_O => open,
      SPI0_SS2_O => open,
      SPI0_SS_T => open,
      SPI1_SCLK_I => '0',
      SPI1_SCLK_O => open,
      SPI1_SCLK_T => open,
      SPI1_MOSI_I => '0',
      SPI1_MOSI_O => open,
      SPI1_MOSI_T => open,
      SPI1_MISO_I => '0',
      SPI1_MISO_O => open,
      SPI1_MISO_T => open,
      SPI1_SS_I => '0',
      SPI1_SS_O => open,
      SPI1_SS1_O => open,
      SPI1_SS2_O => open,
      SPI1_SS_T => open,
      UART0_DTRN => open,
      UART0_RTSN => open,
      UART0_TX => open,
      UART0_CTSN => '0',
      UART0_DCDN => '0',
      UART0_DSRN => '0',
      UART0_RIN => '0',
      UART0_RX => '0',
      UART1_DTRN => open,
      UART1_RTSN => open,
      UART1_TX => open,
      UART1_CTSN => '0',
      UART1_DCDN => '0',
      UART1_DSRN => '0',
      UART1_RIN => '0',
      UART1_RX => '0',
      TTC0_WAVE0_OUT => open,
      TTC0_WAVE1_OUT => open,
      TTC0_WAVE2_OUT => open,
      TTC0_CLK0_IN => '0',
      TTC0_CLK1_IN => '0',
      TTC0_CLK2_IN => '0',
      TTC1_WAVE0_OUT => open,
      TTC1_WAVE1_OUT => open,
      TTC1_WAVE2_OUT => open,
      TTC1_CLK0_IN => '0',
      TTC1_CLK1_IN => '0',
      TTC1_CLK2_IN => '0',
      WDT_CLK_IN => '0',
      WDT_RST_OUT => open,
      TRACE_CLK => '0',
      TRACE_CTL => open,
      TRACE_DATA => open,
      USB0_PORT_INDCTL => open,
      USB1_PORT_INDCTL => open,
      USB0_VBUS_PWRSELECT => open,
      USB1_VBUS_PWRSELECT => open,
      USB0_VBUS_PWRFAULT => '0',
      USB1_VBUS_PWRFAULT => '0',
      SRAM_INTIN => '0',
      M_AXI_GP0_ARESETN => axi_gp_out.ARESETN,
      M_AXI_GP0_ARVALID => axi_gp_out.ARVALID,
      M_AXI_GP0_AWVALID => axi_gp_out.AWVALID,
      M_AXI_GP0_BREADY => axi_gp_out.BREADY,
      M_AXI_GP0_RREADY => axi_gp_out.RREADY,
      M_AXI_GP0_WLAST => axi_gp_out.WLAST,
      M_AXI_GP0_WVALID => axi_gp_out.WVALID,
      M_AXI_GP0_ARID => axi_gp_out.ARID,
      M_AXI_GP0_AWID => axi_gp_out.AWID,
      M_AXI_GP0_WID => axi_gp_out.WID,
      M_AXI_GP0_ARBURST => axi_gp_out.ARBURST,
      M_AXI_GP0_ARLOCK => axi_gp_out.ARLOCK,
      M_AXI_GP0_ARSIZE => axi_gp_out.ARSIZE,
      M_AXI_GP0_AWBURST => axi_gp_out.AWBURST,
      M_AXI_GP0_AWLOCK => axi_gp_out.AWLOCK,
      M_AXI_GP0_AWSIZE => axi_gp_out.AWSIZE,
      M_AXI_GP0_ARPROT => axi_gp_out.ARPROT,
      M_AXI_GP0_AWPROT => axi_gp_out.AWPROT,
      M_AXI_GP0_ARADDR => axi_gp_out.ARADDR,
      M_AXI_GP0_AWADDR => axi_gp_out.AWADDR,
      M_AXI_GP0_WDATA => axi_gp_out.WDATA,
      M_AXI_GP0_ARCACHE => axi_gp_out.ARCACHE,
      M_AXI_GP0_ARLEN => axi_gp_out.ARLEN,
      M_AXI_GP0_ARQOS => axi_gp_out.ARQOS,
      M_AXI_GP0_AWCACHE => axi_gp_out.AWCACHE,
      M_AXI_GP0_AWLEN => axi_gp_out.AWLEN,
      M_AXI_GP0_AWQOS => axi_gp_out.AWQOS,
      M_AXI_GP0_WSTRB => axi_gp_out.WSTRB,
      M_AXI_GP0_ACLK => axi_gp_in.ACLK,
      M_AXI_GP0_ARREADY => axi_gp_in.ARREADY,
      M_AXI_GP0_AWREADY => axi_gp_in.AWREADY,
      M_AXI_GP0_BVALID => axi_gp_in.BVALID,
      M_AXI_GP0_RLAST => axi_gp_in.RLAST,
      M_AXI_GP0_RVALID => axi_gp_in.RVALID,
      M_AXI_GP0_WREADY => axi_gp_in.WREADY,
      M_AXI_GP0_BID => axi_gp_in.BID,
      M_AXI_GP0_RID => axi_gp_in.RID,
      M_AXI_GP0_BRESP => axi_gp_in.BRESP,
      M_AXI_GP0_RRESP => axi_gp_in.RRESP,
      M_AXI_GP0_RDATA => axi_gp_in.RDATA,
      M_AXI_GP1_ARESETN => open,
      M_AXI_GP1_ARVALID => open,
      M_AXI_GP1_AWVALID => open,
      M_AXI_GP1_BREADY => open,
      M_AXI_GP1_RREADY => open,
      M_AXI_GP1_WLAST => open,
      M_AXI_GP1_WVALID => open,
      M_AXI_GP1_ARID => open,
      M_AXI_GP1_AWID => open,
      M_AXI_GP1_WID => open,
      M_AXI_GP1_ARBURST => open,
      M_AXI_GP1_ARLOCK => open,
      M_AXI_GP1_ARSIZE => open,
      M_AXI_GP1_AWBURST => open,
      M_AXI_GP1_AWLOCK => open,
      M_AXI_GP1_AWSIZE => open,
      M_AXI_GP1_ARPROT => open,
      M_AXI_GP1_AWPROT => open,
      M_AXI_GP1_ARADDR => open,
      M_AXI_GP1_AWADDR => open,
      M_AXI_GP1_WDATA => open,
      M_AXI_GP1_ARCACHE => open,
      M_AXI_GP1_ARLEN => open,
      M_AXI_GP1_ARQOS => open,
      M_AXI_GP1_AWCACHE => open,
      M_AXI_GP1_AWLEN => open,
      M_AXI_GP1_AWQOS => open,
      M_AXI_GP1_WSTRB => open,
      M_AXI_GP1_ACLK => '0',
      M_AXI_GP1_ARREADY => '0',
      M_AXI_GP1_AWREADY => '0',
      M_AXI_GP1_BVALID => '0',
      M_AXI_GP1_RLAST => '0',
      M_AXI_GP1_RVALID => '0',
      M_AXI_GP1_WREADY => '0',
      M_AXI_GP1_BID => (others => '0'),
      M_AXI_GP1_RID => (others => '0'),
      M_AXI_GP1_BRESP => (others => '0'),
      M_AXI_GP1_RRESP => (others => '0'),
      M_AXI_GP1_RDATA => (others => '0'),
      S_AXI_GP0_ARESETN => open,
      S_AXI_GP0_ARREADY => open,
      S_AXI_GP0_AWREADY => open,
      S_AXI_GP0_BVALID => open,
      S_AXI_GP0_RLAST => open,
      S_AXI_GP0_RVALID => open,
      S_AXI_GP0_WREADY => open,
      S_AXI_GP0_BRESP => open,
      S_AXI_GP0_RRESP => open,
      S_AXI_GP0_RDATA => open,
      S_AXI_GP0_BID => open,
      S_AXI_GP0_RID => open,
      S_AXI_GP0_ACLK => '0',
      S_AXI_GP0_ARVALID => '0',
      S_AXI_GP0_AWVALID => '0',
      S_AXI_GP0_BREADY => '0',
      S_AXI_GP0_RREADY => '0',
      S_AXI_GP0_WLAST => '0',
      S_AXI_GP0_WVALID => '0',
      S_AXI_GP0_ARBURST => (others => '0'),
      S_AXI_GP0_ARLOCK => (others => '0'),
      S_AXI_GP0_ARSIZE => (others => '0'),
      S_AXI_GP0_AWBURST => (others => '0'),
      S_AXI_GP0_AWLOCK => (others => '0'),
      S_AXI_GP0_AWSIZE => (others => '0'),
      S_AXI_GP0_ARPROT => (others => '0'),
      S_AXI_GP0_AWPROT => (others => '0'),
      S_AXI_GP0_ARADDR => (others => '0'),
      S_AXI_GP0_AWADDR => (others => '0'),
      S_AXI_GP0_WDATA => (others => '0'),
      S_AXI_GP0_ARCACHE => (others => '0'),
      S_AXI_GP0_ARLEN => (others => '0'),
      S_AXI_GP0_ARQOS => (others => '0'),
      S_AXI_GP0_AWCACHE => (others => '0'),
      S_AXI_GP0_AWLEN => (others => '0'),
      S_AXI_GP0_AWQOS => (others => '0'),
      S_AXI_GP0_WSTRB => (others => '0'),
      S_AXI_GP0_ARID => (others => '0'),
      S_AXI_GP0_AWID => (others => '0'),
      S_AXI_GP0_WID => (others => '0'),
      S_AXI_GP1_ARESETN => open,
      S_AXI_GP1_ARREADY => open,
      S_AXI_GP1_AWREADY => open,
      S_AXI_GP1_BVALID => open,
      S_AXI_GP1_RLAST => open,
      S_AXI_GP1_RVALID => open,
      S_AXI_GP1_WREADY => open,
      S_AXI_GP1_BRESP => open,
      S_AXI_GP1_RRESP => open,
      S_AXI_GP1_RDATA => open,
      S_AXI_GP1_BID => open,
      S_AXI_GP1_RID => open,
      S_AXI_GP1_ACLK => '0',
      S_AXI_GP1_ARVALID => '0',
      S_AXI_GP1_AWVALID => '0',
      S_AXI_GP1_BREADY => '0',
      S_AXI_GP1_RREADY => '0',
      S_AXI_GP1_WLAST => '0',
      S_AXI_GP1_WVALID => '0',
      S_AXI_GP1_ARBURST => (others => '0'),
      S_AXI_GP1_ARLOCK => (others => '0'),
      S_AXI_GP1_ARSIZE => (others => '0'),
      S_AXI_GP1_AWBURST => (others => '0'),
      S_AXI_GP1_AWLOCK => (others => '0'),
      S_AXI_GP1_AWSIZE => (others => '0'),
      S_AXI_GP1_ARPROT => (others => '0'),
      S_AXI_GP1_AWPROT => (others => '0'),
      S_AXI_GP1_ARADDR => (others => '0'),
      S_AXI_GP1_AWADDR => (others => '0'),
      S_AXI_GP1_WDATA => (others => '0'),
      S_AXI_GP1_ARCACHE => (others => '0'),
      S_AXI_GP1_ARLEN => (others => '0'),
      S_AXI_GP1_ARQOS => (others => '0'),
      S_AXI_GP1_AWCACHE => (others => '0'),
      S_AXI_GP1_AWLEN => (others => '0'),
      S_AXI_GP1_AWQOS => (others => '0'),
      S_AXI_GP1_WSTRB => (others => '0'),
      S_AXI_GP1_ARID => (others => '0'),
      S_AXI_GP1_AWID => (others => '0'),
      S_AXI_GP1_WID => (others => '0'),
      S_AXI_ACP_ARESETN => open,
      S_AXI_ACP_AWREADY => open,
      S_AXI_ACP_ARREADY => open,
      S_AXI_ACP_BVALID => open,
      S_AXI_ACP_RLAST => open,
      S_AXI_ACP_RVALID => open,
      S_AXI_ACP_WREADY => open,
      S_AXI_ACP_BRESP => open,
      S_AXI_ACP_RRESP => open,
      S_AXI_ACP_BID => open,
      S_AXI_ACP_RID => open,
      S_AXI_ACP_RDATA => open,
      S_AXI_ACP_ACLK => '0',
      S_AXI_ACP_ARVALID => '0',
      S_AXI_ACP_AWVALID => '0',
      S_AXI_ACP_BREADY => '0',
      S_AXI_ACP_RREADY => '0',
      S_AXI_ACP_WLAST => '0',
      S_AXI_ACP_WVALID => '0',
      S_AXI_ACP_ARID => (others => '0'),
      S_AXI_ACP_ARPROT => (others => '0'),
      S_AXI_ACP_AWID => (others => '0'),
      S_AXI_ACP_AWPROT => (others => '0'),
      S_AXI_ACP_WID => (others => '0'),
      S_AXI_ACP_ARADDR => (others => '0'),
      S_AXI_ACP_AWADDR => (others => '0'),
      S_AXI_ACP_ARCACHE => (others => '0'),
      S_AXI_ACP_ARLEN => (others => '0'),
      S_AXI_ACP_ARQOS => (others => '0'),
      S_AXI_ACP_AWCACHE => (others => '0'),
      S_AXI_ACP_AWLEN => (others => '0'),
      S_AXI_ACP_AWQOS => (others => '0'),
      S_AXI_ACP_ARBURST => (others => '0'),
      S_AXI_ACP_ARLOCK => (others => '0'),
      S_AXI_ACP_ARSIZE => (others => '0'),
      S_AXI_ACP_AWBURST => (others => '0'),
      S_AXI_ACP_AWLOCK => (others => '0'),
      S_AXI_ACP_AWSIZE => (others => '0'),
      S_AXI_ACP_ARUSER => (others => '0'),
      S_AXI_ACP_AWUSER => (others => '0'),
      S_AXI_ACP_WDATA => (others => '0'),
      S_AXI_ACP_WSTRB => (others => '0'),
      S_AXI_HP0_ARESETN => open,
      S_AXI_HP0_ARREADY => open,
      S_AXI_HP0_AWREADY => open,
      S_AXI_HP0_BVALID => open,
      S_AXI_HP0_RLAST => open,
      S_AXI_HP0_RVALID => open,
      S_AXI_HP0_WREADY => open,
      S_AXI_HP0_BRESP => open,
      S_AXI_HP0_RRESP => open,
      S_AXI_HP0_BID => open,
      S_AXI_HP0_RID => open,
      S_AXI_HP0_RDATA => open,
      S_AXI_HP0_RCOUNT => open,
      S_AXI_HP0_WCOUNT => open,
      S_AXI_HP0_RACOUNT => open,
      S_AXI_HP0_WACOUNT => open,
      S_AXI_HP0_ACLK => '0',
      S_AXI_HP0_ARVALID => '0',
      S_AXI_HP0_AWVALID => '0',
      S_AXI_HP0_BREADY => '0',
      S_AXI_HP0_RDISSUECAP1_EN => '0',
      S_AXI_HP0_RREADY => '0',
      S_AXI_HP0_WLAST => '0',
      S_AXI_HP0_WRISSUECAP1_EN => '0',
      S_AXI_HP0_WVALID => '0',
      S_AXI_HP0_ARBURST => (others => '0'),
      S_AXI_HP0_ARLOCK => (others => '0'),
      S_AXI_HP0_ARSIZE => (others => '0'),
      S_AXI_HP0_AWBURST => (others => '0'),
      S_AXI_HP0_AWLOCK => (others => '0'),
      S_AXI_HP0_AWSIZE => (others => '0'),
      S_AXI_HP0_ARPROT => (others => '0'),
      S_AXI_HP0_AWPROT => (others => '0'),
      S_AXI_HP0_ARADDR => (others => '0'),
      S_AXI_HP0_AWADDR => (others => '0'),
      S_AXI_HP0_ARCACHE => (others => '0'),
      S_AXI_HP0_ARLEN => (others => '0'),
      S_AXI_HP0_ARQOS => (others => '0'),
      S_AXI_HP0_AWCACHE => (others => '0'),
      S_AXI_HP0_AWLEN => (others => '0'),
      S_AXI_HP0_AWQOS => (others => '0'),
      S_AXI_HP0_ARID => (others => '0'),
      S_AXI_HP0_AWID => (others => '0'),
      S_AXI_HP0_WID => (others => '0'),
      S_AXI_HP0_WDATA => (others => '0'),
      S_AXI_HP0_WSTRB => (others => '0'),
      S_AXI_HP1_ARESETN => open,
      S_AXI_HP1_ARREADY => open,
      S_AXI_HP1_AWREADY => open,
      S_AXI_HP1_BVALID => open,
      S_AXI_HP1_RLAST => open,
      S_AXI_HP1_RVALID => open,
      S_AXI_HP1_WREADY => open,
      S_AXI_HP1_BRESP => open,
      S_AXI_HP1_RRESP => open,
      S_AXI_HP1_BID => open,
      S_AXI_HP1_RID => open,
      S_AXI_HP1_RDATA => open,
      S_AXI_HP1_RCOUNT => open,
      S_AXI_HP1_WCOUNT => open,
      S_AXI_HP1_RACOUNT => open,
      S_AXI_HP1_WACOUNT => open,
      S_AXI_HP1_ACLK => '0',
      S_AXI_HP1_ARVALID => '0',
      S_AXI_HP1_AWVALID => '0',
      S_AXI_HP1_BREADY => '0',
      S_AXI_HP1_RDISSUECAP1_EN => '0',
      S_AXI_HP1_RREADY => '0',
      S_AXI_HP1_WLAST => '0',
      S_AXI_HP1_WRISSUECAP1_EN => '0',
      S_AXI_HP1_WVALID => '0',
      S_AXI_HP1_ARBURST => (others => '0'),
      S_AXI_HP1_ARLOCK => (others => '0'),
      S_AXI_HP1_ARSIZE => (others => '0'),
      S_AXI_HP1_AWBURST => (others => '0'),
      S_AXI_HP1_AWLOCK => (others => '0'),
      S_AXI_HP1_AWSIZE => (others => '0'),
      S_AXI_HP1_ARPROT => (others => '0'),
      S_AXI_HP1_AWPROT => (others => '0'),
      S_AXI_HP1_ARADDR => (others => '0'),
      S_AXI_HP1_AWADDR => (others => '0'),
      S_AXI_HP1_ARCACHE => (others => '0'),
      S_AXI_HP1_ARLEN => (others => '0'),
      S_AXI_HP1_ARQOS => (others => '0'),
      S_AXI_HP1_AWCACHE => (others => '0'),
      S_AXI_HP1_AWLEN => (others => '0'),
      S_AXI_HP1_AWQOS => (others => '0'),
      S_AXI_HP1_ARID => (others => '0'),
      S_AXI_HP1_AWID => (others => '0'),
      S_AXI_HP1_WID => (others => '0'),
      S_AXI_HP1_WDATA => (others => '0'),
      S_AXI_HP1_WSTRB => (others => '0'),
      S_AXI_HP2_ARESETN => open,
      S_AXI_HP2_ARREADY => open,
      S_AXI_HP2_AWREADY => open,
      S_AXI_HP2_BVALID => open,
      S_AXI_HP2_RLAST => open,
      S_AXI_HP2_RVALID => open,
      S_AXI_HP2_WREADY => open,
      S_AXI_HP2_BRESP => open,
      S_AXI_HP2_RRESP => open,
      S_AXI_HP2_BID => open,
      S_AXI_HP2_RID => open,
      S_AXI_HP2_RDATA => open,
      S_AXI_HP2_RCOUNT => open,
      S_AXI_HP2_WCOUNT => open,
      S_AXI_HP2_RACOUNT => open,
      S_AXI_HP2_WACOUNT => open,
      S_AXI_HP2_ACLK => '0',
      S_AXI_HP2_ARVALID => '0',
      S_AXI_HP2_AWVALID => '0',
      S_AXI_HP2_BREADY => '0',
      S_AXI_HP2_RDISSUECAP1_EN => '0',
      S_AXI_HP2_RREADY => '0',
      S_AXI_HP2_WLAST => '0',
      S_AXI_HP2_WRISSUECAP1_EN => '0',
      S_AXI_HP2_WVALID => '0',
      S_AXI_HP2_ARBURST => (others => '0'),
      S_AXI_HP2_ARLOCK => (others => '0'),
      S_AXI_HP2_ARSIZE => (others => '0'),
      S_AXI_HP2_AWBURST => (others => '0'),
      S_AXI_HP2_AWLOCK => (others => '0'),
      S_AXI_HP2_AWSIZE => (others => '0'),
      S_AXI_HP2_ARPROT => (others => '0'),
      S_AXI_HP2_AWPROT => (others => '0'),
      S_AXI_HP2_ARADDR => (others => '0'),
      S_AXI_HP2_AWADDR => (others => '0'),
      S_AXI_HP2_ARCACHE => (others => '0'),
      S_AXI_HP2_ARLEN => (others => '0'),
      S_AXI_HP2_ARQOS => (others => '0'),
      S_AXI_HP2_AWCACHE => (others => '0'),
      S_AXI_HP2_AWLEN => (others => '0'),
      S_AXI_HP2_AWQOS => (others => '0'),
      S_AXI_HP2_ARID => (others => '0'),
      S_AXI_HP2_AWID => (others => '0'),
      S_AXI_HP2_WID => (others => '0'),
      S_AXI_HP2_WDATA => (others => '0'),
      S_AXI_HP2_WSTRB => (others => '0'),
      S_AXI_HP3_ARESETN => open,
      S_AXI_HP3_ARREADY => open,
      S_AXI_HP3_AWREADY => open,
      S_AXI_HP3_BVALID => open,
      S_AXI_HP3_RLAST => open,
      S_AXI_HP3_RVALID => open,
      S_AXI_HP3_WREADY => open,
      S_AXI_HP3_BRESP => open,
      S_AXI_HP3_RRESP => open,
      S_AXI_HP3_BID => open,
      S_AXI_HP3_RID => open,
      S_AXI_HP3_RDATA => open,
      S_AXI_HP3_RCOUNT => open,
      S_AXI_HP3_WCOUNT => open,
      S_AXI_HP3_RACOUNT => open,
      S_AXI_HP3_WACOUNT => open,
      S_AXI_HP3_ACLK => '0',
      S_AXI_HP3_ARVALID => '0',
      S_AXI_HP3_AWVALID => '0',
      S_AXI_HP3_BREADY => '0',
      S_AXI_HP3_RDISSUECAP1_EN => '0',
      S_AXI_HP3_RREADY => '0',
      S_AXI_HP3_WLAST => '0',
      S_AXI_HP3_WRISSUECAP1_EN => '0',
      S_AXI_HP3_WVALID => '0',
      S_AXI_HP3_ARBURST => (others => '0'),
      S_AXI_HP3_ARLOCK => (others => '0'),
      S_AXI_HP3_ARSIZE => (others => '0'),
      S_AXI_HP3_AWBURST => (others => '0'),
      S_AXI_HP3_AWLOCK => (others => '0'),
      S_AXI_HP3_AWSIZE => (others => '0'),
      S_AXI_HP3_ARPROT => (others => '0'),
      S_AXI_HP3_AWPROT => (others => '0'),
      S_AXI_HP3_ARADDR => (others => '0'),
      S_AXI_HP3_AWADDR => (others => '0'),
      S_AXI_HP3_ARCACHE => (others => '0'),
      S_AXI_HP3_ARLEN => (others => '0'),
      S_AXI_HP3_ARQOS => (others => '0'),
      S_AXI_HP3_AWCACHE => (others => '0'),
      S_AXI_HP3_AWLEN => (others => '0'),
      S_AXI_HP3_AWQOS => (others => '0'),
      S_AXI_HP3_ARID => (others => '0'),
      S_AXI_HP3_AWID => (others => '0'),
      S_AXI_HP3_WID => (others => '0'),
      S_AXI_HP3_WDATA => (others => '0'),
      S_AXI_HP3_WSTRB => (others => '0'),
      DMA0_DATYPE => open,
      DMA0_DAVALID => open,
      DMA0_DRREADY => open,
      DMA0_RSTN => open,
      DMA0_ACLK => '0',
      DMA0_DAREADY => '0',
      DMA0_DRLAST => '0',
      DMA0_DRVALID => '0',
      DMA0_DRTYPE => (others => '0'),
      DMA1_DATYPE => open,
      DMA1_DAVALID => open,
      DMA1_DRREADY => open,
      DMA1_RSTN => open,
      DMA1_ACLK => '0',
      DMA1_DAREADY => '0',
      DMA1_DRLAST => '0',
      DMA1_DRVALID => '0',
      DMA1_DRTYPE => (others => '0'),
      DMA2_DATYPE => open,
      DMA2_DAVALID => open,
      DMA2_DRREADY => open,
      DMA2_RSTN => open,
      DMA2_ACLK => '0',
      DMA2_DAREADY => '0',
      DMA2_DRLAST => '0',
      DMA2_DRVALID => '0',
      DMA3_DRVALID => '0',
      DMA3_DATYPE => open,
      DMA3_DAVALID => open,
      DMA3_DRREADY => open,
      DMA3_RSTN => open,
      DMA3_ACLK => '0',
      DMA3_DAREADY => '0',
      DMA3_DRLAST => '0',
      DMA2_DRTYPE => (others => '0'),
      DMA3_DRTYPE => (others => '0'),
      FTMD_TRACEIN_DATA => (others => '0'),
      FTMD_TRACEIN_VALID => '0',
      FTMD_TRACEIN_CLK => '0',
      FTMD_TRACEIN_ATID => (others => '0'),
      FTMT_F2P_TRIG => (others => '0'),
      FTMT_F2P_TRIGACK => open,
      FTMT_F2P_DEBUG => (others => '0'),
      FTMT_P2F_TRIGACK => (others => '0'),
      FTMT_P2F_TRIG => open,
      FTMT_P2F_DEBUG => open,
      FCLK_CLK3 => open,
      FCLK_CLK2 => open,
      FCLK_CLK1 => open,
      FCLK_CLK0 => pl_out.FCLK,
      FCLK_CLKTRIG3_N => '0',
      FCLK_CLKTRIG2_N => '0',
      FCLK_CLKTRIG1_N => '0',
      FCLK_CLKTRIG0_N => '0',
      FCLK_RESET3_N => open,
      FCLK_RESET2_N => open,
      FCLK_RESET1_N => open,
      FCLK_RESET0_N => pl_out.FCLKRESET_N,
      FPGA_IDLE_N => '0',
      DDR_ARB => (others => '0'),
      IRQ_F2P => (others => '0'),
      Core0_nFIQ => '0',
      Core0_nIRQ => '0',
      Core1_nFIQ => '0',
      Core1_nIRQ => '0',
      EVENT_EVENTO => open,
      EVENT_STANDBYWFE => open,
      EVENT_STANDBYWFI => open,
      EVENT_EVENTI => '0',
      MIO                => ps_inout.MIO,
      DDR_Clk            => ps_inout.DDR_Clk,
      DDR_Clk_n          => ps_inout.DDR_Clk_n,
      DDR_CKE            => ps_inout.DDR_CKE,
      DDR_CS_n           => ps_inout.DDR_CS_n,
      DDR_RAS_n          => ps_inout.DDR_RAS_n,
      DDR_CAS_n          => ps_inout.DDR_CAS_n,
      DDR_WEB            => ps_inout.DDR_WEB,
      DDR_BankAddr       => ps_inout.DDR_BankAddr,
      DDR_Addr           => ps_inout.DDR_Addr,
      DDR_ODT            => ps_inout.DDR_ODT,
      DDR_DRSTB          => ps_inout.DDR_DRSTB,
      DDR_DQ             => ps_inout.DDR_DQ,
      DDR_DM             => ps_inout.DDR_DM,
      DDR_DQS            => ps_inout.DDR_DQS,
      DDR_DQS_n          => ps_inout.DDR_DQS_n,
      DDR_VRN            => ps_inout.DDR_VRN,
      DDR_VRP            => ps_inout.DDR_VRP,
      PS_SRSTB           => ps_in.PS_SRSTB,
      PS_CLK             => ps_in.PS_CLK,
      PS_PORB            => ps_in.PS_PORB,
      IRQ_P2F_DMAC_ABORT => open,
      IRQ_P2F_DMAC0 => open,
      IRQ_P2F_DMAC1 => open,
      IRQ_P2F_DMAC2 => open,
      IRQ_P2F_DMAC3 => open,
      IRQ_P2F_DMAC4 => open,
      IRQ_P2F_DMAC5 => open,
      IRQ_P2F_DMAC6 => open,
      IRQ_P2F_DMAC7 => open,
      IRQ_P2F_SMC => open,
      IRQ_P2F_QSPI => open,
      IRQ_P2F_CTI => open,
      IRQ_P2F_GPIO => open,
      IRQ_P2F_USB0 => open,
      IRQ_P2F_ENET0 => open,
      IRQ_P2F_ENET_WAKE0 => open,
      IRQ_P2F_SDIO0 => open,
      IRQ_P2F_I2C0 => open,
      IRQ_P2F_SPI0 => open,
      IRQ_P2F_UART0 => open,
      IRQ_P2F_CAN0 => open,
      IRQ_P2F_USB1 => open,
      IRQ_P2F_ENET1 => open,
      IRQ_P2F_ENET_WAKE1 => open,
      IRQ_P2F_SDIO1 => open,
      IRQ_P2F_I2C1 => open,
      IRQ_P2F_SPI1 => open,
      IRQ_P2F_UART1 => open,
      IRQ_P2F_CAN1 => open
      );
end rtl;
