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
 * OpenCPI Definitions shared between user programs and the kernel mode.
 */

#ifndef KERNELDRIVER_H_
#define KERNELDRIVER_H_

#ifdef __KERNEL__
#include <asm/ioctl.h>
#else
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#endif

#define OCPI_DRIVER_DIR "/dev/ocpi"
#define OCPI_DRIVER_MEM OCPI_DRIVER_DIR "/mem"
#define OCPI_DRIVER_PCI OCPI_DRIVER_DIR "/pci"
#define OCCP_ETHER_MTYPE 0xf040
#define OCCP_ETHER_STYPE 0xf040
#define OCDP_ETHER_TYPE  0xf042
// -----------------------------------------------------------------------------------------------
// Data structures
// -----------------------------------------------------------------------------------------------

typedef uint64_t ocpi_address_t;
typedef uint32_t ocpi_size_t;
typedef enum {
  ocpi_cached,		// This memory block should be cached
  ocpi_uncached, 	// This memory block should be uncached
} ocpi_cached_t;

typedef struct {
  ocpi_size_t	allocated;	// Total memory allocated by the driver
  ocpi_size_t	available;	// Memory available to be mapped
  ocpi_size_t	largest;	// Largest memory block available
} ocpi_status_t;

typedef struct {
  ocpi_size_t	 needed;		// How much memory you need in this request
  ocpi_size_t	 actual;		// How much memory you will receive
  ocpi_address_t address;	        // The physical address of this block from the point
                                        // of view of the local processor (and /dev/mem).
                                        // In linux terminology a "phys" address
  ocpi_address_t bus_addr;	        // The address usable from another bus master to
                                        // see this local contiguous DMA memory
                                        // In linux terminology a "bus" address
  ocpi_cached_t  how_cached;		// How should this block be cached for local access?
} ocpi_request_t;

typedef struct {
  ocpi_address_t bar0;
  ocpi_address_t bar1;
  ocpi_size_t    size0;
  ocpi_size_t    size1;
} ocpi_pci_t;

typedef enum {
  ocpi_none,
  ocpi_discovery, // I send CP broadcast packets for discovery, only one of these allowed
                  // On receive: deliver NOPs back to this.
  ocpi_master,    // I send and receive specific CP packets to a slave, one per slave addr
                  // On receive: deliver if not-nop and from slave
  ocpi_slave,     // I receive broadcast packets
                  // On receive: deliver if to broadcast.
  ocpi_data,      // I am a DP endpoint
                  // On receive: deliver to me for endpoint
  ocpi_device,    // Not used in the kernel driver, but used for a non-broadcast-receiving slave
                  // when there are multiple devices per address, e.g. with UDP
  ocpi_role_limit
} ocpi_role_t;
#define PF_OPENCPI PF_DECnet // poach temporarily
typedef struct sockaddr_ocpi {
  sa_family_t ocpi_family;      // mandatory
  uint8_t     ocpi_role;        // discover, master, slave, data
  uint8_t     ocpi_ifindex;     // for discover, master, or slave
  uint16_t    ocpi_endpoint;    // for data only
  // This must be last!! (see receive code in driver)
  uint8_t     ocpi_remote[6];   // for master
} ocpi_sockaddr_t;

// -----------------------------------------------------------------------------------------------
// ioctl command definitions
// -----------------------------------------------------------------------------------------------

#define OPENCPI_IOC_MAGIC 213

#define OCPI_CMD_STATUS		_IOR(OPENCPI_IOC_MAGIC,  1, ocpi_status_t)
#define OCPI_CMD_REQUEST	_IOWR(OPENCPI_IOC_MAGIC, 2, ocpi_request_t)
#define OCPI_CMD_PCI            _IOR(OPENCPI_IOC_MAGIC,  3, ocpi_pci_t) 
#define OCPI_CMD_DISCOVER       _IOR(OPENCPI_IOC_MAGIC,  4, unsigned) 

#endif /* OPENCPI_H_ */
