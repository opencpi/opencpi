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
      const uint32_t GP0_PADDR = 0xA0000000; //0x40000000
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

      // See "Figure 39-7: CoreSight System Debug Address Map" in:
      // https://www.xilinx.com/support/documentation/user_guides/ug1085-zynq-ultrascale-trm.pdf
      const uint32_t USP_CORESIGHT_ADDR = 0xFE800000;
      const uint32_t USP_FTM_ADDR = USP_CORESIGHT_ADDR + 0x001D0000;

      // See "Table 10-8: PS System Register Map" in:
      // https://www.xilinx.com/support/documentation/user_guides/ug1085-zynq-ultrascale-trm.pdf
      const uint32_t USP_CSU_ADDR       = 0xFFCA0000;

      /*const uint32_t USP_CLAIM_TAG_BIT = 0;  //  CLAIMSET (FTM) Register
      const uint32_t USP_TRIGOUT_BIT = 0;    //  ITTRIGOUT (FTM) Register
      const uint32_t USP_TRIGOUTACK_BIT = 0; //  ITTRIGOUTACK (FTM) Register*/

      // UltraScale+ "FTM Module Register Summary" from UG1087 (v1.6)
      struct USP_FTM {
	uint32_t                  // Relative Address
	  gpi,                    // 0x00000010
	  pad0[(0x020-0x010-4)/4],
	  gpo,                    // 0x00000020
	  pad1[(0xed0-0x020-4)/4],
	  ittrigout,              // 0x00000ED0
	  ittrigoutack,           // 0x00000ED4
	  ittrigin,               // 0x00000ED8
	  ittriginack,            // 0x00000EDC
	  pad2[(0xf00-0xedc-4)/4],
	  itctrl,                 // 0x00000F00
	  pad3[(0xfa0-0xf00-4)/4],
	  claimset,               // 0x00000FA0
	  claimclr,               // 0x00000FA4
	  pad4[(0xfb0-0xfa4-4)/4],
	  lar,                    // 0x00000FB0
	  lsr,                    // 0x00000FB4
	  authstatus,             // 0x00000FB8
	  pad5[(0xfc8-0xfb8-4)/4],
	  devid,                  // 0x00000FC8
	  devtype,                // 0x00000FCC
	  pidr4,                  // 0x00000FD0
	  pidr5,                  // 0x00000FD4
	  pidr6,                  // 0x00000FD8
	  pidr7,                  // 0x00000FDC
	  pidr0,                  // 0x00000FE0
	  pidr1,                  // 0x00000FE4
	  pidr2,                  // 0x00000FE8
	  pidr3,                  // 0x00000FEC
	  cidr0,                  // 0x00000FF0
	  cidr1,                  // 0x00000FF4
	  cidr2,                  // 0x00000FF8
	  cidr3;                  // 0x00000FFC
      };

      // UltraScale+ "Configuration Security Unit" from UG1087 (v1.6)
      struct USP_CSU {
	uint32_t                    // Relative Address
	  csu_status,               // 0x00000000
	  csu_ctrl,                 // 0x00000004
	  csu_sss_cfg,              // 0x00000008
	  csu_dma_reset,            // 0x0000000C
	  csu_multi_boot,           // 0x00000010
	  csu_tamper_trig,          // 0x00000014
	  CSU_FT_STATUS,            // 0x00000018
	  csu_isr,                  // 0x00000020
	  csu_imr,                  // 0x00000024
	  csu_ier,                  // 0x00000028
	  csu_idr,                  // 0x0000002C
	  pad0[(0x0034-0x002c-4)/4],
	  jtag_chain_status,        // 0x00000034
	  jtag_sec,                 // 0x00000038
	  jtag_dap_cfg,             // 0x0000003C
	  IDCODE,                   // 0x00000040
	  version,                  // 0x00000044
	  pad1[(0x0050-0x0044-4)/4],
	  csu_rom_digest_0,         // 0x00000050
	  csu_rom_digest_1,         // 0x00000054
	  csu_rom_digest_2,         // 0x00000058
	  csu_rom_digest_3,         // 0x0000005C
	  csu_rom_digest_4,         // 0x00000060
	  csu_rom_digest_5,         // 0x00000064
	  csu_rom_digest_6,         // 0x00000068
	  csu_rom_digest_7,         // 0x0000006c
	  csu_rom_digest_8,         // 0x00000070
	  csu_rom_digest_9,         // 0x00000074
	  csu_rom_digest_10,        // 0x00000078
	  csu_rom_digest_11,        // 0x0000007C
	  pad2[(0x1000-0x007c-4)/4],
	  aes_status,               // 0x00001000
	  aes_key_src,              // 0x00001004
	  aes_key_load,             // 0x00001008
	  aes_start_msg,            // 0x0000100C
	  aes_reset,                // 0x00001010
	  aes_key_clear,            // 0x00001014
	  aes_cfg,                  // 0x00001018
	  aes_kup_wr,               // 0x0000101C
	  aes_kup_0,                // 0x00001020
	  aes_kup_1,                // 0x00001024
	  aes_kup_2,                // 0x00001028
	  aes_kup_3,                // 0x0000102C
	  aes_kup_4,                // 0x00001030
	  aes_kup_5,                // 0x00001034
	  aes_kup_6,                // 0x00001038
	  aes_kup_7,                // 0x0000103C
	  aes_iv_0,                 // 0x00001040
	  aes_iv_1,                 // 0x00001044
	  aes_iv_2,                 // 0x00001048
	  aes_iv_3,                 // 0x0000104C
	  pad3[(0x2000-0x104c-4)/4],
	  sha_start,                // 0x00002000
	  sha_reset,                // 0x00002004
	  sha_done,                 // 0x00002008
	  sha_digest_0,             // 0x00002010
	  sha_digest_1,             // 0x00002014
	  sha_digest_2,             // 0x00002018
	  sha_digest_3,             // 0x0000201C
	  sha_digest_4,             // 0x00002020
	  sha_digest_5,             // 0x00002024
	  sha_digest_6,             // 0x00002028
	  sha_digest_7,             // 0x0000202C
	  sha_digest_8,             // 0x00002030
	  sha_digest_9,             // 0x00002034
	  sha_digest_10,            // 0x00002038
	  sha_digest_11,            // 0x0000203C
	  pad4[(0x3000-0x203c-4)/4],
	  pcap_prog,                // 0x00003000
	  pcap_rdwr,                // 0x00003004
	  pcap_ctrl,                // 0x00003008
	  pcap_reset,               // 0x0000300C
	  pcap_status,              // 0x00003010
	  pad5[(0x5000-0x3010-4)/4],
	  tamper_status,            // 0x00005000
	  csu_tamper_0,             // 0x00005004
	  csu_tamper_1,             // 0x00005008
	  csu_tamper_2,             // 0x0000500C
	  csu_tamper_3,             // 0x00005010
	  csu_tamper_4,             // 0x00005014
	  csu_tamper_5,             // 0x00005018
	  csu_tamper_6,             // 0x0000501C
	  csu_tamper_7,             // 0x00005020
	  csu_tamper_8,             // 0x00005024
	  csu_tamper_9,             // 0x00005028
	  csu_tamper_10,            // 0x0000502C
	  csu_tamper_11,            // 0x00005030
	  csu_tamper_12;            // 0x00005034
      };

#ifdef __cplusplus
    }
  }
}
#endif
#endif
