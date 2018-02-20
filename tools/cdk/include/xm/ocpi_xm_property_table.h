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

#ifndef INCLUDED_OCPI_XM_PROPERTY_TABLE_H
#define INCLUDED_OCPI_XM_PROPERTY_TABLE_H

#ifdef __cplusplus
  extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

struct RCCWorker;

#define XM_WIDGET_TABLE_INDEX ( 2 )
#define XM_RESULT_TABLE_INDEX ( 3 )
#define XM_SWITCH_TABLE_INDEX ( 4 )
#define XM_PROPERTY_TABLE_INDEX ( 5 )
#define XM_COMMANDLINE_TABLE_INDEX ( 6 )

typedef enum
{
  XM_PROP_TYPE_STRING,
  XM_PROP_TYPE_F32,
  XM_PROP_TYPE_F64,
  XM_PROP_TYPE_S8,
  XM_PROP_TYPE_U8,
  XM_PROP_TYPE_S16,
  XM_PROP_TYPE_U16,
  XM_PROP_TYPE_S32,
  XM_PROP_TYPE_U32,
  XM_PROP_TYPE_S64,
  XM_PROP_TYPE_U64,
  XM_PROP_TYPE_BOOL,
  XM_PROP_TYPE_NONE
} XmPropertyDataType;

typedef enum
{
  XM_PROP_COMMANDLINE,
  XM_PROP_WIDGET,
  XM_PROP_SWITCH,
  XM_PROP_RESULT

} XmPropertyType;

#define RCC_XM_PROPERTY_N_BYTES ( 80 )

#define RCC_XM_PROPERTY_NAME_LEN ( 32 )

typedef struct XmProperty
{
  char name [ RCC_XM_PROPERTY_NAME_LEN ];
  /* Name of the property (name (if any) used in the XM code) */
  size_t offset;
  /* Offset of the property in the worker's property space */
  XmPropertyDataType data_type;
  /* Data type of the property */

} XmProperty;

typedef union
{
  char str [ RCC_XM_PROPERTY_N_BYTES ];
  float f32;
  double f64;
  int8_t s8;
  uint8_t u8;
  int16_t s16;
  uint16_t u16;
  int32_t s32;
  uint32_t u32;
  int64_t s64;
  uint64_t u64;
  uint8_t b;
} XmPropertyValue;

typedef struct XmPropertyTable
{
  size_t n_properties;

  XmProperty properties [ 0 ];

} XmPropertyTable;


typedef struct XmCommandLine
{
  char name [ RCC_XM_PROPERTY_NAME_LEN ];
  /* Name of the XM command line argument */
  char property [ RCC_XM_PROPERTY_NAME_LEN ];
  /* Name of the property that this argument shadows */

  size_t index;
  /* Which comand line argument is it (used by *pick)? */

  XmPropertyDataType data_type;
  /* Data type of the argument (only used when not a property) */
  char value [ RCC_XM_PROPERTY_N_BYTES ];
  /* Value if command line argument is not a property */

} XmCommandLine;

typedef struct XmCommandLineTable
{
  size_t n_arguments;

  XmCommandLine arguments [ 0 ];

} XmCommandLineTable;


typedef struct XmSwitch
{
  char name [ RCC_XM_PROPERTY_NAME_LEN ];
  /* Name of the XM command line argument */
  char property [ RCC_XM_PROPERTY_NAME_LEN ];
  /* Name of the property that this argument shadows */

  XmPropertyDataType data_type;
  /* Data type of the argument (only used when not a property) */
  char value [ RCC_XM_PROPERTY_N_BYTES ];
  /* Value if command line argument is not a property */

} XmSwitch;

typedef struct XmSwitchTable
{
  size_t n_switches;

  XmSwitch switches [ 0 ];

} XmSwitchTable;

typedef struct XmWidget
{
  char name [ RCC_XM_PROPERTY_NAME_LEN ];
  /* Name of the XM command line argument */
  char property [ RCC_XM_PROPERTY_NAME_LEN ];
  /* Name of the property that this argument shadows */

  XmPropertyDataType data_type;
  /* Data type of the argument (only used when not a property) */
  char value [ RCC_XM_PROPERTY_N_BYTES ];
  /* Value if command line argument is not a property */

} XmWidget;

typedef struct XmWidgetTable
{
  size_t n_widgets;

  XmWidget widgets [ 0 ];

} XmWidgetTable;

typedef struct XmResult
{
  char name [ RCC_XM_PROPERTY_NAME_LEN ];
  /* Name of the XM command line argument */
  char property [ RCC_XM_PROPERTY_NAME_LEN ];
  /* Name of the property that this argument shadows */

  XmPropertyDataType data_type;
  /* Data type of the argument (only used when not a property) */
  char value [ RCC_XM_PROPERTY_N_BYTES ];
  /* Value if command line argument is not a property */

} XmResult;

typedef struct XmResultTable
{
  size_t n_results;

  XmResult results [ 0 ];

} XmResultTable;

/* Used for widgets and switches */
int ocpi_xm_get_property_by_name ( RCCWorker* wctx,
                                   const char* name,
                                   XmPropertyType type,
                                   XmPropertyValue* value );

/* Used mostly for command line *pick() */
int ocpi_xm_get_property_by_index ( RCCWorker* wctx,
                                    size_t index,
                                    XmPropertyType type,
                                    XmPropertyValue* value );

int ocpi_xm_find_property_by_name ( RCCWorker* wctx,
                                    const char* name,
                                    XmPropertyType type,
                                    uint32_t* index,
                                    XmPropertyTable** p_tab );


/* Used mostly for command line *pick() */
int ocpi_xm_get_commandline_by_index ( RCCWorker* wctx,
                                       size_t index,
                                       XmPropertyValue* value );

int ocpi_xm_get_switch_by_name ( RCCWorker* wctx,
                                 const char* name,
                                 XmPropertyValue* value );

int ocpi_xm_find_widget_by_name ( RCCWorker* wctx,
                                  const char* name,
                                  XmWidget** pp_tab );


/* Use by the *rslt() functions */
int ocpi_xm_set_result_by_name ( RCCWorker* wctxp_wctx,
                                 const char* label,
                                 XmPropertyValue* value );

#ifdef __cplusplus
  }
#endif

#endif /* End: #ifndef INCLUDED_OCPI_XM_PROPERTY_TABLE_H */
