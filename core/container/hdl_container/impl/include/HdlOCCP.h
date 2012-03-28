
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OCCP_H
#define OCCP_H
#include <stdint.h>

#ifdef __cplusplus
namespace OCPI {
  namespace HDL {
#endif
    typedef struct {
      uint32_t birthday; // The time the final build was started
      uint8_t uuid[16];  // The official UUID of the bitstream
      char platform[16]; // The name of the platform the bitstream was built for
      char device[16];   // The name of the device the bitstream was built for
      char load[4];      // Information to load this particular device on this platform
      char dna[8];       // The serial number of this particular device
    } HdlUUID;
    typedef struct {
      const uint32_t
        magic1,
        magic2,
        revision,
        birthday,
        config,
        pciDevice,
        attention,
        status;
      uint32_t
        scratch20,
        scratch24,
        control;
      const uint32_t
        mbz0,
	timeStatus;
      uint32_t
        timeControl;
      uint64_t
        time,             //38
        timeDelta;        //40
      const uint32_t
        timeClksPerPps,   //48
	pad[1];           //4c
      const uint64_t dna; // 5054
      const uint32_t
        pad1[9],     // 585c6064686c707478
        numRegions,  // 7c
        regions[16]; // 8084888c9094989ca0a4a8acb0b4b8bc
      HdlUUID uuid;
    } OccpAdminRegisters;
    typedef struct {
      const uint32_t
        initialize,
        start,
        stop,
        release,
        test,
        beforeQuery,
        afterConfigure,
        reserved7,
        status;
      uint32_t
        control;
      const uint32_t
        lastConfig;
      uint32_t
        clearError,
	window,
        reserved[3];
    } OccpWorkerRegisters;
#define OCCP_CONTROL_ENABLE 0x80000000
#define OCCP_WORKER_CONTROL_SIZE 0x10000
#define OCCP_ADMIN_SIZE OCCP_WORKER_CONTROL_SIZE
#define OCCP_WORKER_CONFIG_SIZE 0x100000
#define OCCP_WORKER_CONTROL_BASE(i) ((i) * OCCP_BYTES_PER_WORKER)
#define OCCP_WORKER_PROPERTY_BASE(i) ((i) * OCCP_BYTES_PER_WORKER)
#define OCCP_MAX_WORKERS 15
#define OCFRP0_VENDOR 0x10ee
#define OCFRP0_DEVICE 0x4243
#define OCFRP0_CLASS 0x05
#define OCFRP0_SUBCLASS 0x00
// Magic values from config space: these values are 32 bit values with chars big endian
#define OCCP_MAGIC1 (('O'<<24)|('p'<<16)|('e'<<8)|'n')
#define OCCP_MAGIC2 (('C'<<24)|('P'<<16)|('I'<<8))
// Read return values from workers
#define OCCP_ERROR_RESULT   0xc0de4202
#define OCCP_TIMEOUT_RESULT 0xc0de4203
#define OCCP_RESET_RESULT   0xc0de4204
#define OCCP_SUCCESS_RESULT 0xc0de4201
#define OCCP_FATAL_RESULT   0xc0de4205
#define OCCP_STATUS_ALL_ERRORS 0x1ff
    typedef uint8_t *OccpConfigArea;
    typedef struct {
      OccpWorkerRegisters control;
      uint8_t pad[OCCP_WORKER_CONTROL_SIZE - sizeof(OccpWorkerRegisters)];
    } OccpWorker;

    typedef struct {
      OccpAdminRegisters admin;
      uint8_t pad[OCCP_ADMIN_SIZE - sizeof(OccpAdminRegisters)];
      OccpWorker worker[OCCP_MAX_WORKERS];
      uint8_t config[OCCP_MAX_WORKERS][OCCP_WORKER_CONFIG_SIZE];
    } OccpSpace;
#ifdef __cplusplus
  }
}
#endif
#endif
