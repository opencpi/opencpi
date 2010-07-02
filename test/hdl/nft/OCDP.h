#ifndef OCDP_H
#define OCDP_H
namespace CPI {
  namespace RPL {
struct OcdpProperties {
  uint32_t
  nLocalBuffers,	// 0x00
    nRemoteBuffers,	// 0x04
    localBufferBase,	// 0x08
    localMetadataBase,	// 0x0c
    localBufferSize,	// 0x10
    localMetadataSize,	// 0x14
    nRemoteDone,        // 0x18 written indicating remote action on local buffers
    rsvd,		// 0x1c
    nReady;             // 0x20 read by remote to know local buffers for remote action
  const uint32_t
    foodFace,	        // 0x24 constant 0xf00dface
    debug[9];		// 0x28/2c/30/34/38/3c/40/44/48
  uint32_t
    memoryBytes,        // 0x4c
    remoteBufferBase,	// 0x50
    remoteMetadataBase, // 0x54
    remoteBufferSize,   // 0x58
    remoteMetadataSize, // 0x5c
    remoteFlagBase,     // 0x60
    remoteFlagPitch,    // 0x64
    control,            // 0x68
    flowDiagCount;      // 0x6c
};
#define OCDP_CONTROL_DISABLED 0
#define OCDP_CONTROL_PRODUCER 1
#define OCDP_CONTROL_CONSUMER 2
#define OCDP_CONTROL(dir, role) (((dir) << 2) | (role))
#define OCDP_LOCAL_BUFFER_ALIGN 16     // The constraint on the alignment of local buffers
#define OCDP_FAR_BUFFER_ALIGN 4        // The constraint on the alignment of far buffers

struct OcdpMetadata {
  uint32_t
    length,
    opCode,
    tag,
    interval;
};
#define OCDP_METADATA_SIZE sizeof(OcdpMetadata)
enum OcdpRole {
  OCDP_PASSIVE = 0,
  OCDP_ACTIVE_MESSAGE = 1,
  OCDP_ACTIVE_FLOWCONTROL = 2
};

  }  }
#endif
