#ifndef HDLZYNQ_H
#define HDLZYNQ_H
#ifdef __cplusplus
namespace OCPI {
  namespace HDL {
    namespace Zynq {
#endif
      const uint32_t GP0_PADDR = 0x40000000;
      const uint32_t FTM_ADDR = 0xF880B000;
      struct FTM {
	uint32_t
	glbctrl,
	  status,
	  control,
	  p2fdbg0,
	  p2fdbg1,
	  p2fdbg2,
	  p2fdbg3,
	  f2pdbg0,
	  f2pdbg1,
	  f2pdbg2,
	  f2pdbg3,
	  cycountpre,
	  syncreload,
	  synccount,
	  pad0[(0x400-0x34-4)/4],
	  atid,
	  pad1[(0xed0-0x400-4)/4],
	  ittrigoutack,
	  ittrigger,
	  ittracedis,
	  itcyccount,
	  pad2[(0xeec-0xedc-4)/4],
	  itatbdata0,
	  itatbctr2,
	  itatbctr1,
	  itatbctr0,
	  pad3[(0xf00-0xef8-4)/4],	
	  itcr,
	  pad4[(0xfa0-0xf00-4)/4],
	  claimtagset,
	  claimtagclr,
	  pad5[(0xfb0-0xfa4-4)/4],
	  lock_access,
	  lock_status,
	  authstatus,
	  pad6[(0xfc8-0xfb8-4)/4],
	  devid,
	  dev_type,
	  periphid4,
	  periphid5,
	  periphid6,
	  periphid7,
	  periphid0,
	  periphid1,
	  periphid2,
	  periphid3,
	  componid0,
	  componid1,
	  componid2,
	  componid3;
      };
      struct FCLK {
	uint32_t
	clk_ctrl,
	  thr_ctrl,
	  thr_count,
	  thr_sta;
      };
#define NFCLKS 4
      //      const unsigned NFCLKS = 4;
      const uint32_t SLCR_ADDR = 0xf8000000;
      struct SLCR {
	uint32_t
	scl,
	  slcr_lock,
	  slcr_unlock,
	  slcr_locksta,
	  pad0[(0x100 - 0xc - 4)/4],
	  arm_pll_ctrl,
	  ddr_pll_ctrl,
	  io_pll_ctrl,
	  pll_status,
	  arm_pll_cfg,
	  ddr_pll_cfg,
	  io_pll_cfg,
	  pad1,
	  arm_clk_ctrl,
	  ddr_clk_ctrl,
	  dci_clk_ctrl,
	  aper_clk_ctrl,
	  usb0_clk_ctrl,
	  usb1_clk_ctrl,
	  gem0_rclk_ctrl,
	  gem1_rclk_ctrl,
	  gem0_clk_ctrl,
	  gem1_clk_ctrl,
	  smc_clk_ctrl,
	  lqspi_clk_ctrl,
	  sdio_clk_ctrl,
	  uart_clk_ctrl,
	  spi_clk_ctrl,
	  can_clk_ctrl,
	  can_mioclk_ctrl,
	  dbg_clk_ctrl,
	  pcap_clk_ctrl,
	  topsw_clk_ctrl;
	struct FCLK fpga[NFCLKS];
	uint32_t
	  pad2[(0x1c4-0x1ac-4)/4],
	  clk_621_true,
	  pad3[(0x200-0x1c4-4)/4],
	  pss_rst_ctrl,
	  ddr_rst_ctrl,
	  topsw_rst_ctrl,
	  dmac_rst_ctrl,
	  usb_rst_ctrl,
	  gem_rst_ctrl,
	  sdio_rst_ctrl,
	  spi_rst_ctrl,
	  can_rst_ctrl,
	  i2c_rst_ctrl,
	  uart_rst_ctrl,
	  gpio_rst_ctrl,
	  lqspi_rst_ctrl,
	  smc_rst_ctrl,
	  ocm_rst_ctrl,
          pad3a,
	  fpga_rst_ctrl,
	  a9_cpu_rst_ctrl,
	  pad4[(0x24c-0x244-4)/4],
	  rs_awdt_ctrl,
	  pad5[(0x258-0x24c-4)/4],
	  reboot_status,
	  boot_mode,
	  pad6[(0x300-0x25c-4)/4],
	  apu_ctrl,
	  wdt_clk_sel,
	  pad7[(0x440-0x304-4)/4],
	  tz_dma_ns,
	  tz_dma_irq_ns,
	  tz_dma_periph_ns,
	  pad8[(0x530-0x448-4)/4],
	  pss_idcode;
      };
#ifdef __cplusplus
    }
  }
}
#endif
#endif
