/*
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

// This file contains basic definitions of opencpi's usage of ethernet that are 
// used by C code and driver code.  THe compile-time definitions are in the
// global namespace.
/*
  Generic Ethernet L2 "discovery" etc.
  We define L2 Ethernet discovery as sending a broadcast packet with a defined ethertype and payload,
  and receiving one or more responses back of the same ethertype, and with a payload.
  This the "found" callback is the "name" (including mac address) and response payload.
  Thus this module knows about ethernet, but not about the details of discovery payloads.
*/

#ifndef NETDEFS_H
#define NETDEFS_H
#ifdef __cplusplus
namespace OCPI {
  namespace HDL {
    namespace Net {
      extern unsigned typeLength[];
#define OCPI_HDL_ETHER OCPI::HDL::Net::
#else
#define OCPI_HDL_ETHER
#endif
// These structures overlap the ethertype since that makes the likelihood of proper/32-bit alignment
// near 100%.
// FIXME: add the option to the packet socket to back up the payload.
typedef struct {
  uint16_t etherTypeOverlay;  // this is to align the protocol in ethernet frames.
  uint16_t length; // big endian
  uint16_t pad;
  uint8_t  typeEtc;
  uint8_t  tag;
} EtherControlHeader;
typedef struct {
  EtherControlHeader header;
  uint8_t mbx80;
  uint8_t mbz0;
  uint8_t mbz1;
  uint8_t maxCoalesced;
} EtherControlNop;
typedef struct {
  EtherControlHeader header;
  uint32_t address, data;
} EtherControlWrite;
typedef struct {
  EtherControlHeader header;
  uint32_t address;
} EtherControlRead;
typedef struct {
  EtherControlHeader header;
  uint8_t mbx40;
  uint8_t mbz0;
  uint8_t mbz1;
  uint8_t maxCoalesced;
} EtherControlNopResponse;
typedef struct {
  EtherControlHeader header;
  uint32_t data; // read and nop
} EtherControlReadResponse;
typedef struct {
  EtherControlHeader header;
} EtherControlWriteResponse;
typedef union {
  EtherControlHeader header;
  EtherControlNop nop;
  EtherControlWrite write;
  EtherControlRead read;
  EtherControlNopResponse nopResponse;
  EtherControlWriteResponse writeResponse;
  EtherControlReadResponse readResponse;
} EtherControlPacket;
typedef enum {
  OCCP_NOP,
  OCCP_WRITE,
  OCCP_READ,
  OCCP_RESPONSE,
  OCCP_TYPE_LIMIT
} EtherControlMessageType;
typedef enum {
  OK,
  WORKER_TIMEOUT,
  ERROR,
  ETHER_TIMEOUT,
  RESPONSE_LIMIT
} EtherControlResponse;
#define OCCP_ETHER_UNCACHED(t_and_be) ((t_and_be) & 0x40)
#define OCCP_ETHER_MESSAGE_TYPE(t_and_be) ((OCPI_HDL_ETHER EtherControlMessageType)(((t_and_be) >> 4) & 3))
#define OCCP_ETHER_BYTE_ENABLES(t_and_be) ((t_and_be) & 0xf)
#define OCCP_ETHER_RESPONSE(t_and_be) ((OCPI_HDL_ETHER EtherControlResponse)((t_and_be) & 0xf))
#define OCCP_ETHER_TYPE_ETC(type, be, uncache, rsvd) \
  (uint8_t)(((type) << 4) | ((be) & 0xf) | ((uncache) << 6) | ((rsvd) << 7))
#define OCCP_ETHER_RESERVED(t_and_be) ((t_and_be) & 0x80)

#ifdef __cplusplus
    }
  }
}
#endif
#endif
