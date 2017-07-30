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

#ifndef INCLUDED_OCPI_XM_INTERCEPT_CPP_H
#define INCLUDED_OCPI_XM_INTERCEPT_CPP_H

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- C/C++ language includes ------------------------------------------ */

/* ---- OpenOCPI for X-Midas types ---------------------------------------- */

#include "cdefs.h"

/* ---- OpenOCPI for X-Midas file header replacement ---------------------- */

typedef struct HEADER
{
  HEADER ( )
    : filespec ( "" ),
      file_name ( "" ),
      type_allow ( "" ),
      format_allow ( "" ),
      format ( "" ),
      scr_format ( "" ),
      Switch ( "" ),
      open ( 0 ),
      buf_type ( 0 ),
      type ( 0 ),
      xstart ( 0 ),
      xdelta ( 0 ),
      ystart ( 0 ),
      ydelta ( 0 ),
      xunits ( 0 ),
      yunits ( 0 ),
      size ( 0 ),
      data_size ( 0 ),
      xfer_len ( 0 ),
      bpa ( 0 ),
      bps ( 0 ),
      spa ( 0 ),
      bpe ( 0 ),
      ape ( 0 ),
      dbpe ( 0.0 ),
      class_ ( 0 ),
      pipe ( 0 ),
      subsize ( 0 ),
      cons_len ( 0 ),
      offset ( 0 )
  {
    // Empty
  }

  ~HEADER ( )
  {
    // Empty
  }

  // Variables are public to accomodate direct access by XM primitives
  std::string filespec;
  std::string file_name;
  std::string type_allow;
  std::string format_allow;
  std::string format;
  std::string scr_format;
  std::string Switch;
  uint32_t open;
  int8_t buf_type;
  int32_t type;
  double xstart;
  double xdelta;
  double ystart;
  double ydelta;
  int xunits;
  int yunits;
  double size;
  double data_size;
  int xfer_len;
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

  // OpenOCPI Information
  std::size_t port_idx;

} HEADER;

typedef HEADER CPHEADER;

/* ---- OpenOCPI for X-Midas defines -------------------------------------- */

enum
{
  HCBF_INPUT = ( 1 << 8 ),
  HCBF_OUTPUT =  ( 1 << 9 ),
  HCBF_OPTIONAL = ( 1 << 11 )
};

/* ---- OpenOCPI for X-Midas global variables (from X-Midas) -------------- */

extern const char* standard_windows;

/* ---- OpenOCPI for X-Midas forward declarations ------------------------- */

struct RCCWorker;

/* ---- Macro to rename emulated X-Midas routines ------------------------ */

#define OCPI_CONCAT_NEST( _lhs, _rhs ) _lhs##_rhs

#define OCPI_XM_PREFIX( _routine ) OCPI_CONCAT_NEST( ocpi_xm_, _routine )

/* ----------------------------------------------------------------------- */

#ifdef OCPI_XM_MAINROUTINE

#define mainroutine() OCPI_XM_MAINROUTINE ( RCCWorker* p_wctx )

extern void OCPI_XM_MAINROUTINE ( RCCWorker* );

#endif

/* ----------------------------------------------------------------------- */

#define m_align(...) OCPI_XM_PREFIX( m_align ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m_align ( RCCWorker*, HEADER&, HEADER&, int_u4, real_8&, real_8& );

/* ----------------------------------------------------------------------- */

#define m_allocate(...) OCPI_XM_PREFIX( m_allocate ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m_allocate ( RCCWorker*, HEADER& );

/* ------------------------------------------------------------------------ */

#define m_apick(...) OCPI_XM_PREFIX( m_apick ) ( p_wctx, __VA_ARGS__ )

extern std::string ocpi_xm_m_apick ( RCCWorker*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_bpa(...) OCPI_XM_PREFIX( m_bpa ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m_bpa ( RCCWorker*, const std::string& );

/* ----------------------------------------------------------------------- */

#define m_bps(...) OCPI_XM_PREFIX( m_bps ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m_bps ( RCCWorker*, const std::string& );

/* ----------------------------------------------------------------------- */

#define m_spa(...) OCPI_XM_PREFIX( m_spa ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m_spa ( RCCWorker*, const std::string& );

/* ----------------------------------------------------------------------- */

#define m_bpe(...) OCPI_XM_PREFIX( m_bpe ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m_bpe ( RCCWorker*, const HEADER& );

/* ----------------------------------------------------------------------- */

#define m_bytes_reqd(...) OCPI_XM_PREFIX( m_bytes_reqd ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m_bytes_reqd ( RCCWorker*, int_4, int_4 );

/* ----------------------------------------------------------------------- */

#define m_force1000(...) OCPI_XM_PREFIX( m_force1000 ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m_force1000 ( RCCWorker*, HEADER& );

/* ----------------------------------------------------------------------- */

#define m_initialize(...) OCPI_XM_PREFIX( m_initialize ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m_initialize ( RCCWorker*, HEADER& );

/* ----------------------------------------------------------------------- */

#define m_inok(...) OCPI_XM_PREFIX( m_inok ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m_inok ( RCCWorker*, HEADER& );

/* ----------------------------------------------------------------------- */

#define m_promote_format(...) OCPI_XM_PREFIX( m_promote_format ) ( p_wctx, __VA_ARGS__ )

extern std::string ocpi_xm_m_promote_format ( RCCWorker*, const std::string&, const std::string&, const std::string& );

/* ----------------------------------------------------------------------- */

#define m_propagate(...) OCPI_XM_PREFIX( m_propagate ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m_propagate ( RCCWorker*, const HEADER&, HEADER& );

/* ----------------------------------------------------------------------- */

#define m_spick(...) OCPI_XM_PREFIX( m_spick ) ( p_wctx, __VA_ARGS__ )

extern bool_4 ocpi_xm_m_spick ( RCCWorker*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_xfer_length(...) OCPI_XM_PREFIX( m_xfer_length ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m_xfer_length ( RCCWorker*, int_4, int_4 );

/* ----------------------------------------------------------------------- */

#define m_warning(...) OCPI_XM_PREFIX( m_warning ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m_warning ( RCCWorker*, const std::string& );

/* ----------------------------------------------------------------------- */

#define m_sync(...) OCPI_XM_PREFIX( m_sync ) ( p_wctx )

extern void ocpi_xm_m_sync ( RCCWorker* );

/* ----------------------------------------------------------------------- */

#define m_do(...) OCPI_XM_PREFIX( m_do ) ( p_wctx, __VA_ARGS__ )

extern bool_4 ocpi_xm_m_do ( RCCWorker*, int_4, int_4& );

/* ----------------------------------------------------------------------- */

#define m_error(...) OCPI_XM_PREFIX( m_error ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m_error ( RCCWorker*, const std::string& );

/* ----------------------------------------------------------------------- */

#define m_grabx(...) OCPI_XM_PREFIX( m_grabx ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m_grabx ( RCCWorker*, HEADER&, void*, int_4& );

/* ----------------------------------------------------------------------- */

#define m_grab(...) OCPI_XM_PREFIX( m_grab ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m_grab ( RCCWorker*, HEADER&, void*, int_4, int_4, int_4& );

/* ----------------------------------------------------------------------- */

#define m_filad(...) OCPI_XM_PREFIX( m_filad ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m_filad ( RCCWorker*, HEADER&, void*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_close(...) OCPI_XM_PREFIX( m_close ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m_close ( RCCWorker*, HEADER& );

/* ----------------------------------------------------------------------- */

#define m_checkin(...) OCPI_XM_PREFIX( m_checkin ) ( p_wctx )

extern void ocpi_xm_m_checkin ( RCCWorker* );

/* ----------------------------------------------------------------------- */

#define m_get_switch(...) OCPI_XM_PREFIX( m_get_switch ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m_get_switch ( RCCWorker*, const std::string& );

/* ----------------------------------------------------------------------- */

#define m_return(...) OCPI_XM_PREFIX( m_return ) ( p_wctx )

extern void ocpi_xm_m_return ( RCCWorker* );

/* ----------------------------------------------------------------------- */

#define m_test_format(...) OCPI_XM_PREFIX( m_test_format ) ( p_wctx, __VA_ARGS__ )

std::string ocpi_xm_m_test_format ( RCCWorker* p_wctx, const std::string&, const std::string& );

/* ----------------------------------------------------------------------- */

#define m_dpick(...) OCPI_XM_PREFIX( m_dpick ) ( p_wctx, __VA_ARGS__ )

extern real_8 ocpi_xm_m_dpick ( RCCWorker*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_pause(...) OCPI_XM_PREFIX( m_pause ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m_pause ( RCCWorker*, real_4 );

/* ----------------------------------------------------------------------- */

#define m_mwinit(...) OCPI_XM_PREFIX( m_mwinit ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m_mwinit ( RCCWorker*, const std::string&, int_4, const std::string& );

/* ----------------------------------------------------------------------- */

#define m_dwinit(...) OCPI_XM_PREFIX( m_dwinit ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m_dwinit ( RCCWorker*, const std::string&, real_8, real_8, real_8, real_8 );

/* ----------------------------------------------------------------------- */

#define m_todo(...) OCPI_XM_PREFIX( m_todo ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m_todo ( RCCWorker*, real_8 );

/* ----------------------------------------------------------------------- */

#define m_mwget(...) OCPI_XM_PREFIX( m_mwget ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m_mwget ( RCCWorker*, int_4, int_4& );

/* ----------------------------------------------------------------------- */

#define m_dwget(...) OCPI_XM_PREFIX( m_dwget ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m_dwget ( RCCWorker*, int_4, real_8& );

/* ----------------------------------------------------------------------- */

#define m_length(...) OCPI_XM_PREFIX( m_length ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m_length ( RCCWorker*, const std::string& );

/* ----------------------------------------------------------------------- */

#define m_get_switch_def(...) OCPI_XM_PREFIX( m_get_switch_def ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m_get_switch_def ( RCCWorker*, const std::string&, int_4 );

/* ----------------------------------------------------------------------- */

#define m_npm(...) OCPI_XM_PREFIX( m_npm ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m_npm ( RCCWorker*, char );

/* ----------------------------------------------------------------------- */

#define m_npt(...) OCPI_XM_PREFIX( m_npt ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m_npt ( RCCWorker*, char );

/* ----------------------------------------------------------------------- */

#define m_init(...) OCPI_XM_PREFIX( m_init ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m_init ( RCCWorker*, HEADER&, const std::string&, const std::string&, const std::string&, int_4* );

/* ----------------------------------------------------------------------- */

#define m_open(...) OCPI_XM_PREFIX( m_open ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m_open ( RCCWorker*, HEADER&, int_4 );

/* ----------------------------------------------------------------------- */

#define m_lpick(...) OCPI_XM_PREFIX( m_lpick ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m_lpick ( RCCWorker*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_fpick(...) OCPI_XM_PREFIX( m_fpick ) ( p_wctx, __VA_ARGS__ )

extern float ocpi_xm_m_fpick ( RCCWorker*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_bpick(...) OCPI_XM_PREFIX( m_bpick ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m_bpick ( RCCWorker*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_ipick(...) OCPI_XM_PREFIX( m_ipick ) ( p_wctx, __VA_ARGS__ )

extern int_2 ocpi_xm_m_ipick ( RCCWorker*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_upick(...) OCPI_XM_PREFIX( m_upick ) ( p_wctx, __VA_ARGS__ )

extern const char* ocpi_xm_m_upick ( RCCWorker*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_window_by_id(...) OCPI_XM_PREFIX( m_window_by_id ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_window_by_id ( RCCWorker*, real_4*, int_4, int_4 );

/* ----------------------------------------------------------------------- */

#define m_malloc(...) OCPI_XM_PREFIX( m_malloc ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m_malloc ( RCCWorker*, int_4, void** );

/* ----------------------------------------------------------------------- */

#define m_window_id(...) OCPI_XM_PREFIX( m_window_id ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m_window_id ( RCCWorker*, const std::string& );

/* ----------------------------------------------------------------------- */

#define m_lwinit(...) OCPI_XM_PREFIX( m_lwinit ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m_lwinit ( RCCWorker*, const std::string&, int_4, int_4, int_4, int_4 );

/* ----------------------------------------------------------------------- */

#define m_lwget(...) OCPI_XM_PREFIX( m_lwget ) ( p_wctx, __VA_ARGS__ )

int_4 ocpi_xm_m_lwget ( RCCWorker*, const int_4, int_4& );

/* ----------------------------------------------------------------------- */

#define m_mfree(...) OCPI_XM_PREFIX( m_mfree ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_mfree ( RCCWorker*, int_4, void** );

/* ----------------------------------------------------------------------- */

#define m_free(...) OCPI_XM_PREFIX( m_free ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_free ( RCCWorker*, void** );

/* ----------------------------------------------------------------------- */

#define m_cvsum(...) OCPI_XM_PREFIX( m_cvsum ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_cvsum ( RCCWorker*, const void*, void*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_cvmul(...) OCPI_XM_PREFIX( m_cvmul ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_cvmul ( RCCWorker*, const void*, const void*, void*, int_4, int_4 );

/* ----------------------------------------------------------------------- */

#define m_crvmul(...) OCPI_XM_PREFIX( m_crvmul ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_crvmul ( RCCWorker*, const void*, const void*, void*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_cvblk(...) OCPI_XM_PREFIX( m_cvblk ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_cvblk ( RCCWorker*, const void*, void*, int_4, int_4 );

/* ----------------------------------------------------------------------- */

#define m_cvsadd(...) OCPI_XM_PREFIX( m_cvsadd ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_cvsadd ( RCCWorker*, const void*, const void*, void*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_cvadd(...) OCPI_XM_PREFIX( m_cvadd ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_cvadd ( RCCWorker*, const void*, const void*, void*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_vsum(...) OCPI_XM_PREFIX( m_vsum ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_vsum ( RCCWorker*, const void*, void*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_vmul(...) OCPI_XM_PREFIX( m_vmul ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_vmul ( RCCWorker*, const void*, const void*, void*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_vsmul(...) OCPI_XM_PREFIX( m_vsmul ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_vsmul ( RCCWorker*, const void*, const void*, void*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_vadd(...) OCPI_XM_PREFIX( m_vadd ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_vadd ( RCCWorker*, const void*, const void*, void*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_vsadd(...) OCPI_XM_PREFIX( m_vsadd ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_vsadd ( RCCWorker*, const void*, const void*, void*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_vmov(...) OCPI_XM_PREFIX( m_vmov ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_vmov ( RCCWorker*, const void*, int_4, void*, int_4, int_4 );

/* ----------------------------------------------------------------------- */

#define m_vfill(...) OCPI_XM_PREFIX( m_vfill ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_vfill ( RCCWorker*, void*, const void*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_cvmag2(...) OCPI_XM_PREFIX( m_cvmag2 ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_cvmag2 ( RCCWorker*, const void*, void*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_power2ge(...) OCPI_XM_PREFIX( m_power2ge ) ( p_wctx, __VA_ARGS__ )

int_4 ocpi_xm_m_power2ge ( RCCWorker*, int_4 );

/* ----------------------------------------------------------------------- */

#define m_genradix(...) OCPI_XM_PREFIX( m_genradix ) ( p_wctx, __VA_ARGS__ )

int_4 ocpi_xm_m_genradix ( RCCWorker*, int_4, int_4*, int_4*, int_4*, int_4* );

/* ----------------------------------------------------------------------- */

#define m_window(...) OCPI_XM_PREFIX( m_window ) ( p_wctx, __VA_ARGS__ )

int_4 ocpi_xm_m_window ( RCCWorker*, real_4*, const std::string&, int_4 );

/* ----------------------------------------------------------------------- */

#define m_cfft(...) OCPI_XM_PREFIX( m_cfft ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_cfft ( RCCWorker*, real_4*, int_4, int_4 );

/* ----------------------------------------------------------------------- */

#define m_rfft(...) OCPI_XM_PREFIX( m_rfft ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_rfft ( RCCWorker*, real_4*, int_4, int_4 );

/* ----------------------------------------------------------------------- */

#define m_arslt(...) OCPI_XM_PREFIX( m_arslt ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_arslt ( RCCWorker*, const std::string&, const std::string& );

/* ----------------------------------------------------------------------- */

#define m_brslt(...) OCPI_XM_PREFIX( m_brslt ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_brslt ( RCCWorker*, const std::string&, int_4 );

/* ----------------------------------------------------------------------- */

#define m_drslt(...) OCPI_XM_PREFIX( m_drslt ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_drslt ( RCCWorker*, const std::string&, double );

/* ----------------------------------------------------------------------- */

#define m_frslt(...) OCPI_XM_PREFIX( m_frslt ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_frslt ( RCCWorker*, const std::string&, float );

/* ----------------------------------------------------------------------- */

#define m_irslt(...) OCPI_XM_PREFIX( m_irslt ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_irslt ( RCCWorker*, const std::string&, int_2 );

/* ----------------------------------------------------------------------- */

#define m_lrslt(...) OCPI_XM_PREFIX( m_lrslt ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m_lrslt ( RCCWorker*, const std::string&, int_4 );

/* ---- Non intercepted non-XM routines ---------------------------------- */

void sscal ( int_4 count, float scalar, void* vector, int_4 stride );

#ifdef __cplusplus
}
#endif

#endif // End: #ifndef INCLUDED_OCPI_XM_INTERCEPT_CPP_H
