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

#ifndef INCLUDED_OCPI_XM_PORT_TABLE_H
#define INCLUDED_OCPI_XM_PORT_TABLE_H

#ifdef __cplusplus
  extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#define XM_PORT_TABLE_INDEX ( 1 )

typedef enum
{
  XM_PORT_TARGET, // Input
  XM_PORT_SOURCE // Output

} XmPortDirection;

#define RCC_XM_PORT_ATTR_N_BYTES ( 32 )

typedef struct XmPort
{
  XmPortDirection direction;
  char filespec [ RCC_XM_PORT_ATTR_N_BYTES ];
  char file_name [ RCC_XM_PORT_ATTR_N_BYTES ];
  char type_allow [ RCC_XM_PORT_ATTR_N_BYTES ];
  char format_allow [ RCC_XM_PORT_ATTR_N_BYTES ];
  int8_t buf_type;
  char format [ RCC_XM_PORT_ATTR_N_BYTES ];
  char scr_format [ RCC_XM_PORT_ATTR_N_BYTES ];
  int xfer_len;
  double size;
  char Switch [ RCC_XM_PORT_ATTR_N_BYTES ];
  uint32_t open;
  int32_t type;
  double xstart;
  double xdelta;
  int xunits;
  double ystart;
  double ydelta;
  int yunits;
  int bpa;
  int bps;
  int spa;
  int bpe;
  int ape;
  double dbpe;
  int class_;
  int pipe;
  double subsize;
  int cons_len;
  int offset;
  size_t port_idx;
} XmPort;

typedef struct XmPortTable
{
  size_t n_ports;

  XmPort ports [ 0 ];

} XmPortTable;

struct RCCWorker;

/* Used by inok and allocate */
int ocpi_xm_get_port_by_name ( RCCWorker* wctx,
                               const char* name,
                               XmPort* port );

typedef struct Sheader Sheader;

void ocpi_xm_convert_port_to_sheader ( const XmPort* port, Sheader* hcb );

typedef struct HEADER HEADER;

void ocpi_xm_convert_port_to_cpheader ( const XmPort* port, HEADER* hcb );

#ifdef __cplusplus
  }
#endif

#endif /* End: #ifndef INCLUDED_OCPI_XM_PORT_TABLE_H */
