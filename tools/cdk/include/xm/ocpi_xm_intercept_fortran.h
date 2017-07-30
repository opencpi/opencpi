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

#ifndef INCLUDED_OCPI_XM_INTERCEPT_FORTRAN_H
#define INCLUDED_OCPI_XM_INTERCEPT_FORTRAN_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---- OpenOCPI for X-Midas types ---------------------------------------- */

#include "cdefs.h"

#include <stdlib.h>
#include <stdint.h>


/* ---- OpenOCPI for X-Midas constants ------------------------------------ */

#define twopi 6.283185308

/* ---- OpenOCPI for X-Midas header replacement --------------------------- */

#define XM_SHEADER_NAME_LEN 40

#define XM_SHEADER_FILE_NAME_LEN 80

typedef struct Sheader
{
  char filespec [ XM_SHEADER_NAME_LEN ];
  char file_name [ XM_SHEADER_FILE_NAME_LEN ];
  char type_allow [ XM_SHEADER_NAME_LEN ];
  char format_allow [ XM_SHEADER_NAME_LEN ];
  char format [ 2 ];
  char scr_format [ 2 ];
  char Switch [ 8 ];
  uint32_t open;
  int8_t buf_type [ 1 ];
  int32_t type;
  double xstart;
  double xdelta;
  double ystart;
  double ydelta;
  int xunits;
  int yunits;
  double size;
  double data_size;
  int32_t xfer_len;
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

} Sheader;

extern const char* standard_windows;

/* ---- OpenOCPI for X-Midas common replacement --------------------------- */

typedef struct
{
  int Break;

} Smcommon;

typedef struct
{
  Smcommon *mc;

} Cm$common;

extern Cm$common* Tm$common;

/* ---- OpenOCPI for X-Midas forward declarations ------------------------- */

typedef struct RCCWorker RCCWorker;

/* ---- Macro to rename emulated X-Midas routines ------------------------ */

#define OCPI_CONCAT_NEST( _lhs, _rhs ) _lhs##_rhs

#define OCPI_XM_PREFIX( _routine ) OCPI_CONCAT_NEST( ocpi_xm_, _routine )

/* ----------------------------------------------------------------------- */

#ifdef OCPI_XM_MAINROUTINE

#define mainroutine() OCPI_XM_MAINROUTINE ( RCCWorker* p_wctx )

extern void OCPI_XM_MAINROUTINE ( RCCWorker* );

#endif

/* ----------------------------------------------------------------------- */

#define m$align_(...) OCPI_XM_PREFIX( m$align ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m$align ( RCCWorker*, Sheader*, Sheader*, int_u4*, real_8*, real_8* );

/* ----------------------------------------------------------------------- */

#define m$allocate_(...) OCPI_XM_PREFIX( m$allocate ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m$allocate ( RCCWorker*, Sheader* );

/* ------------------------------------------------------------------------ */

#define m$apick_(...) OCPI_XM_PREFIX( m$apick ) ( p_wctx, __VA_ARGS__ )

extern const char* ocpi_xm_m$apick ( RCCWorker*, int_4* );

/* ----------------------------------------------------------------------- */

#define m$bpa_(...) OCPI_XM_PREFIX( m$bpa ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m$bpa ( RCCWorker*, const char*, int_4 );

/* ----------------------------------------------------------------------- */

#define m$bps_(...) OCPI_XM_PREFIX( m$bps ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m$bps ( RCCWorker*, const char* );

/* ----------------------------------------------------------------------- */

#define m$spa_(...) OCPI_XM_PREFIX( m$spa ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m$spa ( RCCWorker*, const char* );

/* ----------------------------------------------------------------------- */

#define m$bpe_(...) OCPI_XM_PREFIX( m$bpe ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m$bpe ( RCCWorker*, const Sheader* );

/* ----------------------------------------------------------------------- */

#define m$bytes_reqd_(...) OCPI_XM_PREFIX( m$bytes_reqd ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m$bytes_reqd ( RCCWorker*, int_4, int_4 );

/* ----------------------------------------------------------------------- */

#define m$force1000_(...) OCPI_XM_PREFIX( m$force1000 ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m$force1000 ( RCCWorker*, Sheader* );

/* ----------------------------------------------------------------------- */

#define m$initialize_(...) OCPI_XM_PREFIX( m$initialize ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m$initialize ( RCCWorker*, Sheader* );

/* ----------------------------------------------------------------------- */

#define m$inok_(...) OCPI_XM_PREFIX( m$inok ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m$inok ( RCCWorker*, Sheader* );

/* ----------------------------------------------------------------------- */

#define m$promote_format_(...) OCPI_XM_PREFIX( m$promote_format ) ( p_wctx, __VA_ARGS__ )

extern const char* ocpi_xm_m$promote_format ( RCCWorker*, const char*, const char*, const char* );

/* ----------------------------------------------------------------------- */

#define m$propagate_(...) OCPI_XM_PREFIX( m$propagate ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m$propagate ( RCCWorker*, const Sheader*, Sheader* );

/* ----------------------------------------------------------------------- */

#define m$spick_(...) OCPI_XM_PREFIX( m$spick ) ( p_wctx, __VA_ARGS__ )

extern bool_4 ocpi_xm_m$spick ( RCCWorker*, int_4 );

/* ----------------------------------------------------------------------- */

#define m$xfer_length_(...) OCPI_XM_PREFIX( m$xfer_length ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m$xfer_length ( RCCWorker*, int_4*, int_4* );

/* ----------------------------------------------------------------------- */

#define m$warning_(...) OCPI_XM_PREFIX( m$warning ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m$warning ( RCCWorker*, const char*, int_4 );

/* ----------------------------------------------------------------------- */

#define m$sync_(...) OCPI_XM_PREFIX( m$sync ) ( p_wctx )

extern void ocpi_xm_m$sync ( RCCWorker* );

/* ----------------------------------------------------------------------- */

#define m$do_(...) OCPI_XM_PREFIX( m$do ) ( p_wctx, __VA_ARGS__ )

extern bool_4 ocpi_xm_m$do ( RCCWorker*, int_4*, int_4* );

/* ----------------------------------------------------------------------- */

#define m$error_(...) OCPI_XM_PREFIX( m$error ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m$error ( RCCWorker*, const char*, int_4 );

/* ----------------------------------------------------------------------- */

#define m$grabx_(...) OCPI_XM_PREFIX( m$grabx ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m$grabx ( RCCWorker*, Sheader*, void*, int_4* );

/* ----------------------------------------------------------------------- */

#define m$grab_(...) OCPI_XM_PREFIX( m$grab ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m$grab ( RCCWorker*, Sheader*, void*, int_4*, int_4*, int_4* );

/* ----------------------------------------------------------------------- */

#define m$filad_(...) OCPI_XM_PREFIX( m$filad ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m$filad ( RCCWorker*, Sheader*, void*, int_4* );

/* ----------------------------------------------------------------------- */

#define m$close_(...) OCPI_XM_PREFIX( m$close ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m$close ( RCCWorker*, Sheader* );

/* ----------------------------------------------------------------------- */

#define m$checkin_(...) OCPI_XM_PREFIX( m$checkin ) ( p_wctx )

extern void ocpi_xm_m$checkin ( RCCWorker* );

/* ----------------------------------------------------------------------- */

#define m$get_switch_(...) OCPI_XM_PREFIX( m$get_switch ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m$get_switch ( RCCWorker*, const char*, size_t );

/* ----------------------------------------------------------------------- */

#define m$return_(...) OCPI_XM_PREFIX( m$return ) ( p_wctx )

extern void ocpi_xm_m$return ( RCCWorker* );

/* ----------------------------------------------------------------------- */

#define m$test_format_(...) OCPI_XM_PREFIX( m$test_format ) ( p_wctx, __VA_ARGS__ )

extern const char* ocpi_xm_m$test_format ( RCCWorker* p_wctx, const char*, const char*, int_4, int_4 );

/* ----------------------------------------------------------------------- */

#define m$dpick_(...) OCPI_XM_PREFIX( m$dpick ) ( p_wctx, __VA_ARGS__ )

extern real_8 ocpi_xm_m$dpick ( RCCWorker*, int_4* );

/* ----------------------------------------------------------------------- */

#define m$pause_(...) OCPI_XM_PREFIX( m$pause ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m$pause ( RCCWorker*, const real_4* );

/* ----------------------------------------------------------------------- */

#define m$mwinit_(...) OCPI_XM_PREFIX( m$mwinit ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m$mwinit ( RCCWorker*, const char*, const int_4*, const char*, int_4, int_4 );

/* ----------------------------------------------------------------------- */

#define m$dwinit_(...) OCPI_XM_PREFIX( m$dwinit ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m$dwinit ( RCCWorker*, const char*, const real_8*, const real_8*, const real_8*, const real_8*, int_4 );

/* ----------------------------------------------------------------------- */

#define m$todo_(...) OCPI_XM_PREFIX( m$todo ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m$todo ( RCCWorker*, const real_8* );

/* ----------------------------------------------------------------------- */

#define m$mwget_(...) OCPI_XM_PREFIX( m$mwget ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m$mwget ( RCCWorker*, const int_4*, int_4* );

/* ----------------------------------------------------------------------- */

#define m$dwget_(...) OCPI_XM_PREFIX( m$dwget ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m$dwget ( RCCWorker*, const int_4*, real_8* );

/* ----------------------------------------------------------------------- */

#define m$length_(...) OCPI_XM_PREFIX( m$length ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m$length ( RCCWorker*, const char*, int_4 );

/* ----------------------------------------------------------------------- */

#define m$get_switch_def_(...) OCPI_XM_PREFIX( m$get_switch_def ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m$get_switch_def ( RCCWorker*, const char*, const int_4*, int_4 );

/* ----------------------------------------------------------------------- */

#define m$npt_(...) OCPI_XM_PREFIX( m$npt ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m$npt ( RCCWorker*, const char*, int_4 );

/* ----------------------------------------------------------------------- */

#define m$npm_(...) OCPI_XM_PREFIX( m$npm ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m$npm ( RCCWorker*, const char*, int_4 );

/* ----------------------------------------------------------------------- */

#define m$init_(...) OCPI_XM_PREFIX( m$init ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m$init ( RCCWorker*, Sheader*, const char*, const char*, const char*, int_4*, int_4, int_4, int_4 );

/* ----------------------------------------------------------------------- */

#define m$open_(...) OCPI_XM_PREFIX( m$open ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m$open ( RCCWorker*, Sheader*, int_4* );

/* ----------------------------------------------------------------------- */

#define m$lpick_(...) OCPI_XM_PREFIX( m$lpick ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m$lpick ( RCCWorker*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$fpick_(...) OCPI_XM_PREFIX( m$fpick ) ( p_wctx, __VA_ARGS__ )

extern float ocpi_xm_m$fpick ( RCCWorker*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$bpick_(...) OCPI_XM_PREFIX( m$bpick ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m$bpick ( RCCWorker*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$ipick_(...) OCPI_XM_PREFIX( m$ipick ) ( p_wctx, __VA_ARGS__ )

extern int_2 ocpi_xm_m$ipick ( RCCWorker*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$upick_(...) OCPI_XM_PREFIX( m$upick ) ( p_wctx, __VA_ARGS__ )

extern const char* ocpi_xm_m$upick ( RCCWorker*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$malloc_(...) OCPI_XM_PREFIX( m$malloc ) ( p_wctx, __VA_ARGS__ )

extern void ocpi_xm_m$malloc ( RCCWorker*, int_4*, void** );

/* ----------------------------------------------------------------------- */

#define m$lwinit_(...) OCPI_XM_PREFIX( m$lwinit ) ( p_wctx, __VA_ARGS__ )

extern int_4 ocpi_xm_m$lwinit ( RCCWorker*, const char*, int_4*, int_4*, int_4*, int_4*, int_4 );

/* ----------------------------------------------------------------------- */

#define m$lwget_(...) OCPI_XM_PREFIX( m$lwget ) ( p_wctx, __VA_ARGS__ )

int_4 ocpi_xm_m$lwget ( RCCWorker*, const int_4*, int_4* );

/* ----------------------------------------------------------------------- */

#define m$mfree_(...) OCPI_XM_PREFIX( m$mfree ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$mfree ( RCCWorker*, const int_4*, void** );

/* ----------------------------------------------------------------------- */

#define m$free_(...) OCPI_XM_PREFIX( m$free ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$free ( RCCWorker*, void** );

/* ----------------------------------------------------------------------- */

#define m$window_id_(...) OCPI_XM_PREFIX( m$window_id ) ( p_wctx, __VA_ARGS__ )

int_4 ocpi_xm_m$window_id ( RCCWorker*, const char*, int_4 );

/* ----------------------------------------------------------------------- */

#define m$window_by_id_(...) OCPI_XM_PREFIX( m$window_by_id ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$window_by_id ( RCCWorker*, real_4*, const int_4*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$calc_(...) OCPI_XM_PREFIX( m$calc ) ( p_wctx, __VA_ARGS__ )

int_4 ocpi_xm_m$calc ( RCCWorker*, char*, double*, int*, char*, int*, int, int );

/* ----------------------------------------------------------------------- */

#define m$cvsum_(...) OCPI_XM_PREFIX( m$cvsum ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$cvsum ( RCCWorker*, const void*, void*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$cvmul_(...) OCPI_XM_PREFIX( m$cvmul ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$cvmul ( RCCWorker*, const void*, const void*, void*, const int_4*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$cvblk_(...) OCPI_XM_PREFIX( m$cvblk ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$cvblk ( RCCWorker*, const void*, void*, const int_4*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$crvmul_(...) OCPI_XM_PREFIX( m$crvmul ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$crvmul ( RCCWorker*, const void*, const void*, void*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$cvsadd_(...) OCPI_XM_PREFIX( m$cvsadd ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$cvsadd ( RCCWorker*, const void*, const void*, void*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$cvadd_(...) OCPI_XM_PREFIX( m$cvsadd ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$cvadd ( RCCWorker*, const void*, const void*, void*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$vsum_(...) OCPI_XM_PREFIX( m$vsum ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$vsum ( RCCWorker*, const void*, void*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$vmul_(...) OCPI_XM_PREFIX( m$vmul ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$vmul ( RCCWorker*, const void*, const void*, void*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$vsmul_(...) OCPI_XM_PREFIX( m$vsmul ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$vsmul ( RCCWorker*, const void*, const void*, void*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$vadd_(...) OCPI_XM_PREFIX( m$vadd ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$vadd ( RCCWorker*, const void*, const void*, void*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$vsadd_(...) OCPI_XM_PREFIX( m$vsadd ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$vsadd ( RCCWorker*, const void*, const void*, void*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$vmov_(...) OCPI_XM_PREFIX( m$vmov ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$vmov ( RCCWorker*, const void*, const int_4*, void*, const int_4*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$vfill_(...) OCPI_XM_PREFIX( m$vfill ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$vfill ( RCCWorker*, void*, const void*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$cvmag2_(...) OCPI_XM_PREFIX( m$cvmag2 ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$cvmag2 ( RCCWorker*, const void*, void*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$power2ge_(...) OCPI_XM_PREFIX( m$power2ge ) ( p_wctx, __VA_ARGS__ )

int_4 ocpi_xm_m$power2ge ( RCCWorker*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$genradix_(...) OCPI_XM_PREFIX( m$genradix ) ( p_wctx, __VA_ARGS__ )

int_4 ocpi_xm_m$genradix ( RCCWorker*, const int_4*, int_4*, int_4*, int_4*, int_4* );

/* ----------------------------------------------------------------------- */

#define m$window_(...) OCPI_XM_PREFIX( m$window ) ( p_wctx, __VA_ARGS__ )

int_4 ocpi_xm_m$window ( RCCWorker*, real_4*, const char*, const int_4*, int_4 );

/* ----------------------------------------------------------------------- */

#define m$cfft_(...) OCPI_XM_PREFIX( m$cfft ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$cfft ( RCCWorker*, real_4*, const int_4*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$rfft_(...) OCPI_XM_PREFIX( m$rfft ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$rfft ( RCCWorker*, real_4*, const int_4*, const int_4* );

/* ----------------------------------------------------------------------- */

#define m$arslt_(...) OCPI_XM_PREFIX( m$arslt ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$arslt ( RCCWorker*, const char*, const char*, int_4, int_4 );

/* ----------------------------------------------------------------------- */

#define m$brslt_(...) OCPI_XM_PREFIX( m$brslt ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$brslt ( RCCWorker*, const char*, const int_4*, int_4 );

/* ----------------------------------------------------------------------- */

#define m$drslt_(...) OCPI_XM_PREFIX( m$drslt ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$drslt ( RCCWorker*, const char*, const double*, int_4 );

/* ----------------------------------------------------------------------- */

#define m$frslt_(...) OCPI_XM_PREFIX( m$frslt ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$frslt ( RCCWorker*, const char*, const float*, int_4 );

/* ----------------------------------------------------------------------- */

#define m$irslt_(...) OCPI_XM_PREFIX( m$irslt ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$irslt ( RCCWorker*, const char*, const int_2*, int_4 );

/* ----------------------------------------------------------------------- */

#define m$lrslt_(...) OCPI_XM_PREFIX( m$lrslt ) ( p_wctx, __VA_ARGS__ )

void ocpi_xm_m$lrslt ( RCCWorker*, const char*, const int_4*, int_4 );

/* ---- Non intercepted non-XM routines ---------------------------------- */

void sscal_ ( const int_4* count, const float* scalar, void* vector, const int_4* stride );

#ifdef __cplusplus
}
#endif

#endif // End: #ifndef INCLUDED_OCPI_XM_INTERCEPT_FORTRAN_H
