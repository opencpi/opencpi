// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

#ifndef OCCP_H
#define OCCP_H
#include <stdint.h>

namespace CPI {
  namespace RPL {
    struct OccpAdminRegisters {
      const uint32_t
        magic1,
        magic2,
        revision,
        birthday,
        config,
        pciDevice,
        attention,
        mbz1;
      uint32_t
        scratch20,
        scratch24;
      const uint32_t
        counter,
        status;
      uint32_t
        msiHigh,
        msiLow,
        msiData;
      const uint32_t
        mbz2;
    };
    struct OccpWorkerRegisters {
      const uint32_t
        initialize,
        start,
        stop,
        release,
        test,
        beforeQuery,
        afterConfigure,
        reserved7;
      uint32_t
        status,
        control;
      const uint32_t
        lastConfig,
        reserved[5];
    };
#define OCCP_CONTROL_ENABLE 0x80000000
#define OCCP_WORKER_SIZE 0x10000
#define OCCP_ADMIN_SIZE 0x10000
#define OCCP_WORKER_CONTROL_BASE(i) ((i) * OCCP_BYTES_PER_WORKER)
#define OCCP_WORKER_PROPERTY_BASE(i) ((i) * OCCP_BYTES_PER_WORKER)
#define OCCP_MAX_WORKERS 15
#define OCFRP0_VENDOR 0x10ee
#define OCFRP0_DEVICE 0x4243
#define OCFRP0_CLASS 0x05
#define OCFRP0_SUBCLASS 0x00
#define OCCP_MAGIC1 'Open'
#define OCCP_MAGIC2 'CPI\0'
#define OCCP_ERROR_RESULT   0xc0de4202
#define OCCP_TIMEOUT_RESULT 0xc0de4203
#define OCCP_RESET_RESULT   0xc0de4204
#define OCCP_SUCCESS_RESULT 0xc0de4201
#define OCCP_FATAL_RESULT   0xc0de4205
#define OCCP_STATUS_ALL_ERRORS 0x1ff
    typedef uint8_t *OccpConfigArea;
    typedef struct {
      uint8_t config[OCCP_WORKER_SIZE - sizeof(OccpWorkerRegisters)];
      OccpWorkerRegisters control;
    } OccpWorker;

    typedef struct {
      OccpAdminRegisters admin;
      uint8_t pad[OCCP_ADMIN_SIZE - sizeof(OccpAdminRegisters)];
      OccpWorker worker[OCCP_MAX_WORKERS];
    } OccpSpace;

  }
}
#endif
