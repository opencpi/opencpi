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

 /*
  * This file contains support for the HDL device in the PL on the Xilinx Zynq platform.
  * On Zynq, the control plane is implemented using the M_AXI_GP0 or M_AXI_GP1 port, which
  * is located at physical address 0x4000000 or 0x80000000.
  * The data plane is implemented with the AXI_HP0-3 and other ports, acting
  * as bus masters only.
  */
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include "ocpi-config.h"
#include "HdlZynq.h"
#include "HdlBusDriver.h"
#include "zlib.h"
#ifdef OCPI_OS_macos
#define mmap64 mmap
#define off64_t off_t
#endif

 namespace OCPI {
   namespace HDL {
     namespace Zynq {
       namespace OU = OCPI::Util;

       class Device
	 : public OCPI::HDL::Device {
	 Driver    &m_driver;
	 uint8_t  *m_vaddr;
	 friend class Driver;
         struct Xld { // struct allocated on the stack for easy cleanup
           int xfd, bfd;
           gzFile gz;
           uint8_t buf[8*1024];
           int zerror;
           size_t len;
           int n;
           void cleanup() { // used for constructor catch cleanup and in destructor
             if (xfd >= 0) ::close(xfd);
             if (bfd >= 0) ::close(bfd);
             if (gz) gzclose(gz);
           }
           Xld(const char *file, std::string &a_error) : xfd(-1), bfd(-1), gz(NULL) {
             // Open the device LAST since just opening it will do bad things
             uint8_t *p8;
             if ((bfd = ::open(file, O_RDONLY)) < 0)
               OU::format(a_error, "Can't open bitstream file '%s' for reading: %s(%d)",
                          file, strerror(errno), errno);
             else if ((gz = ::gzdopen(bfd, "rb")) == NULL)
               OU::format(a_error, "Can't open compressed bitstream file '%s' for : %s(%u)",
                          file, strerror(errno), errno);
             // Read up to the sync pattern before byte swapping
             else if ((n = ::gzread(gz, buf, sizeof(buf))) <= 0)
               OU::format(a_error, "Error reading initial bitstream buffer: %s(%u/%d)",
                          gzerror(gz, &zerror), errno, n);
             else if (!(p8 = findsync(buf, sizeof(buf))))
               OU::format(a_error, "Can't find sync pattern in compressed bit file");
             else {
               len = buf + sizeof(buf) - p8;
               if (p8 != buf)
                 memcpy(buf, p8, len);
               // We've done as much as we can before opening the device, which
               // does bad things to the Zynq PL
               //if ((xfd = ::open("/dev/xdevcfg", O_RDWR)) < 0)
                // OU::format(a_error, "Can't open /dev/xdevcfg for bitstream loading: %s(%d)",
                //            strerror(errno), errno);
             }
           }
           ~Xld() {
             cleanup();
           }
           int gzread(uint8_t *&argBuf, std::string &a_error) {
             if ((n = ::gzread(gz, buf + len, (unsigned)(sizeof(buf) - len))) < 0)
               OU::format(a_error, "Error reading compressed bitstream: %s(%u/%d)",
                          gzerror(gz, &zerror), errno, n);
             else {
               n += OCPI_UTRUNCATE(int, len);
               len = 0;
               argBuf = buf;
             }
             return n;
           }
           uint32_t readConfigReg(unsigned /*reg*/) {
             // write the "read config register" frame.
             // read back the value
             return 0;
           }
         }; // struct Xld
	 Device(Driver &driver, std::string &a_name, bool forLoad, const OU::PValue *params,
		std::string &err)
	   : OCPI::HDL::Device(a_name, "ocpi-dma-pio", params),
	     m_driver(driver), m_vaddr(NULL) {
	   m_isAlive = false;
	   m_endpointSize = sizeof(OccpSpace);
	   if (isProgrammed(err)) {
	     if (init(err)) {
	       if (forLoad)
		 err.clear();
	     }
	   } else if (err.empty())
	       ocpiInfo("There is no bitstream loaded on this HDL device: %s", a_name.c_str());
	 }
	 ~Device() {
	   std::string ignore;
	   if (m_vaddr)
	     m_driver.unmap(m_vaddr, sizeof(OccpSpace), ignore);
	 }

	 bool
	 configure(ezxml_t config, std::string &err) {
	   if (!m_isAlive) {
	     volatile SLCR *slcr =
	       (volatile SLCR *)m_driver.map(sizeof(SLCR), SLCR_ADDR, err);
	     if (!slcr)
	       return true;
             // this isnt working on the zynqmp why ?
             //volatile IDCODE *idcode =
             //  (volatile IDCODE *)m_driver.map(sizeof(IDCODE), IDCODE_ADDR, err);
  	     //if (!idcode)
  	     //  return true;
	     // We're not loaded, but fake as much stuff as possible.
	     const char *p = ezxml_cattr(config, "platform");
	     m_platform = p ? p : "zed"; // FIXME: is there any other automatic way for this?
#if defined(OCPI_ARCH_arm64)
             volatile uint32_t *zynqmp_slcr =
	       (volatile uint32_t *)m_driver.map(sizeof(uint32_t), FPD_SLCR_ADDR, err);
             *zynqmp_slcr = 0;  // set gp0/1 to 32bit mode
             m_driver.unmap((uint8_t *)zynqmp_slcr, sizeof(uint32_t), err);

             volatile USP_AXI_HP *axi_hp =
               (volatile USP_AXI_HP *)m_driver.map(sizeof(USP_AXI_HP), USP_AXI_HP_ADDR, err);
             axi_hp->afifm[0].rdctrl = 0xB1;
             axi_hp->afifm[0].wrctrl = 0xB1;
             axi_hp->afifm[1].rdctrl = 0xB1;
             axi_hp->afifm[1].wrctrl = 0xB1;
             axi_hp->afifm[2].rdctrl = 0xB1;
             axi_hp->afifm[2].wrctrl = 0xB1;
             axi_hp->afifm[3].rdctrl = 0xB1;
             axi_hp->afifm[3].wrctrl = 0xB1;
             m_driver.unmap((uint8_t *)axi_hp, sizeof(USP_AXI_HP), err);

             // what is the automatic way to determine this?
             // 0xFFCA0040 returns a bus error I would have thought this was it
             if (m_platform == "zcu102") {
               m_part = "xczu9eg";
             }
             else if (m_platform == "zcu111") {
               m_part = "xczu28dr";
             }
             else {
               m_part = "xczu9eg";
             }
#else
	     switch ((slcr->pss_idcode >> 12) & 0x1f) {
	     case 0x02: m_part = "xc7z010"; break;
	     case 0x07: m_part = "xc7z020"; break;
	     case 0x0c: m_part = "xc7z030"; break;
	     case 0x11: m_part = "xc7z045"; break;
	     default:
	       m_part = "xc7zXXX";

	     }
             ocpiDebug("Zynq SLCR PSS_IDCODE: 0x%x", slcr->pss_idcode);
#endif
	     return m_driver.unmap((uint8_t *)slcr, sizeof(SLCR), err);
	   }
	  return OCPI::HDL::Device::configure(config, err);
	}
	bool
	init(std::string &err) {
	  ocpiDebug("Setting up the Zynq PL");
          volatile FTM *ftm = (volatile FTM *)m_driver.map(sizeof(FTM), FTM_ADDR, err);
#if defined(OCPI_ARCH_arm) || defined(OCPI_ARCH_arm_cs)
          if (!ftm)
            return true;
          ocpiDebug("Debug register 3 from Zynq FTM is 0x%x", ftm->f2pdbg3);
          // Find out whether the OpenCPI control plane is available at GP0 or GP1, GP0 first
          bool useGP1 = (ftm->f2pdbg3 & 0x80) != 0;
	  uint32_t gpAddr = useGP1 ? GP1_PADDR : GP0_PADDR;
#else
          uint32_t gpAddr = MP_GP0_PADDR;
#endif
	  if (m_driver.unmap((uint8_t *)ftm, sizeof(FTM), err) ||
	      (m_vaddr && m_driver.unmap((uint8_t *)m_vaddr, sizeof(OccpSpace), err)) ||
	      !(m_vaddr = m_driver.map(sizeof(OccpSpace), gpAddr, err)))
	    return true;
	  ocpiDebug("Mapping for GP at %p", m_vaddr);
	  cAccess().setAccess(m_vaddr, NULL, OCPI_UTRUNCATE(RegisterOffset, 0));
	  if (OCPI::HDL::Device::init(err))
	    return true;
	  OU::format(m_endpointSpecific,
		     "ocpi-dma-pio:0x%" PRIx32 ".0x%" PRIx32 ".0x%" PRIx32,
		     gpAddr, 0, 0);
	  dAccess().setAccess(NULL, NULL, 0); // the data space will never be accessed by CPU
	  m_isAlive = true;
	  return false;
	}
        // return true if programmed, false if not programmed
        // when false, err may be set or not
        bool
        isProgrammed(std::string &err) {
          std::string val;
#if defined(OCPI_ARCH_arm) || defined(OCPI_ARCH_arm_cs)
          const char *e = OU::file2String(val, "/sys/class/xdevcfg/xdevcfg/device/prog_done", '|');
          bool ret_val = val.c_str()[0] == '1';
          ocpiDebug("OCPI::HDL::Zynq::Device::isProgrammed: got %s%s (%s)", e ? "error: " : "",
            e ? e : (ret_val ? "done" : "not done"), val.c_str());
#else
          //TODO this always seems to return operating even if it isnt loaded
          //     the new way to determine if it was successfully loaded seems to
          //     be to capture the output of the loading command which wont help
          //     us here
          const char *e = OU::file2String(val, "/sys/class/fpga_manager/fpga0/state", '|');
          bool ret_val = val == "operating";
          ocpiDebug("OCPI::HDL::Zynq::Device::isProgrammed: got %s%s (%s)", e ? "error: " : "",
            e ? e : (ret_val ? "operating" : "not operating"), val.c_str());
#endif
          if (e) {
            err = e;
            return false;
          }
          return ret_val;
        }
	bool getMetadata(std::vector<char> &xml, std::string &err) {
	  if (isProgrammed(err))
	    return OCPI::HDL::Device::getMetadata(xml, err);
	  if (err.empty())
	    OU::format(err,
		       "There is no bitstream loaded on this HDL device: %s", name().c_str());
	  return true;
	}
	// The zynq setup does not provide a slave interface to the DMA BRAM,
	// since the SDP is only attached as master to the S_AXI_HP ports.
	// Thus we only allow ActiveMessage since the SDP BRAMs are not memory mapped.
	// (M_AXI_GP0/1 is dedicated to the control plane).
	uint32_t dmaOptions(ezxml_t /*icImplXml*/, ezxml_t /*icInstXml*/, bool isProvider) {
	  return isProvider ?
	    (1 << OCPI::RDT::ActiveMessage) | (1 << OCPI::RDT::FlagIsMeta) :
	    (1 << OCPI::RDT::ActiveMessage) | (1 << OCPI::RDT::FlagIsMetaOptional);
	}

	// Scan the buffer and identify the start of the sync pattern
	static uint8_t *findsync(uint8_t *buf, size_t len) {
	  static uint8_t startup[] = {
	    0xff, 0xff, 0xff, 0xff,
	    0xff, 0xff, 0xff, 0xff,
	    0xff, 0xff, 0xff, 0xff,
	    0xff, 0xff, 0xff, 0xff,
	    0xff, 0xff, 0xff, 0xff,
	    0xff, 0xff, 0xff, 0xff,
	    0xff, 0xff, 0xff, 0xff,
	    0xff, 0xff, 0xff, 0xff,
	    0x00, 0x00, 0x00, 0xbb,
	    0x11, 0x22, 0x00, 0x44,
	    0xff, 0xff, 0xff, 0xff,
	    0xff, 0xff, 0xff, 0xff,
	    0xaa, 0x99, 0x55, 0x66};
	  uint8_t *p8 = startup;
	  for (uint8_t *u8 = buf; u8 < buf + len; u8++)
	    if (*u8 == *p8++) {
	      if (p8 >= startup+sizeof(startup))
		return u8 + 1 - sizeof(startup);
	    } else {
	      p8 = startup;
	      if (*u8 == *p8) p8++;
	    }
	  return 0;
	}

        inline bool file_exists (const std::string& fileName) {
          struct stat my_struct;
          return (stat (fileName.c_str(), &my_struct) == 0);
        }

        bool
        load_fpga_manager(const char *fileName, std::string &error) {
          if (!file_exists("/lib/firmware")){
            mkdir("/lib/firmware",0666);
          }
          int out_file = creat("/lib/firmware/opencpi_temp.bin", 0666);
          gzFile bin_file;
          int bfd, zerror;
          uint8_t buf[8*1024];

          if ((bfd = ::open(fileName, O_RDONLY)) < 0)
            OU::format(error, "Can't open bitstream file '%s' for reading: %s(%d)",
                       fileName, strerror(errno), errno);
          if ((bin_file = ::gzdopen(bfd, "rb")) == NULL)
            OU::format(error, "Can't open compressed bin file '%s' for : %s(%u)",
                       fileName, strerror(errno), errno);
          do {
	    uint8_t *bit_buf = buf;
            int n = ::gzread(bin_file, bit_buf, sizeof(buf));
	    if (n < 0)
	      return true;
	    if (n & 3)
	      return OU::eformat(error, "Bitstream data in is '%s' not a multiple of 3 bytes",
				 fileName);
            if (n == 0)
              break;
	    if (write(out_file, buf, n) <= 0)
	      return OU::eformat(error,
				 "Error writing to /lib/firmware/opencpi_temp.bin for bin loading: %s(%u/%d)",
				 strerror(errno), errno, n);
	  } while (1);
          close(out_file);
          std::ofstream fpga_flags("/sys/class/fpga_manager/fpga0/flags");
          std::ofstream fpga_firmware("/sys/class/fpga_manager/fpga0/firmware");
          fpga_flags << 0 << std::endl;
          fpga_firmware << "opencpi_temp.bin" << std::endl;

          remove("/lib/firmware/opencpi_temp.bin");
          return isProgrammed(error) ? init(error) : true;
        }

	// Load a bitstream
	bool
	load_xdevconfig(const char *fileName, std::string &error) {
          ocpiDebug("Loading file \"%s\" on zynq FPGA", fileName);
      	  struct Xld { // struct allocated on the stack for easy cleanup
      	    int xfd, bfd;
      	    gzFile gz;
      	    uint8_t buf[8*1024];
      	    int zerror;
      	    size_t len;
      	    int n;
      	    void cleanup() { // used for constructor catch cleanup and in destructor
      	      if (xfd >= 0) ::close(xfd);
      	      if (bfd >= 0) ::close(bfd);
      	      if (gz) gzclose(gz);
      	    }
      	    Xld(const char *file, std::string &a_error) : xfd(-1), bfd(-1), gz(NULL) {
      	      // Open the device LAST since just opening it will do bad things
      	      uint8_t *p8;
      	      if ((bfd = ::open(file, O_RDONLY)) < 0)
      		OU::format(a_error, "Can't open bitstream file '%s' for reading: %s(%d)",
      			   file, strerror(errno), errno);
      	      else if ((gz = ::gzdopen(bfd, "rb")) == NULL)
      		OU::format(a_error, "Can't open compressed bitstream file '%s' for : %s(%u)",
      			   file, strerror(errno), errno);
      	      // Read up to the sync pattern before byte swapping
      	      else if ((n = ::gzread(gz, buf, sizeof(buf))) <= 0)
      		OU::format(a_error, "Error reading initial bitstream buffer: %s(%u/%d)",
      			   gzerror(gz, &zerror), errno, n);
      	      else if (!(p8 = findsync(buf, sizeof(buf))))
      		OU::format(a_error, "Can't find sync pattern in compressed bit file");
      	      else {
      		len = buf + sizeof(buf) - p8;
      		if (p8 != buf)
      		  memcpy(buf, p8, len);
      		// We've done as much as we can before opening the device, which
      		// does bad things to the Zynq PL
      		if ((xfd = ::open("/dev/xdevcfg", O_RDWR)) < 0)
      		  OU::format(a_error, "Can't open /dev/xdevcfg for bitstream loading: %s(%d)",
      			     strerror(errno), errno);
      	      }
      	    }
      	    ~Xld() {
      	      cleanup();
      	    }
      	    int gzread(uint8_t *&argBuf, std::string &a_error) {
      	      if ((n = ::gzread(gz, buf + len, (unsigned)(sizeof(buf) - len))) < 0)
      		OU::format(a_error, "Error reading compressed bitstream: %s(%u/%d)",
      			   gzerror(gz, &zerror), errno, n);
      	      else {
      		n += OCPI_UTRUNCATE(int, len);
      		len = 0;
      		argBuf = buf;
      	      }
      	      return n;
      	    }
      	    uint32_t readConfigReg(unsigned /*reg*/) {
      	      // write the "read config register" frame.
      	      // read back the value
      	      return 0;
      	    }
      	  };
      	  Xld xld(fileName, error);
      	  if (!error.empty())
      	    return true;
      	  do {
      	    uint8_t *buf;
      	    int n = xld.gzread(buf, error);
      	    if (n < 0)
      	      return true;
      	    if (n & 3)
      	      return OU::eformat(error, "Bitstream data in is '%s' not a multiple of 4 bytes",
      				 fileName);
      	    if (n == 0)
      	      break;
      	    uint32_t *p32 = (uint32_t*)buf;
      	    for (unsigned nn = n; nn; nn -= 4, p32++)
      	      *p32 = OU::swap32(*p32);
      	    if (write(xld.xfd, buf, n) <= 0)
      	      return OU::eformat(error,
      				 "Error writing to /dev/xdevcfg for bitstream loading: %s(%u/%d)",
      				 strerror(errno), errno, n);
      	  } while (1);
      	  if (::close(xld.xfd))
      	    return OU::eformat(error, "Error closing /dev/xdevcfg: %s(%u)",
      			       strerror(errno), errno);
      	  xld.xfd = -1;
      	  ocpiDebug("Loading complete, testing for programming done and initialization");
      	  return isProgrammed(error) ? init(error) : true;
      #if 0
      	  // We have written all the data from the file to the device.
      	  // Now we can retrieve status registers
      	  const uint32_t
      	    c_opencpi = 0xdd9fda7a,
      	    c_statmask = 0x12345678,
      	    c_statvalue = 0x12345678;
      	  uint32_t
      	    stat = xld.readConfigReg(7),
      	    axss = xld.readConfigReg(9);
      	  // Check stat for good value
      	  if ((stat & c_statmask) != c_statvalue)
      	    throw OU::Error("After loading, the configuration status register was 0x%x,"
      			    " but should be 0x%x (masked with 0x%x)",
      			    stat, c_statvalue, c_statmask);
      	  // Check axss for our special value 0xdd9fda7a
      	  if (axss != c_opencpi)
      	    throw OU::Error("After loading, the USR_ACCESS code was not correct: 0x%x - should be 0x%x",
      			    axss, c_opencpi);
      #endif
	}

        bool
        load(const char *fileName, std::string &error){
          bool ret_val = false;

          if (file_exists("/dev/xdevcfg")){
            ret_val= load_xdevconfig(fileName, error);
          }
          else if (file_exists("/sys/class/fpga_manager/fpga0/")){
            ret_val= load_fpga_manager(fileName, error);
          }
          return ret_val;
        }

	bool
	unload(std::string &error) {
	  int xfd;
	  if ((xfd = ::open("/dev/xdevcfg", O_WRONLY)) < 0)
	    return OU::eformat(error, "Can't open /dev/xdevcfg for bitstream unloading: %s(%d)",
			       strerror(errno), errno);
	  close(xfd);
	  return false;
	}
      };

      Driver::
      Driver()
	: m_memFd(-1) {
      }
      Driver::
      ~Driver() {
	if (m_memFd >= 0)
	  ::close(m_memFd);
      }

      unsigned Driver::
      search(const OU::PValue *params, const char **exclude, bool discoveryOnly,
	     std::string &error) {
	// Opening implies canonicalizing the name, which is needed for excludes
	ocpiInfo("Searching for local Zynq/PL HDL device.");
#if 0
	bool verbose = false;
	OU::findBool(params, "verbose", verbose);
	if (verbose)
	  printf("Searching for local Zynq/PL HDL device.\n");
#endif
	OCPI::HDL::Device *dev = open("0", true, params, error);
	return dev && !found(*dev, exclude, discoveryOnly, error) ? 1 : 0;
      }

      OCPI::HDL::Device *Driver::
      open(const char *busName, bool forLoad, const OU::PValue *params, std::string &error) {
	(void)params;
	std::string name("PL:");
	name += busName;
#if defined(OCPI_ARCH_arm) || defined(OCPI_ARCH_arm_cs) || defined(OCPI_ARCH_arm64)
	Device *dev = new Device(*this, name, forLoad, params, error);
	if (error.empty())
	  return dev;
	delete dev;
	ocpiBad("When searching for PL device '%s': %s", busName, error.c_str());
#else
	(void)forLoad;
	error.clear();
#endif
	return NULL;
      }
      uint8_t *Driver::
      map(size_t size, uint32_t offset, std::string &error) {
	void *vaddr;
	off64_t off64 = offset;
	ocpiDebug("Zynq map of offset %" PRIx32 " off64 %" PRIx64 " size %zu",
		  offset, off64, size);
	if (m_memFd < 0 && (m_memFd = ::open("/dev/mem", O_RDWR|O_SYNC)) < 0)
	  error = "Can't open /dev/mem, forgot to load the driver? sudo?";
	else if ((vaddr = mmap64(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, m_memFd,
				 off64)) == MAP_FAILED)
	  error = "can't mmap /dev/mem for control space";
	else {
	  ocpiDebug("Zynq map returns %p", vaddr);
	  return (uint8_t*)vaddr;
	}
	return NULL;
      }
       bool Driver::
       unmap(uint8_t *addr, size_t size, std::string &error) {
	 ocpiDebug("Zynq unmap %p %zu", addr, size);
	 if (m_memFd < 0)
	   error = "Memory device not open for unmap";
	 else if (munmap(static_cast<void*>(addr), size) != 0)
	   OU::format(error, "unmap failure, %s", strerror(errno));
	 else
	   return false;
	 return true;
       }
    } // namespace BUS
  } // namespace HDL
} // namespace OCPI
