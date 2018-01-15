-- This file is protected by Copyright. Please refer to the COPYRIGHT file
-- distributed with this source distribution.
--
-- This file is part of OpenCPI <http://www.opencpi.org>
--
-- OpenCPI is free software: you can redistribute it and/or modify it under the
-- terms of the GNU Lesser General Public License as published by the Free
-- Software Foundation, either version 3 of the License, or (at your option) any
-- later version.
--
-- OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
-- WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
-- A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
-- details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program. If not, see <http://www.gnu.org/licenses/>.

-- This file contains zynq-specific definitions that have nothing to do with the
-- particulars of the zed board.

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all;
library platform; use platform.platform_pkg.all;
library axi; use axi.axi_pkg.all;
package zynq_pkg is

-- Signals from the PS for use by the PL
type ps2pl_t is record
  FCLK         : std_logic_vector(3 downto 0);
  FCLKRESET_N  : std_logic;
end record ps2pl_t;
-- Signals from the PL for use by the PS
type pl2ps_t is record
  DEBUG        : std_logic_vector(31 downto 0); --     FTMT_F2P_DEBUG
end record pl2ps_t;

  -- Consistent with EDK/hw/XilinxProcessorIPLib/pcores/
  --                 processing_system7_v4_03_a/hdl/verilog/processing_system7.v
  -- They don't supply the VHDL version...
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
    C_EN_EMIO_TRACE                 : integer := 0;
    C_DQ_WIDTH                      : integer := 32;
    C_DQS_WIDTH                     : integer := 4;
    C_DM_WIDTH                      : integer := 4;
    C_MIO_PRIMITIVE                 : integer := 54;
    C_PACKAGE_NAME                  : string  := "clg484"
    );
  port (
    CAN0_PHY_TX : out std_logic;
    CAN0_PHY_RX : in std_logic;

    CAN1_PHY_TX : out std_logic;
    CAN1_PHY_RX : in std_logic;

    ENET0_GMII_TX_EN         : out std_logic;
    ENET0_GMII_TX_ER         : out std_logic;
    ENET0_MDIO_MDC           : out std_logic;
    ENET0_MDIO_O             : out std_logic;
    ENET0_MDIO_T             : out std_logic;
    ENET0_PTP_DELAY_REQ_RX   : out std_logic;
    ENET0_PTP_DELAY_REQ_TX   : out std_logic;
    ENET0_PTP_PDELAY_REQ_RX  : out std_logic;
    ENET0_PTP_PDELAY_REQ_TX  : out std_logic;
    ENET0_PTP_PDELAY_RESP_RX : out std_logic;
    ENET0_PTP_PDELAY_RESP_TX : out std_logic;
    ENET0_PTP_SYNC_FRAME_RX  : out std_logic;
    ENET0_PTP_SYNC_FRAME_TX  : out std_logic;
    ENET0_SOF_RX             : out std_logic;
    ENET0_SOF_TX             : out std_logic;

    ENET0_GMII_TXD           : out std_logic_vector(7 downto 0);

    ENET0_GMII_COL           : in std_logic;
    ENET0_GMII_CRS           : in std_logic;
    ENET0_GMII_RX_CLK        : in std_logic;
    ENET0_GMII_RX_DV         : in std_logic;
    ENET0_GMII_RX_ER         : in std_logic;
    ENET0_GMII_TX_CLK        : in std_logic;
    ENET0_MDIO_I             : in std_logic;
    ENET0_EXT_INTIN          : in std_logic;
    ENET0_GMII_RXD           : in std_logic_vector(7 downto 0);

    ENET1_GMII_TX_EN         : out std_logic;
    ENET1_GMII_TX_ER         : out std_logic;
    ENET1_MDIO_MDC           : out std_logic;
    ENET1_MDIO_O             : out std_logic;
    ENET1_MDIO_T             : out std_logic;
    ENET1_PTP_DELAY_REQ_RX   : out std_logic;
    ENET1_PTP_DELAY_REQ_TX   : out std_logic;
    ENET1_PTP_PDELAY_REQ_RX  : out std_logic;
    ENET1_PTP_PDELAY_REQ_TX  : out std_logic;
    ENET1_PTP_PDELAY_RESP_RX : out std_logic;
    ENET1_PTP_PDELAY_RESP_TX : out std_logic;
    ENET1_PTP_SYNC_FRAME_RX  : out std_logic;
    ENET1_PTP_SYNC_FRAME_TX  : out std_logic;
    ENET1_SOF_RX             : out std_logic;
    ENET1_SOF_TX             : out std_logic;
    ENET1_GMII_TXD           : out std_logic_vector(7 downto 0);

    ENET1_GMII_COL           : in std_logic;
    ENET1_GMII_CRS           : in std_logic;
    ENET1_GMII_RX_CLK        : in std_logic;
    ENET1_GMII_RX_DV         : in std_logic;
    ENET1_GMII_RX_ER         : in std_logic;
    ENET1_GMII_TX_CLK        : in std_logic;
    ENET1_MDIO_I             : in std_logic;
    ENET1_EXT_INTIN          : in std_logic;
    ENET1_GMII_RXD           : in std_logic_vector(7 downto 0);

    GPIO_I : in std_logic_vector(C_EMIO_GPIO_WIDTH-1 downto 0);
    GPIO_O : out std_logic_vector(C_EMIO_GPIO_WIDTH-1 downto 0);
    GPIO_T : out std_logic_vector(C_EMIO_GPIO_WIDTH-1 downto 0);

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

    PJTAG_TCK  : in std_logic;
    PJTAG_TMS  : in std_logic;
    PJTAG_TD_I : in std_logic;
    PJTAG_TD_T : out std_logic;
    PJTAG_TD_O : out std_logic;

    SDIO0_CLK     : out std_logic;
    SDIO0_CLK_FB  : in std_logic;
    SDIO0_CMD_O   : out std_logic;
    SDIO0_CMD_I   : in std_logic;
    SDIO0_CMD_T   : out std_logic;
    SDIO0_DATA_I  : in std_logic_vector(3 downto 0);
    SDIO0_DATA_O  : out std_logic_vector(3 downto 0);
    SDIO0_DATA_T  : out std_logic_vector(3 downto 0);
    SDIO0_LED     : out std_logic;
    SDIO0_CDN     : in std_logic;
    SDIO0_WP      : in std_logic;
    SDIO0_BUSPOW  : out std_logic;
    SDIO0_BUSVOLT : out std_logic_vector(2 downto 0);

    SDIO1_CLK     : out std_logic;
    SDIO1_CLK_FB  : in std_logic;
    SDIO1_CMD_O   : out std_logic;
    SDIO1_CMD_I   : in std_logic;
    SDIO1_CMD_T   : out std_logic;
    SDIO1_DATA_I  : in std_logic_vector(3 downto 0);
    SDIO1_DATA_O  : out std_logic_vector(3 downto 0);
    SDIO1_DATA_T  : out std_logic_vector(3 downto 0);
    SDIO1_LED     : out std_logic;
    SDIO1_CDN     : in std_logic;
    SDIO1_WP      : in std_logic;
    SDIO1_BUSPOW  : out std_logic;
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
    SPI0_SS_I   : in std_logic;
    SPI0_SS_O   : out std_logic;
    SPI0_SS1_O  : out std_logic;
    SPI0_SS2_O  : out std_logic;
    SPI0_SS_T   : out std_logic;

    SPI1_SCLK_I : in std_logic;
    SPI1_SCLK_O : out std_logic;
    SPI1_SCLK_T : out std_logic;
    SPI1_MOSI_I : in std_logic;
    SPI1_MOSI_O : out std_logic;
    SPI1_MOSI_T : out std_logic;
    SPI1_MISO_I : in std_logic;
    SPI1_MISO_O : out std_logic;
    SPI1_MISO_T : out std_logic;
    SPI1_SS_I   : in std_logic;
    SPI1_SS_O   : out std_logic;
    SPI1_SS1_O  : out std_logic;
    SPI1_SS2_O  : out std_logic;
    SPI1_SS_T   : out std_logic;

    UART0_DTRN : out std_logic;
    UART0_RTSN : out std_logic;
    UART0_TX   : out std_logic;
    UART0_CTSN : in std_logic;
    UART0_DCDN : in std_logic;
    UART0_DSRN : in std_logic;
    UART0_RIN  : in std_logic;
    UART0_RX   : in std_logic;

    UART1_DTRN : out std_logic;
    UART1_RTSN : out std_logic;
    UART1_TX   : out std_logic;
    UART1_CTSN : in std_logic;
    UART1_DCDN : in std_logic;
    UART1_DSRN : in std_logic;
    UART1_RIN  : in std_logic;
    UART1_RX   : in std_logic;

    TTC0_WAVE0_OUT : out std_logic;
    TTC0_WAVE1_OUT : out std_logic;
    TTC0_WAVE2_OUT : out std_logic;
    TTC0_CLK0_IN   : in std_logic;
    TTC0_CLK1_IN   : in std_logic;
    TTC0_CLK2_IN   : in std_logic;

    TTC1_WAVE0_OUT : out std_logic;
    TTC1_WAVE1_OUT : out std_logic;
    TTC1_WAVE2_OUT : out std_logic;
    TTC1_CLK0_IN   : in std_logic;
    TTC1_CLK1_IN   : in std_logic;
    TTC1_CLK2_IN   : in std_logic;

    WDT_CLK_IN  : in std_logic;
    WDT_RST_OUT : out std_logic;

    TRACE_CLK  : in std_logic;
    TRACE_CTL  : out std_logic;
    TRACE_DATA : out std_logic_vector(31 downto 0);

    USB0_PORT_INDCTL    : out std_logic_vector(1 downto 0);
    USB0_VBUS_PWRSELECT : out std_logic;
    USB0_VBUS_PWRFAULT  : in std_logic;

    USB1_PORT_INDCTL    : out std_logic_vector(1 downto 0);
    USB1_VBUS_PWRSELECT : out std_logic;
    USB1_VBUS_PWRFAULT  : in std_logic;

    SRAM_INTIN : in std_logic;

    M_AXI_GP0_ARESETN : out std_logic;
    M_AXI_GP0_ARVALID : out std_logic;
    M_AXI_GP0_AWVALID : out std_logic;
    M_AXI_GP0_BREADY : out std_logic;
    M_AXI_GP0_RREADY : out std_logic;
    M_AXI_GP0_WLAST : out std_logic;
    M_AXI_GP0_WVALID : out std_logic;
    M_AXI_GP0_ARID : out std_logic_vector(C_M_AXI_GP0_THREAD_ID_WIDTH-1 downto 0);
    M_AXI_GP0_AWID : out std_logic_vector(C_M_AXI_GP0_THREAD_ID_WIDTH-1 downto 0);
    M_AXI_GP0_WID : out std_logic_vector(C_M_AXI_GP0_THREAD_ID_WIDTH-1 downto 0);
    M_AXI_GP0_ARBURST : out std_logic_vector(1 downto 0);
    M_AXI_GP0_ARLOCK : out std_logic_vector(1 downto 0);
    M_AXI_GP0_ARSIZE : out std_logic_vector(2 downto 0);
    M_AXI_GP0_AWBURST : out std_logic_vector(1 downto 0);
    M_AXI_GP0_AWLOCK : out std_logic_vector(1 downto 0);
    M_AXI_GP0_AWSIZE : out std_logic_vector(2 downto 0);
    M_AXI_GP0_ARPROT : out std_logic_vector(2 downto 0);
    M_AXI_GP0_AWPROT : out std_logic_vector(2 downto 0);
    M_AXI_GP0_ARADDR : out std_logic_vector(C_AXI_ADDR_WIDTH-1 downto 0);
    M_AXI_GP0_AWADDR : out std_logic_vector(C_AXI_ADDR_WIDTH-1 downto 0);
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
    M_AXI_GP0_BID : in std_logic_vector(C_M_AXI_GP0_THREAD_ID_WIDTH-1 downto 0);
    M_AXI_GP0_RID : in std_logic_vector(C_M_AXI_GP0_THREAD_ID_WIDTH-1 downto 0);
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
    M_AXI_GP1_ARID : out std_logic_vector(C_M_AXI_GP1_THREAD_ID_WIDTH-1 downto 0);
    M_AXI_GP1_AWID : out std_logic_vector(C_M_AXI_GP1_THREAD_ID_WIDTH-1 downto 0);
    M_AXI_GP1_WID : out std_logic_vector(C_M_AXI_GP1_THREAD_ID_WIDTH-1 downto 0);
    M_AXI_GP1_ARBURST : out std_logic_vector(1 downto 0);
    M_AXI_GP1_ARLOCK : out std_logic_vector(1 downto 0);
    M_AXI_GP1_ARSIZE : out std_logic_vector(2 downto 0);
    M_AXI_GP1_AWBURST : out std_logic_vector(1 downto 0);
    M_AXI_GP1_AWLOCK : out std_logic_vector(1 downto 0);
    M_AXI_GP1_AWSIZE : out std_logic_vector(2 downto 0);
    M_AXI_GP1_ARPROT : out std_logic_vector(2 downto 0);
    M_AXI_GP1_AWPROT : out std_logic_vector(2 downto 0);
    M_AXI_GP1_ARADDR : out std_logic_vector(C_AXI_ADDR_WIDTH-1 downto 0);
    M_AXI_GP1_AWADDR : out std_logic_vector(C_AXI_ADDR_WIDTH-1 downto 0);
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
    M_AXI_GP1_BID : in std_logic_vector(C_M_AXI_GP1_THREAD_ID_WIDTH-1 downto 0);
    M_AXI_GP1_RID : in std_logic_vector(C_M_AXI_GP1_THREAD_ID_WIDTH-1 downto 0);
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
    S_AXI_GP0_BID : out std_logic_vector(C_S_AXI_GP0_ID_WIDTH-1 downto 0);
    S_AXI_GP0_RID : out std_logic_vector(C_S_AXI_GP0_ID_WIDTH-1 downto 0);
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
    S_AXI_GP0_ARADDR : in std_logic_vector(C_AXI_ADDR_WIDTH-1 downto 0);
    S_AXI_GP0_AWADDR : in std_logic_vector(C_AXI_ADDR_WIDTH-1 downto 0);
    S_AXI_GP0_WDATA : in std_logic_vector(31 downto 0);
    S_AXI_GP0_ARCACHE : in std_logic_vector(3 downto 0);
    S_AXI_GP0_ARLEN : in std_logic_vector(3 downto 0);
    S_AXI_GP0_ARQOS : in std_logic_vector(3 downto 0);
    S_AXI_GP0_AWCACHE : in std_logic_vector(3 downto 0);
    S_AXI_GP0_AWLEN : in std_logic_vector(3 downto 0);
    S_AXI_GP0_AWQOS : in std_logic_vector(3 downto 0);
    S_AXI_GP0_WSTRB : in std_logic_vector(3 downto 0);
    S_AXI_GP0_ARID : in std_logic_vector(C_S_AXI_GP0_ID_WIDTH-1 downto 0);
    S_AXI_GP0_AWID : in std_logic_vector(C_S_AXI_GP0_ID_WIDTH-1 downto 0);
    S_AXI_GP0_WID : in std_logic_vector(C_S_AXI_GP0_ID_WIDTH-1 downto 0);

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
    S_AXI_GP1_BID : out std_logic_vector(C_S_AXI_GP1_ID_WIDTH-1 downto 0);
    S_AXI_GP1_RID : out std_logic_vector(C_S_AXI_GP1_ID_WIDTH-1 downto 0);
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
    S_AXI_GP1_ARADDR : in std_logic_vector(C_AXI_ADDR_WIDTH-1 downto 0);
    S_AXI_GP1_AWADDR : in std_logic_vector(C_AXI_ADDR_WIDTH-1 downto 0);
    S_AXI_GP1_WDATA : in std_logic_vector(31 downto 0);
    S_AXI_GP1_ARCACHE : in std_logic_vector(3 downto 0);
    S_AXI_GP1_ARLEN : in std_logic_vector(3 downto 0);
    S_AXI_GP1_ARQOS : in std_logic_vector(3 downto 0);
    S_AXI_GP1_AWCACHE : in std_logic_vector(3 downto 0);
    S_AXI_GP1_AWLEN : in std_logic_vector(3 downto 0);
    S_AXI_GP1_AWQOS : in std_logic_vector(3 downto 0);
    S_AXI_GP1_WSTRB : in std_logic_vector(3 downto 0);
    S_AXI_GP1_ARID : in std_logic_vector(C_S_AXI_GP1_ID_WIDTH-1 downto 0);
    S_AXI_GP1_AWID : in std_logic_vector(C_S_AXI_GP1_ID_WIDTH-1 downto 0);
    S_AXI_GP1_WID : in std_logic_vector(C_S_AXI_GP1_ID_WIDTH-1 downto 0);

    S_AXI_ACP_ARESETN : out std_logic;
    S_AXI_ACP_AWREADY : out std_logic;
    S_AXI_ACP_ARREADY : out std_logic;
    S_AXI_ACP_BVALID : out std_logic;
    S_AXI_ACP_RLAST : out std_logic;
    S_AXI_ACP_RVALID : out std_logic;
    S_AXI_ACP_WREADY : out std_logic;
    S_AXI_ACP_BRESP : out std_logic_vector(1 downto 0);
    S_AXI_ACP_RRESP : out std_logic_vector(1 downto 0);
    S_AXI_ACP_BID : out std_logic_vector(C_S_AXI_ACP_ID_WIDTH-1 downto 0);
    S_AXI_ACP_RID : out std_logic_vector(C_S_AXI_ACP_ID_WIDTH-1 downto 0);
    S_AXI_ACP_RDATA : out std_logic_vector(63 downto 0);
    S_AXI_ACP_ACLK : in std_logic;
    S_AXI_ACP_ARVALID : in std_logic;
    S_AXI_ACP_AWVALID : in std_logic;
    S_AXI_ACP_BREADY : in std_logic;
    S_AXI_ACP_RREADY : in std_logic;
    S_AXI_ACP_WLAST : in std_logic;
    S_AXI_ACP_WVALID : in std_logic;
    S_AXI_ACP_ARID : in std_logic_vector(C_S_AXI_ACP_ID_WIDTH-1 downto 0);
    S_AXI_ACP_ARPROT : in std_logic_vector(2 downto 0);
    S_AXI_ACP_AWID : in std_logic_vector(C_S_AXI_ACP_ID_WIDTH-1 downto 0);
    S_AXI_ACP_AWPROT : in std_logic_vector(2 downto 0);
    S_AXI_ACP_WID : in std_logic_vector(C_S_AXI_ACP_ID_WIDTH-1 downto 0);
    S_AXI_ACP_ARADDR : in std_logic_vector(C_AXI_ADDR_WIDTH-1 downto 0);
    S_AXI_ACP_AWADDR : in std_logic_vector(C_AXI_ADDR_WIDTH-1 downto 0);
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
    S_AXI_HP0_BID : out std_logic_vector(C_S_AXI_HP0_ID_WIDTH-1 downto 0);
    S_AXI_HP0_RID : out std_logic_vector(C_S_AXI_HP0_ID_WIDTH-1 downto 0);
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
    S_AXI_HP0_ARADDR : in std_logic_vector(C_AXI_ADDR_WIDTH-1 downto 0);
    S_AXI_HP0_AWADDR : in std_logic_vector(C_AXI_ADDR_WIDTH-1 downto 0);
    S_AXI_HP0_ARCACHE : in std_logic_vector(3 downto 0);
    S_AXI_HP0_ARLEN : in std_logic_vector(3 downto 0);
    S_AXI_HP0_ARQOS : in std_logic_vector(3 downto 0);
    S_AXI_HP0_AWCACHE : in std_logic_vector(3 downto 0);
    S_AXI_HP0_AWLEN : in std_logic_vector(3 downto 0);
    S_AXI_HP0_AWQOS : in std_logic_vector(3 downto 0);
    S_AXI_HP0_ARID : in std_logic_vector(C_S_AXI_HP0_ID_WIDTH-1 downto 0);
    S_AXI_HP0_AWID : in std_logic_vector(C_S_AXI_HP0_ID_WIDTH-1 downto 0);
    S_AXI_HP0_WID : in std_logic_vector(C_S_AXI_HP0_ID_WIDTH-1 downto 0);
    S_AXI_HP0_WDATA : in std_logic_vector(C_S_AXI_HP0_DATA_WIDTH-1 downto 0);
    S_AXI_HP0_WSTRB : in std_logic_vector((C_S_AXI_HP0_DATA_WIDTH/8)-1 downto 0);

    S_AXI_HP1_ARESETN : out std_logic;
    S_AXI_HP1_ARREADY : out std_logic;
    S_AXI_HP1_AWREADY : out std_logic;
    S_AXI_HP1_BVALID : out std_logic;
    S_AXI_HP1_RLAST : out std_logic;
    S_AXI_HP1_RVALID : out std_logic;
    S_AXI_HP1_WREADY : out std_logic;
    S_AXI_HP1_BRESP : out std_logic_vector(1 downto 0);
    S_AXI_HP1_RRESP : out std_logic_vector(1 downto 0);
    S_AXI_HP1_BID : out std_logic_vector(C_S_AXI_HP1_ID_WIDTH-1 downto 0);
    S_AXI_HP1_RID : out std_logic_vector(C_S_AXI_HP1_ID_WIDTH-1 downto 0);
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
    S_AXI_HP1_ARADDR : in std_logic_vector(C_AXI_ADDR_WIDTH-1 downto 0);
    S_AXI_HP1_AWADDR : in std_logic_vector(C_AXI_ADDR_WIDTH-1 downto 0);
    S_AXI_HP1_ARCACHE : in std_logic_vector(3 downto 0);
    S_AXI_HP1_ARLEN : in std_logic_vector(3 downto 0);
    S_AXI_HP1_ARQOS : in std_logic_vector(3 downto 0);
    S_AXI_HP1_AWCACHE : in std_logic_vector(3 downto 0);
    S_AXI_HP1_AWLEN : in std_logic_vector(3 downto 0);
    S_AXI_HP1_AWQOS : in std_logic_vector(3 downto 0);
    S_AXI_HP1_ARID : in std_logic_vector(C_S_AXI_HP1_ID_WIDTH-1 downto 0);
    S_AXI_HP1_AWID : in std_logic_vector(C_S_AXI_HP1_ID_WIDTH-1 downto 0);
    S_AXI_HP1_WID : in std_logic_vector(C_S_AXI_HP1_ID_WIDTH-1 downto 0);
    S_AXI_HP1_WDATA : in std_logic_vector(C_S_AXI_HP1_DATA_WIDTH-1 downto 0);
    S_AXI_HP1_WSTRB : in std_logic_vector((C_S_AXI_HP1_DATA_WIDTH/8)-1 downto 0);

    S_AXI_HP2_ARESETN : out std_logic;
    S_AXI_HP2_ARREADY : out std_logic;
    S_AXI_HP2_AWREADY : out std_logic;
    S_AXI_HP2_BVALID : out std_logic;
    S_AXI_HP2_RLAST : out std_logic;
    S_AXI_HP2_RVALID : out std_logic;
    S_AXI_HP2_WREADY : out std_logic;
    S_AXI_HP2_BRESP : out std_logic_vector(1 downto 0);
    S_AXI_HP2_RRESP : out std_logic_vector(1 downto 0);
    S_AXI_HP2_BID : out std_logic_vector(C_S_AXI_HP2_ID_WIDTH-1 downto 0);
    S_AXI_HP2_RID : out std_logic_vector(C_S_AXI_HP2_ID_WIDTH-1 downto 0);
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
    S_AXI_HP2_ARADDR : in std_logic_vector(C_AXI_ADDR_WIDTH-1 downto 0);
    S_AXI_HP2_AWADDR : in std_logic_vector(C_AXI_ADDR_WIDTH-1 downto 0);
    S_AXI_HP2_ARCACHE : in std_logic_vector(3 downto 0);
    S_AXI_HP2_ARLEN : in std_logic_vector(3 downto 0);
    S_AXI_HP2_ARQOS : in std_logic_vector(3 downto 0);
    S_AXI_HP2_AWCACHE : in std_logic_vector(3 downto 0);
    S_AXI_HP2_AWLEN : in std_logic_vector(3 downto 0);
    S_AXI_HP2_AWQOS : in std_logic_vector(3 downto 0);
    S_AXI_HP2_ARID : in std_logic_vector(C_S_AXI_HP2_ID_WIDTH-1 downto 0);
    S_AXI_HP2_AWID : in std_logic_vector(C_S_AXI_HP2_ID_WIDTH-1 downto 0);
    S_AXI_HP2_WID : in std_logic_vector(C_S_AXI_HP2_ID_WIDTH-1 downto 0);
    S_AXI_HP2_WDATA : in std_logic_vector(C_S_AXI_HP2_DATA_WIDTH-1 downto 0);
    S_AXI_HP2_WSTRB : in std_logic_vector((C_S_AXI_HP2_DATA_WIDTH/8)-1 downto 0);

    S_AXI_HP3_ARESETN : out std_logic;
    S_AXI_HP3_ARREADY : out std_logic;
    S_AXI_HP3_AWREADY : out std_logic;
    S_AXI_HP3_BVALID : out std_logic;
    S_AXI_HP3_RLAST : out std_logic;
    S_AXI_HP3_RVALID : out std_logic;
    S_AXI_HP3_WREADY : out std_logic;
    S_AXI_HP3_BRESP : out std_logic_vector(1 downto 0);
    S_AXI_HP3_RRESP : out std_logic_vector(1 downto 0);
    S_AXI_HP3_BID : out std_logic_vector(C_S_AXI_HP3_ID_WIDTH-1 downto 0);
    S_AXI_HP3_RID : out std_logic_vector(C_S_AXI_HP3_ID_WIDTH-1 downto 0);
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
    S_AXI_HP3_ARADDR : in std_logic_vector(C_AXI_ADDR_WIDTH-1 downto 0);
    S_AXI_HP3_AWADDR : in std_logic_vector(C_AXI_ADDR_WIDTH-1 downto 0);
    S_AXI_HP3_ARCACHE : in std_logic_vector(3 downto 0);
    S_AXI_HP3_ARLEN : in std_logic_vector(3 downto 0);
    S_AXI_HP3_ARQOS : in std_logic_vector(3 downto 0);
    S_AXI_HP3_AWCACHE : in std_logic_vector(3 downto 0);
    S_AXI_HP3_AWLEN : in std_logic_vector(3 downto 0);
    S_AXI_HP3_AWQOS : in std_logic_vector(3 downto 0);
    S_AXI_HP3_ARID : in std_logic_vector(C_S_AXI_HP3_ID_WIDTH-1 downto 0);
    S_AXI_HP3_AWID : in std_logic_vector(C_S_AXI_HP3_ID_WIDTH-1 downto 0);
    S_AXI_HP3_WID : in std_logic_vector(C_S_AXI_HP3_ID_WIDTH-1 downto 0);
    S_AXI_HP3_WDATA : in std_logic_vector(C_S_AXI_HP3_DATA_WIDTH-1 downto 0);
    S_AXI_HP3_WSTRB : in std_logic_vector((C_S_AXI_HP3_DATA_WIDTH/8)-1 downto 0);

    IRQ_P2F_DMAC_ABORT : out std_logic;
    IRQ_P2F_DMAC0      : out std_logic;
    IRQ_P2F_DMAC1      : out std_logic;
    IRQ_P2F_DMAC2      : out std_logic;
    IRQ_P2F_DMAC3      : out std_logic;
    IRQ_P2F_DMAC4      : out std_logic;
    IRQ_P2F_DMAC5      : out std_logic;
    IRQ_P2F_DMAC6      : out std_logic;
    IRQ_P2F_DMAC7      : out std_logic;
    IRQ_P2F_SMC        : out std_logic;
    IRQ_P2F_QSPI       : out std_logic;
    IRQ_P2F_CTI        : out std_logic;
    IRQ_P2F_GPIO       : out std_logic;
    IRQ_P2F_USB0       : out std_logic;
    IRQ_P2F_ENET0      : out std_logic;
    IRQ_P2F_ENET_WAKE0 : out std_logic;
    IRQ_P2F_SDIO0      : out std_logic;
    IRQ_P2F_I2C0       : out std_logic;
    IRQ_P2F_SPI0       : out std_logic;
    IRQ_P2F_UART0      : out std_logic;
    IRQ_P2F_CAN0       : out std_logic;
    IRQ_P2F_USB1       : out std_logic;
    IRQ_P2F_ENET1      : out std_logic;
    IRQ_P2F_ENET_WAKE1 : out std_logic;
    IRQ_P2F_SDIO1      : out std_logic;
    IRQ_P2F_I2C1       : out std_logic;
    IRQ_P2F_SPI1       : out std_logic;
    IRQ_P2F_UART1      : out std_logic;
    IRQ_P2F_CAN1       : out std_logic;
    IRQ_F2P            : in std_logic_vector(15 downto 0);
    Core0_nFIQ         : in std_logic;
    Core0_nIRQ         : in std_logic;
    Core1_nFIQ         : in std_logic;
    Core1_nIRQ         : in std_logic;

    DMA0_DATYPE  : out std_logic_vector(1 downto 0);
    DMA0_DAVALID : out std_logic;
    DMA0_DRREADY : out std_logic;
    DMA0_RSTN    : out std_logic;
    DMA1_DATYPE  : out std_logic_vector(1 downto 0);
    DMA1_DAVALID : out std_logic;
    DMA1_DRREADY : out std_logic;
    DMA1_RSTN    : out std_logic;
    DMA2_DATYPE  : out std_logic_vector(1 downto 0);
    DMA2_DAVALID : out std_logic;
    DMA2_DRREADY : out std_logic;
    DMA2_RSTN    : out std_logic;
    DMA3_DATYPE  : out std_logic_vector(1 downto 0);
    DMA3_DAVALID : out std_logic;
    DMA3_DRREADY : out std_logic;
    DMA3_RSTN    : out std_logic;
    DMA0_ACLK    : in std_logic;
    DMA0_DAREADY : in std_logic;
    DMA0_DRLAST  : in std_logic;
    DMA0_DRVALID : in std_logic;
    DMA1_ACLK    : in std_logic;
    DMA1_DAREADY : in std_logic;
    DMA1_DRLAST  : in std_logic;
    DMA1_DRVALID : in std_logic;
    DMA2_ACLK    : in std_logic;
    DMA2_DAREADY : in std_logic;
    DMA2_DRLAST  : in std_logic;
    DMA2_DRVALID : in std_logic;
    DMA3_ACLK    : in std_logic;
    DMA3_DAREADY : in std_logic;
    DMA3_DRLAST  : in std_logic;
    DMA3_DRVALID : in std_logic;
    DMA0_DRTYPE  : in std_logic_vector(1 downto 0);
    DMA1_DRTYPE  : in std_logic_vector(1 downto 0);
    DMA2_DRTYPE  : in std_logic_vector(1 downto 0);
    DMA3_DRTYPE  : in std_logic_vector(1 downto 0);

    FCLK_CLK3          : out std_logic;
    FCLK_CLK2          : out std_logic;
    FCLK_CLK1          : out std_logic;
    FCLK_CLK0          : out std_logic;
    FCLK_CLKTRIG3_N    : in std_logic;
    FCLK_CLKTRIG2_N    : in std_logic;
    FCLK_CLKTRIG1_N    : in std_logic;
    FCLK_CLKTRIG0_N    : in std_logic;
    FCLK_RESET3_N      : out std_logic;
    FCLK_RESET2_N      : out std_logic;
    FCLK_RESET1_N      : out std_logic;
    FCLK_RESET0_N      : out std_logic;

    FTMD_TRACEIN_DATA  : in std_logic_vector(31 downto 0);
    FTMD_TRACEIN_VALID : in std_logic;
    FTMD_TRACEIN_CLK   : in std_logic;
    FTMD_TRACEIN_ATID  : in std_logic_vector(3 downto 0);

    FTMT_F2P_TRIG      : in std_logic_vector(3 downto 0);
    FTMT_F2P_TRIGACK   : out std_logic_vector(3 downto 0);
    FTMT_F2P_DEBUG     : in std_logic_vector(31 downto 0);
    FTMT_P2F_TRIGACK   : in std_logic_vector(3 downto 0);
    FTMT_P2F_TRIG      : out std_logic_vector(3 downto 0);
    FTMT_P2F_DEBUG     : out std_logic_vector(31 downto 0);

    FPGA_IDLE_N        : in std_logic;

    EVENT_EVENTO     : out std_logic;
    EVENT_STANDBYWFE : out std_logic_vector(1 downto 0);
    EVENT_STANDBYWFI : out std_logic_vector(1 downto 0);
    EVENT_EVENTI     : in std_logic;

    DDR_ARB      : in std_logic_vector(3 downto 0);
    MIO          : inout std_logic_vector(C_MIO_PRIMITIVE-1 downto 0);

    DDR_CAS_n    : inout std_logic;
    DDR_CKE      : inout std_logic;
    DDR_Clk_n    : inout std_logic;
    DDR_Clk      : inout std_logic;
    DDR_CS_n     : inout std_logic;
    DDR_DRSTB    : inout std_logic;
    DDR_ODT      : inout std_logic;
    DDR_RAS_n    : inout std_logic;
    DDR_WEB      : inout std_logic;
    DDR_BankAddr : inout std_logic_vector(2 downto 0);
    DDR_Addr     : inout std_logic_vector(14 downto 0);
    DDR_VRN      : inout std_logic;
    DDR_VRP      : inout std_logic;
    DDR_DM       : inout std_logic_vector(C_DM_WIDTH-1  downto 0);
    DDR_DQ       : inout std_logic_vector(C_DQ_WIDTH-1  downto 0);
    DDR_DQS_n    : inout std_logic_vector(C_DQS_WIDTH-1 downto 0);
    DDR_DQS      : inout std_logic_vector(C_DQS_WIDTH-1 downto 0);

    PS_SRSTB     : in std_logic;
    PS_CLK       : in std_logic;
    PS_PORB      : in std_logic
    );
  end component;

component zynq_ps is
  port(
    ps_in        : in    pl2ps_t;
    ps_out       : out   ps2pl_t;
    m_axi_gp_in  : in    m_axi_gp_in_array_t(0 to C_M_AXI_GP_COUNT-1);
    m_axi_gp_out : out   m_axi_gp_out_array_t(0 to C_M_AXI_GP_COUNT-1);
    s_axi_gp_in  : in    s_axi_gp_in_array_t(0 to C_S_AXI_GP_COUNT-1);
    s_axi_gp_out : out   s_axi_gp_out_array_t(0 to C_S_AXI_GP_COUNT-1);
    s_axi_hp_in  : in    s_axi_hp_in_array_t(0 to C_S_AXI_HP_COUNT-1);
    s_axi_hp_out : out   s_axi_hp_out_array_t(0 to C_S_AXI_HP_COUNT-1)
    );
end component zynq_ps;

end package zynq_pkg;
