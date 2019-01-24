/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HDLZYNQ_H
#define HDLZYNQ_H
#ifdef __cplusplus
namespace OCPI {
  namespace HDL {
    namespace Zynq {
#endif
      const uint32_t GP0_PADDR = 0x40000000;
      const uint32_t GP1_PADDR = 0x80000000;
      const uint32_t FTM_ADDR = 0xF880B000;
      // register map from Appendix B.12 "PL Fabric Trace Monitor" in:
      // https://www.xilinx.com/support/documentation/user_guides/ug585-Zynq-7000-TRM.pdf
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

      // See "Table 10-8: PS System Register Map" in:
      // https://www.xilinx.com/support/documentation/user_guides/ug1085-zynq-ultrascale-trm.pdf
      const uint32_t USP_AXI_HP_ADDR  = 0xFD380000;
      // UltraScale+ "AFIFM Module" from:
      // https://www.xilinx.com/html_docs/registers/ug1087/ug1087-zynq-ultrascale-registers.html
      struct USP_AFIFM {
	uint32_t                    // Relative Address
	  rdctrl,                   // 0x00000000
	  rdissue,                  // 0x00000004
	  rdqos,                    // 0x00000008
	  pad0[(0x010-0x008-4)/4],
	  rddebug,                  // 0x00000010
	  wrctrl,                   // 0x00000014
	  wrissue,                  // 0x00000018
	  wrqos,                    // 0x0000001C
	  pad1[(0xe00-0x01c-4)/4],
	  i_sts,                    // 0x00000E00
	  i_en,                     // 0x00000E04
	  i_dis,                    // 0x00000E08
	  i_mask,                   // 0x00000E0C
	  pad2[(0xf04-0xe0c-4)/4],
	  control,                  // 0x00000F04
	  pad3[(0xf0c-0xf04-4)/4],
	  safety_chk,               // 0x00000F0C
	  pad4[(0x10000-0x00f0c-4)/4];
      };
      const unsigned USP_NAXI_HPS = 4;
      struct USP_AXI_HP {
	  USP_AFIFM afifm[USP_NAXI_HPS];
      };

#ifdef __cplusplus
    }
  }
}
#endif
#endif
