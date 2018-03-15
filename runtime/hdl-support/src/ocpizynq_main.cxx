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

#define OCPI_OPTIONS_HELP \
  "Usage syntax is: ocpizynq [options] <command>\n" \
  "  Commands are:\n" \
  "    clocks  - print how the clocks are configured\n" \
  "    axi_hp   - print how the axi_hp interfaces are configured\n"

//          name      abbrev  type    value description
#define OCPI_OPTIONS \
  CMD_OPTION  (psclk,     p,    Double,   "33.3333e6", "Frequency of the PS_CLK clock into the Zync SoC") \

#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>
#include "OcpiUtilMisc.h"
#include "CmdOption.h"
#include "HdlZynq.h"

namespace OU = OCPI::Util;
using namespace OCPI::HDL::Zynq;

struct PLL {
  const char *name;
  uint32_t ctrl, cfg, fdiv;
  PLL(const char *a_name, uint32_t a_ctrl, uint32_t a_cfg)
    : name(a_name), ctrl(a_ctrl), cfg(a_cfg), fdiv((ctrl >> 12) & 0x7f) {
    bool bpmode = ((ctrl >> 3) & 1) != 0;
    printf("%3s PLL: FDIV: %u, bypass mode: %s, bypass status: %s,"
	   " power: %s, reset: %s\n",
	   name, fdiv,  bpmode ? "pin" : "reg",
	   (ctrl >> 4)&1 ? "bypassed" : (bpmode ? "pin" : "enabled"),
	   ctrl & 2 ? "off" : "on", ctrl & 1 ? "asserted" : "deasserted");
  }
  double freq(double psclk) { return psclk * fdiv; }
};

static uint8_t *map(off_t addr, size_t arg_size) {
  static int fd = -1;
  if (fd < 0 &&
      (fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0) {
    perror("opening /dev/mem") ;
    return NULL ;
  }
  int pagesize = getpagesize();
  off_t base = addr & ~(pagesize - 1);
  size_t size = OU::roundUp(addr + arg_size - base, pagesize);
  void *ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, base);
  if (ptr == MAP_FAILED) {
    perror("mapping /dev/mem");
    return NULL;
  }
  return ((uint8_t*)ptr) + (addr - base);
}

int
mymain(const char **/*argv*/) {
#if !defined(OCPI_ARCH_arm) && !defined(OCPI_ARCH_arm_cs)
  fprintf(stderr, "This program is only functional on Zynq/Arm platforms\n");
  return 1;
#endif
  std::string cmd = options.argv()[0];
  if (cmd == "test")
    printf("test\n");
  else if (cmd == "clocks") {
    volatile FTM *ftm = (volatile FTM *)map(FTM_ADDR, sizeof(FTM));
    if (!ftm)
      return 1;
    volatile SLCR *slcr = (volatile SLCR *)map(SLCR_ADDR, sizeof(SLCR));
    if (!slcr)
      return 1;
    printf("ftm: glbctrl 0x%x status 0x%x control 0x%x cycountpre 0x%x synccount 0x%x "
	   "itcyccount 0x%x lock_status 0x%x lock_access 0x%x\n",
	   ftm->glbctrl, ftm->status, ftm->control, ftm->cycountpre, ftm->synccount,
	   ftm->itcyccount, ftm->lock_status, ftm->lock_access);
    ftm->lock_access = 0xc5acce55;
    ftm->glbctrl = 1;
    ftm->control = 4;
    ftm->lock_access = 0;
#ifdef OCPI_OS_macos
    uint32_t t0 = 0;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec++;
    uint32_t t0 = ftm->itcyccount;
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL);
#endif
    uint32_t t1 = ftm->itcyccount;
    double freq = (t1 - t0)/2.;
    freq *= slcr->clk_621_true & 1 ? 6 : 4;
    uint32_t ctrl = slcr->arm_clk_ctrl;
    PLL
      arm("ARM", slcr->arm_pll_ctrl, slcr->arm_pll_cfg),
      ddr("DDR", slcr->ddr_pll_ctrl, slcr->ddr_pll_cfg),
      io("IO", slcr->io_pll_ctrl, slcr->io_pll_cfg),
      &pll = ((ctrl & 0x30) >> 4) == 2 ? ddr : 
             ((ctrl & 0x30) >> 4) == 3 ? io : arm;
    uint32_t divisor = (ctrl >> 8) & 0x3f;
    double pssclk = (freq * divisor) / pll.fdiv;
    printf("Apparent CPU frequency is:~ %4.2f\n", (freq + 5000.)/1000000.);
    printf("Apparent PSCLK frequency is: %4.2f\n", (pssclk + 5000.)/1000000.);
    printf("Apparent PLL frequencies are: ARM: %4.2f, DDR: %4.2f, IO: %4.2f\n",
	   (arm.freq(pssclk) + 5000.)/1000000.,
	   (ddr.freq(pssclk) + 5000.)/1000000.,
	   (io.freq(pssclk) + 5000.)/1000000.);
    bool x6 = (slcr->clk_621_true & 1) != 0;
    double topfreq = pll.freq(pssclk) / divisor;
    double
      x1 = topfreq / (x6 ? 6 : 4),
      x2 = x1 * 2,
      x3x2 = x1 * (x6 ? 3 : 2);
    printf("ARM CPU Clocks: source: %s, mode: %s, divisor: %u, freq: %4.2f MHz\n",
	   pll.name, x6 ? "6:3:2:1" : "4:2:2:1", divisor,
	   (topfreq + 5000.) / 1000000);
    printf("  Enabled: peri: %s, 1x: %s %4.2f, 2x: %s %4.2f, 3x2x: %s %4.2f, 6x4x: %s %4.2f\n",
	   (ctrl & (1 << 28)) ? "on" : "off",
	   (ctrl & (1 << 27)) ? "on" : "off",
	   (x1 + 5000.)/1000000.,
	   (ctrl & (1 << 26)) ? "on" : "off",
	   (x2 + 5000.)/1000000.,
	   (ctrl & (1 << 25)) ? "on" : "off",
	   (x3x2 + 5000.)/1000000.,
	   (ctrl & (1 << 24)) ? "on" : "off",
	   (topfreq + 5000.)/1000000.);
    volatile FCLK *fp = slcr->fpga;
    for (unsigned n = 0; n < NFCLKS; n++, fp++) {
      volatile FCLK f = *(FCLK*)fp;
      //    printf("%u %p %08x %08x %08x %08x\n", n, fp,
      //	   f.clk_ctrl, fp->thr_ctrl, fp->thr_count, fp->thr_sta); 
      PLL &fpll =
	((f.clk_ctrl & 0x30) >> 4) == 2 ? arm : 
	((f.clk_ctrl & 0x30) >> 4) == 3 ? ddr : io;
      uint32_t
	divisor0 = (f.clk_ctrl >> 8) & 0x3f,
	divisor1 = (f.clk_ctrl >> 20) & 0x3f;

      printf("FCLK %u: source: %s PLL, divisor0: %2u, divisor1: %2u",
	     n, fpll.name, divisor0, divisor1);
      printf(", throttling %sreset, throttling %sstarted",
	     f.thr_ctrl & 2 ? "is " : "not ", f.thr_ctrl & 1 ? "is " : "not ");
      if (f.thr_count & 0xffff)
	printf(", last count: %u", f.thr_count & 0xffff);
      printf(", mode: %s", f.thr_sta & 0x10000 ? "debug/static" : "stopped/normal");
      if (f.thr_sta & 0xffff)
	printf(", current count: %u", f.thr_sta & 0xffff);
      printf(", frequency: %4.2f MHz\n",
	     ((((options.psclk() * fpll.fdiv) / divisor0 ) / divisor1) + 5000) / 1000000.);
    }
  } else if (cmd == "axi_hp") {
    struct AFI {
      uint32_t
        rdchan_ctrl,
        rdchan_issuingcap,
        rdqos,
        rddatafifo_level,
        rddebug,
        wrchan_ctrl,
        wrchan_issuingcap,
        wrqos,
        wrdatafifo_level,
        wrdebug,
        pad[(0x1000 - 0x24 - 4)/4];
    };
    const unsigned NAXI_HPS = 4;
    struct AXI_HP {
      AFI afi[NAXI_HPS];
    };
    const uint32_t AXI_HP_ADDR = 0xf8008000;
    volatile AXI_HP *axi_hp = (volatile AXI_HP *)map(AXI_HP_ADDR, sizeof(AXI_HP));
    if (!axi_hp)
      return 1;
    volatile AFI *afi = axi_hp->afi;
    printf("AXI_HP_ADDR 0x%x axi_hp 0x%p afi 0x%p\n", AXI_HP_ADDR, axi_hp, afi);
    sleep(10);
    for (unsigned n = 0; n < NAXI_HPS; n++, afi++) {
#if 1
      printf("AXI_HP %u: rdctrl: 0x%x rdissue: 0x%x rdqos: 0x%x rdfifo: <unread> rddebug: 0x%x\n",
	     n, afi->rdchan_ctrl, afi->rdchan_issuingcap, afi->rdqos,
	     /*afi->rddatafifo_level,*/ afi->rddebug);
      printf("        : wrctrl: 0x%x wrissue: 0x%x wrqos: 0x%x wrfifo: <unread> wrdebug: 0x%x\n",
	     afi->wrchan_ctrl, afi->wrchan_issuingcap, afi->wrqos, /*afi->wrdatafifo_level,*/
	     afi->wrdebug);
#else
      printf("AXI_HP %u: rdctrl: 0x%x rdissue: 0x%x rdqos: 0x%x rdfifo: 0x%x rddebug: 0x%x\n",
	     n, afi->rdchan_ctrl, afi->rdchan_issuingcap, afi->rdqos, afi->rddatafifo_level, 0);
#endif
      sleep(10);
    }
  }
  return 0;
}
