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

#ifndef HDLOCDP_H
#define HDLOCDP_H
#ifdef __cplusplus
namespace OCPI {
  namespace HDL {
#endif
typedef struct {
  uint32_t
    nLocalBuffers,      // 0x00
    nRemoteBuffers,     // 0x04
    localBufferBase,    // 0x08
    localMetadataBase,  // 0x0c
    localBufferSize,    // 0x10
    localMetadataSize,  // 0x14
    nRemoteDone,        // 0x18 written indicating remote action on local buffers
    rsvdregion,         // 0x1c
    nReady;             // 0x20 read by remote to know local buffers for remote action
  const uint32_t
    foodFace,           // 0x24 constant 0xf00dface: debug
    debug[9];           // 0x28/2c/30/34/38/3c/40/44/48
  uint32_t
    memoryBytes,        // 0x4c
    remoteBufferBase,   // 0x50
    remoteMetadataBase, // 0x54
    remoteBufferSize,   // 0x58
    remoteMetadataSize, // 0x5c
    remoteFlagBase,     // 0x60
    remoteFlagPitch,    // 0x64
    control,            // 0x68
    flowDiagCount,      // 0x6c
    debug1[9],          // 0x70/74/78/7c/80/84/88/8c/90
    remoteBufferHi,     // 0x94
    remoteMetadataHi,   // 0x98
    remoteFlagHi;       // 0x9c
  uint64_t
    startTime,          // 0xa0
    doneTime;           // 0xa8
} OcdpProperties;
#define OCDP_CONTROL_DISABLED 0
#define OCDP_CONTROL_PRODUCER 1
#define OCDP_CONTROL_CONSUMER 2
#define OCDP_CONTROL(dir, role) ((uint32_t)(((dir) << 2) | (role)))
#define OCDP_LOCAL_BUFFER_ALIGN 16     // The constraint on the alignment of local buffers
#define OCDP_FAR_BUFFER_ALIGN 4        // The constraint on the alignment of far buffers

typedef struct {
  uint32_t
    length,
    opCode,
    tag,
    interval;
} OcdpMetadata;
#define OCDP_METADATA_SIZE ((uint32_t)sizeof(OcdpMetadata))
enum OcdpRole {
  OCDP_PASSIVE = 0,
  OCDP_ACTIVE_MESSAGE = 1,
  OCDP_ACTIVE_FLOWCONTROL = 2
};

#ifdef __cplusplus
  }
}
#endif
#endif
