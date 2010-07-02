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
	attention;
      uint32_t
        ppsCount,
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
        mbz;
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
	reserved7,
	status;
      uint32_t
	control;
      const uint32_t
        lastConfig;
      uint32_t
        clearError,
	reserved[4];
    };
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
// Magic values from config space
#define OCCP_MAGIC1 "Open"
#define OCCP_MAGIC2 "CPI"
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
  }
}
#endif
