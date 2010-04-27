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


/* ****

  This file is licensed from, and is a trade secret of:

    Mercury Computer Systems, Incorporated
    199 Riverneck Road
    Chelmsford, Massachusetts 01824-2820
    United States of America
    Telephone + 1 978 256-1300
    Telecopy/FAX + 1 978 256-3599
    US Customer Support (800) 872-0040

  Refer to your Software License Agreements for provisions on use,
  duplication, third party rights and disclosure. This file: (a) was
  developed at private expense, and no part was developed with government
  funds, (b) is a trade secret of Mercury Computer Systems, Inc. for the
  purposes of the Freedom of Information Act, (c) is "commercial computer
  software" subject to limited utilization as provided in the above noted
  Software License Agreements and (d) is in all respects data belonging
  exclusively to either Mercury Computer Systems, Inc. or its third party
  providers.

  Copyright (c) Mercury Computer Systems, Inc., Chelmsford MA., 1984-2008,
  and all third party embedded software sources. All rights reserved under
  the Copyright laws of US. and international treaties.

************************************************************************** */

/**
  @file

  @brief
    Contains the declarations of the WCI interface as specified in
    the WCI API functional specification.

************************************************************************** */

#ifndef INCLUDED_WCI_H
#define INCLUDED_WCI_H

#ifdef __cplusplus
  extern "C" {
#endif

/* ---- WCI - primitive types -------------------------------------------- */

typedef float WCI_f32;
/**< 32-bit floating point */
typedef double WCI_f64;
/**< 64-bit floating point */

typedef unsigned int WCI_boolean;
/**< Boolean */

typedef unsigned char WCI_u8;
/**< 8-bit unsigned integer */
typedef unsigned short WCI_u16;
/**< 16-bit unsigned integer */
typedef unsigned int WCI_u32;
/**< 32-bit unsigned integer */
typedef unsigned long long WCI_u64;
/**< 64-bit unsigned integer */

/**
  @typedef WCI_timeout

  The WCI_timeout type is used to indicate an amount of time after which
  blocking calls should return with a WCI_ERROR_TIMEOUT error code. The
  timeout unit is milliseconds.  Zero indicates no timeout.

************************************************************************** */

typedef WCI_u32 WCI_timeout;

/* ---- WCI - enumerations ----------------------------------------------- */


/**
  @enum WCI_data_type

  The WCI_data_type type is used to indicate the format of the data passed
  to wci_read(), wci_write(), wci_read_list(), and wci_write_list(). The
  data type information permits the WCI API to perform endian conversion
  when there is an endian mismatch between the host and worker.

************************************************************************** */

typedef enum
{
  WCI_DATA_TYPE_F32,
  /**< 32-bit floating point */
  WCI_DATA_TYPE_F64,
  /**< 64-bit floating point */
  WCI_DATA_TYPE_U8,
  /**< 8-bit unsigned integer */
  WCI_DATA_TYPE_U16,
  /**< 16-bit unsigned integer */
  WCI_DATA_TYPE_U32,
  /**< 32-bit unsigned integer */
  WCI_DATA_TYPE_U64
  /**< 64-bit unsigned integer */

} WCI_data_type;

/**
  @enum WCI_options

  The WCI_options type is a bit mask of options.  A value of zero indicates
  defaults for all options.

************************************************************************** */

typedef enum
{
  WCI_DEFAULT = 0,
  /**< Use the default options of a function. */
  WCI_USE_POLLING = ( 1 << 1 ),
  /**< WCI implementation will poll instead of block. */
  WCI_DO_NOT_RESET = ( 1 << 2 )
  /**< wci_close() will not put the RPL worker into reset. */
} WCI_options;

/**
  @enum WCI_event

  The WCI_event type is an enumeration of the various types of
  asynchronous events produced by a worker.

************************************************************************** */

typedef enum
{
  WCI_EVENT_READ,
  /**< Read operation. */
  WCI_EVENT_WRITE,
  /**< Write operation. */
  WCI_EVENT_CONTROL,
  /**< Control operation. */
  WCI_EVENT_NOP,
  /**< No-op operation. */
  WCI_EVENT_ATTENTION,
  /**< Attention operation. */
  WCI_EVENT_OVERRUN
  /**< Overrun (property space, request queue, or notification queue). */
} WCI_event;

/**
  @enum WCI_control

  The WCI_control type is an enumeration for worker control operations.

************************************************************************** */

typedef enum
{
  WCI_CONTROL_INITIALIZE,
  /**< Tell worker to perform initialization. */
  WCI_CONTROL_START,
  /**< Tell worker to start. */
  WCI_CONTROL_STOP,
  /**< Tell worker to stop. */
  WCI_CONTROL_RELEASE,
  /**< Release the worker (worker goes back into reset). */
  WCI_CONTROL_TEST,
  /**< Tell worker to perform a built-in test. */
  WCI_CONTROL_AFTER_CONFIG,
  /**< Tell worker property configuration is complete. */
  WCI_CONTROL_BEFORE_QUERY
  /**< Tell worker to prepare for a property read request. */

} WCI_control;

/**
  @enum WCI_attribute

  The WCI_attribute type is an enumeration that indicates the attributes
  of the worker session handle.

************************************************************************** */

typedef enum
{
  WCI_SUB32_ACCESS = 1,
  /**< Sub-32 bit property space access is supported. */
  WCI_BIG_ENDIAN = 2,
  /**< Worker uses big endian data ordering. */
  WCI_CONTROL_SUPPORT = 4
  /**< Worker supports control operations */

} WCI_attribute;

/**
  @enum WCI_error

  The WCI_error type is a type with a success value of zero enabling
  logical boolean tests in C to used:  true is error, false is success.
  False/errors are enumerated.

************************************************************************** */

typedef int WCI_error;
/**< Represents the return value from the WCI API */

#define WCI_SUCCESS ( 0 )
/**< Successful operation. */
#define WCI_FAILURE ( 1 )
/**< Generic error return. */
#define WCI_ERROR_TIMEOUT ( 2 )
/**< Timeout exceeded. */
#define WCI_ERROR_OUT_OF_MEMORY ( 3 )
/**< Memory allocation failure. */
#define WCI_ERROR_SUB32_ACCESS ( 4 )
/**< Sub-32 bit property space access is not supported. */
#define WCI_ERROR_NULL_HANDLE ( 5 )
/**< WCI_worker handle is null. */
#define WCI_ERROR_INVALID_HANDLE ( 6 )
/**< WCI_worker handle is invalid. */
#define WCI_ERROR_UNKNOWN_WORKER_TYPE ( 7 )
/**< String does not identify a valid worker type. */
#define WCI_ERROR_UNKNOWN_WORKER ( 8 )
/**< String does not identify a valid worker. */
#define WCI_ERROR_PROPERTY_OVERRUN ( 9 )
/**< Attempt to access an invalid property location. */
#define WCI_ERROR_REQUEST_ERROR ( 10 )
/**< Last non-NOP request completed with a worker indicating an error. */
#define WCI_ERROR_UNABLE_TO_FIND_WORKERS ( 11 )
/**< Unable to find workers. Is the bitstream loaded? */
#define WCI_ERROR_UNABLE_TO_MAP_WORKER ( 12 )
/**< Unable to memory map worker. Is the worker ID valid? */
#define WCI_ERROR_INTERNAL_NON_WCI_ERROR ( 13 )
/**< Internal error. A non-CPI operation returned an error. */
#define WCI_ERROR_UNABLE_TO_MAP_DEVICE ( 14 )
/**< Unable to memory map the device. Is the worker ID valid? */
#define WCI_ERROR_INVALID_CONTROL_SEQUENCE ( 15 )
/**< Control operations have been issued in the wrong order. */
#define WCI_ERROR_UNUSABLE_WORKER ( 16 )
/**< Worker is unusable due to an error. Reload the worker. */
#define WCI_ERROR_WORKER_NOT_INITIALIZED ( 17 )
/**< Initialize control operation must be issued before worker is usable. */
#define WCI_ERROR_TEST_NOT_IMPLEMENTED ( 18 )
/**< The "test" control operation is not supported by the worker. */
#define WCI_ERROR_RELEASE_NOT_IMPLEMENTED ( 19 )
/**< The "release" control operation is not supported by the worker. */
#define WCI_ERROR_NOTIFICATION_SETUP ( 20 )
/**< Error setting up for notifications. */
#define WCI_ERROR_UNKNOWN ( 21 )
/**< An unexpected error occurred. */

/* ---- WCI - structures ------------------------------------------------- */

/**
  @struct WCI_worker

  The WCI_worker type is partially opaque, it represents a handle/pointer
  to an active session used to access the worker.  It is defined as a
  pointer type.

************************************************************************** */

typedef struct WCI__worker
{
  WCI_attribute attributes;
  /**< Session handle attributes. */

  WCI_u32 n_property_bytes;
  /**< Size of bytes of the worker∆s property space */

  volatile WCI_u8* p_property_writable;
  /* Non-null if worker supports MMIO writes to its property space. */

  const volatile WCI_u8* p_property_readable;
  /* Non-null if worker supports MMIO reads from its property space. */

} *WCI_worker;

/**
  @struct WCI_status

  The WCI_status type is used to describe the status of the worker.

************************************************************************** */

typedef struct
{
  WCI_event event;
  /**< Last completed event. */
  WCI_boolean error;
  /**< Sticky error value. false = no error, true = error. */
  WCI_boolean pending;
  /**< Indicates one or more events are not completed */

  /* Content of info is determined by the event type */
  union
  {
    WCI_u32 offset;
    /**< Offset of completed read request */
    WCI_control control;
    /**< Opcode for the completed control operation */
    WCI_u32 attention;
    /**< Worker attention state. zero = no attention, non-zero = attention */
    WCI_u32 overrun;
    /**< Request/notification/property overrun. zero = no overrun,
         non-zero = overrun */
  } info;

  /* Content of data is determined by the event type */
  union
  {
    WCI_u8 data_u8;
    /**< 8-bit unsigned integer value returned from read request. */
    WCI_u16 data_u16;
    /**< 16-bit unsigned integer value returned from read request. */
    WCI_u32 data_u32;
    /**< 32-bit unsigned integer value returned from read request. */
    WCI_f32 data_f32;
    /**< 32-bit floating-point value returned from read request. */
  } data;

} WCI_status;

/**
  @struct WCI_access

  The WCI_access type is used to describe an access (read or write) to the
  worker∆s property space.

************************************************************************** */

typedef struct
{
  WCI_u32 offset;
  /**< Zero based offset into worker's property space for I/O operation. */
  WCI_u32 n_bytes;
  /**< Size in bytes of I/O request. */
  WCI_data_type data_type;
  /**< Data type descriptor (used for endian conversion). */
  void* p_data;
  /**< Pointer to user provided data buffer for the I/O request. */
} WCI_access;

/* ---- WCI - prototypes ------------------------------------------------- */

/**
  @brief
    Establish a session to control and configure a worker.

  The wci_open() function established a session to control and configure a
  worker identified by the p_name argument.  The interpretation of the name
  argument is platform-specific.  The n_msecs argument specifies the global
  timeout value in milliseconds.  The session handle is returned at
  *ph_worker.  The wci_close() function must be called to release resources
  allocated in wci_open(). The call to wci_open() deasserts the reset signal
  on the RPL worker.

  @param [ in ] p_name
                Platform specific string that identifies a worker.
                Example: FPGA (RPL) worker
                wci-rpl://[rapidio|pcie]:<device_id>/<worker_id>
                Example: GPP (RCC) worker
                wci-rcc://[rapidio|pcie]:<device_id>/<worker_id>

  @param [ in ] n_msecs
                Timeout value in milliseconds. Zero indicates an infinite
                timeout. The timeout value determines the amount of time
                after which blocking calls should return with a
                WCI_ERROR_TIMEOUT error code.

  @param [ in ] options
                Options must be one of:
                - WCI_DEFAULT (default) @n
                  WCI implementation uses blocking notifications.
                - WCI_USE_POLLING @n
                  WCI implementation uses polling notifications.

  @param [ out ] ph_worker
                 Session handle is returned in this parameter.

  @return
    Session handle is returned in this parameter.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_close()

************************************************************************** */

extern WCI_error wci_open ( const char* p_name,
                            WCI_timeout n_msecs,
                            WCI_options options,
                            WCI_worker* ph_worker );

/**
  @brief
    Ends a session created by wci_open() and releases resources.

    The wci_close() function destroys the session created by wci_open() and
    releases all resources.  If it fails, the resources may not all be
    recovered. By default, the call to wci_close() asserts the reset signal
    on the RPL worker.

  @param [ in ] h_worker
                Session handle returned from wci_open().

  @param [ in ] options
                Options must be one of:
                - WCI_DEFAULT (default) @n
                  WCI implementation will put the RPL worker into reset.
                - WCI_DO_NOT_RESET @n
                  WCI implementation will not put the RPL worker into reset.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_open()

************************************************************************** */

extern WCI_error wci_close ( WCI_worker h_worker,
                             WCI_options options );

/**
  @brief
    Invokes control operation on a worker.

  The function wci_control() invokes the control operation indicated by
  the operation argument on the worker indicated by the h_worker argument.
  The control operation is synchronous and the function will not
  return until the operation is complete, or until a timeout has occurred.
  If the worker encounters an error in carrying out the operation, an error
  code will be returned.

  @param [ in ] h_worker
                Session handle returned from wci_open().

  @param [ in ] operation
                Operation must be one of:
                - WCI_CONTROL_INITIALIZE @n
                - WCI_CONTROL_START @n
                - WCI_CONTROL_STOP @n
                - WCI_CONTROL_TEST @n
                - WCI_CONTROL_RELEASE @n
                - WCI_CONTROL_AFTER_CONFIG @n
                - WCI_CONTROL_BEFORE_QUERY @n

  @param [ in ] options
                Reserved. Please specify WCI_DEFAULT.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_status()

************************************************************************** */

extern WCI_error wci_control ( WCI_worker h_worker,
                               WCI_control operation,
                               WCI_options options );

/**
  @brief
    Retrieves the current status of a worker.

  The function wci_status() retrieves the current status of a worker, and
  may be used to test for the completion of asynchronous control and
  configuration operations.

  @param [ in ] h_worker
                Session handle returned from wci_open().

  @param [ out ] p_status
                 The current status of the worker is returned in this
                 parameter. The caller must provide the WCI_status structure.

  @return
    WCI_status structure populated with the worker's current status.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_control()

************************************************************************** */

extern WCI_error wci_status ( WCI_worker h_worker,
                              WCI_status* p_status );

/**
  @brief
    Returns a string describing the error code.

    The function wci_strerror() returns a pointer to an immutable static
    string that describes the error code.

    @param [ in ] error_number
                  Error code whose description string will be returned.

    @return
      An immutable string describing the error code passed in the argument
      error_number or "Unknown return code" if the error code is not
      recognized.

    @sa WCI_error

************************************************************************** */

extern const char* wci_strerror ( WCI_error error_number );

/**
  @brief
    Reads n_bytes into p_data from the worker∆s property space starting at
    the specified offset.

  The function wci_read() reads n_bytes number of bytes into the p_data
  buffer from the worker∆s property space at the specified offset. In a
  mixed endian situation the data_type parameter will be used to convert
  the data to the proper endian. The read operation is synchronous and the
  function will not return until the operation is complete, or until a
  timeout has occurred. If the worker encounters an error in carrying out
  the operation, an error code will be returned.

  @param [ in ] h_worker
                Session handle returned from wci_open().

  @param [ in ] offset
                Offset into the worker's property space.

  @param [ in ] n_bytes
                Number of bytes to read.

  @param [ in ] data_type
                Indicates the type of the data.

  @param [ in ] options
                Reserved. Please specify WCI_DEFAULT.

  @param [ out ] p_data
                 The data copied from the worker's property area will be
                 copied into this buffer provided by the caller.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_read_list()
  @sa wci_write()
  @sa wci_write_list()

************************************************************************** */

extern WCI_error wci_read ( WCI_worker h_worker,
                            WCI_u32 offset,
                            WCI_u32 n_bytes,
                            WCI_data_type data_type,
                            WCI_options options,
                            void* p_data );

/**
  @brief
    Writes n_bytes from p_data from the worker∆s property space at
    the specified offset.

  The function wci_write() writes n_bytes number of bytes from the p_data
  buffer to the worker∆s property space at the specified offset. In a mixed
  endian situation the data_type parameter will be used to convert the data
  to the proper endian. The write operation is synchronous and the function
  will not return until the operation is complete, or until a timeout has
  occurred. If the worker encounters an error in carrying out the operation,
  an error code will be returned.

  @param [ in ] h_worker
                Session handle returned from wci_open().

  @param [ in ] offset
                Offset into the worker's property space.

  @param [ in ] n_bytes
                Number of bytes to write.

  @param [ in ] data_type
                Indicates the type of the data.

  @param [ in ] options
                Reserved. Please specify WCI_DEFAULT.

  @param [ in ] p_data
                The data to copy to the worker's property.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_read()
  @sa wci_read_list()
  @sa wci_write_list()

************************************************************************** */

extern WCI_error wci_write ( WCI_worker h_worker,
                             WCI_u32 offset,
                             WCI_u32 n_bytes,
                             WCI_data_type data_type,
                             WCI_options options,
                             const void* p_data );

/**
  @brief
    Processes a list of read WCI_access requests.

  The function wci_read_list() processes a list of read WCI_access requests.
  The number of read requests in the list is n_list_elems. In a mixed
  endian situation the data_type member of the WCI_access structure will be
  used to convert the data to the proper endian. The read list operation is
  synchronous and the function will not return until the operation is
  complete, or until a timeout has occurred. If the worker encounters an
  error in carrying out the operation, an error code will be returned.

  @param [ in ] h_worker
                Session handle returned from wci_open().

  @param [ in ] p_accesses
                Array of WCI_access structures that specify the read
                operations that will take place on the worker's
                property space. .

  @param [ in ] n_list_elems
                Specifies the number of WCI_access structures in the list
                pointed to by p_accesses that will be processed by the call.

  @param [ in ] options
                Reserved. Please specify WCI_DEFAULT.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_read()
  @sa wci_write()
  @sa wci_write_list()

************************************************************************** */

extern WCI_error wci_read_list ( WCI_worker h_worker,
                                 const WCI_access* p_accesses,
                                 WCI_u32 n_list_elems,
                                 WCI_options options );

/**
  @brief
    Processes a list of write WCI_access requests.

  The function wci_write_list() processes a list of write WCI_access
  requests.  The number of write requests in the list is n_list_elems.
  In a mixed endian situation the data_type member of the WCI_access
  structure will be used to convert the data to the proper endian. The
  write list operation is  synchronous and the function will not return
  until the operation is  complete, or until a timeout has occurred. If
  the worker encounters an error in carrying out the operation, an error
  code will be returned.

  @param [ in ] h_worker
                Session handle returned from wci_open().

  @param [ in ] p_accesses
                Array of WCI_access structures that specify the write
                operations that will take place on the worker's
                property space.

  @param [ in ] n_list_elems
                Specifies the number of WCI_access structures in the list
                pointed to by p_accesses that will be processed by the call.

  @param [ in ] options
                Reserved. Please specify WCI_DEFAULT.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_read()
  @sa wci_read_list()
  @sa wci_write()

************************************************************************** */

extern WCI_error wci_write_list ( WCI_worker h_worker,
                                  const WCI_access* p_accesses,
                                  WCI_u32 n_list_elems,
                                  WCI_options options );

/**
  @brief
    Sets an 8-bit unsigned integer scalar property.

  The function wci_set_u8() sets an 8-bit unsigned integer scalar property
  at the specified offset into the worker's property space. The set operation
  is synchronous and the function will not return until the operation is
  complete, or until a timeout has occurred. If the worker produces and
  error in response to accepting this configuration value, an error code
  will be returned.

  @param [ in ] h_worker
                Session handle returned from wci_open().

  @param [ in ] offset
                Offset into the worker's property space.

  @param [ in ] value
                8-bit unsigned integer value to write into the worker's
                property space.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_get_u16()
  @sa wci_get_u32()
  @sa wci_get_u64()
  @sa wci_get_f32()
  @sa wci_get_f64()
  @sa wci_set_u8()
  @sa wci_set_u16()
  @sa wci_set_u32()
  @sa wci_set_u64()
  @sa wci_set_f32()
  @sa wci_set_f64()

************************************************************************** */

extern WCI_error wci_set_u8 ( WCI_worker h_worker,
                              WCI_u32 offset,
                              WCI_u8 value );

/**
  @brief
    Sets an 16-bit unsigned integer scalar property.

  The function wci_set_u16() sets an 16-bit unsigned integer scalar property
  at the specified offset into the worker's property space. The set operation
  is synchronous and the function will not return until the operation is
  complete, or until a timeout has occurred. If the worker produces and
  error in response to accepting this configuration value, an error code
  will be returned.

  @param [ in ] h_worker
                Session handle returned from wci_open().

  @param [ in ] offset
                Offset into the worker's property space.

  @param [ in ] value
                16-bit unsigned integer value to write into the worker's
                property space.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_get_u8()
  @sa wci_get_u16()
  @sa wci_get_u32()
  @sa wci_get_u64()
  @sa wci_get_f32()
  @sa wci_get_f64()
  @sa wci_set_u8()
  @sa wci_set_u32()
  @sa wci_set_u64()
  @sa wci_set_f32()
  @sa wci_set_f64()

************************************************************************** */

extern WCI_error wci_set_u16 ( WCI_worker h_worker,
                               WCI_u32 offset,
                               WCI_u16 value );

/**
  @brief
    Sets an 32-bit unsigned integer scalar property.

  The function wci_set_u32() sets an 32-bit unsigned integer scalar property
  at the specified offset into the worker's property space. The set operation
  is synchronous and the function will not return until the operation is
  complete, or until a timeout has occurred. If the worker produces and
  error in response to accepting this configuration value, an error code
  will be returned.

  @param [ in ] h_worker
                Session handle returned from wci_open().

  @param [ in ] offset
                Offset into the worker's property space.

  @param [ in ] value
                32-bit unsigned integer value to write into the worker's
                property space.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_get_u8()
  @sa wci_get_u16()
  @sa wci_get_u32()
  @sa wci_get_u64()
  @sa wci_get_f32()
  @sa wci_get_f64()
  @sa wci_set_u8()
  @sa wci_set_u16()
  @sa wci_set_u64()
  @sa wci_set_f32()
  @sa wci_set_f64()

************************************************************************** */

extern WCI_error wci_set_u32 ( WCI_worker h_worker,
                               WCI_u32 offset,
                               WCI_u32 value );

/**
  @brief
    Sets an 64-bit unsigned integer scalar property.

  The function wci_set_u64() sets an 64-bit unsigned integer scalar property
  at the specified offset into the worker's property space. The set operation
  is synchronous and the function will not return until the operation is
  complete, or until a timeout has occurred. If the worker produces and
  error in response to accepting this configuration value, an error code
  will be returned.


  @param [ in ] h_worker
                Session handle returned from wci_open().

  @param [ in ] offset
                Offset into the worker's property space.

  @param [ in ] value
                64-bit unsigned integer value to write into the worker's
                property space.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_get_u8()
  @sa wci_get_u16()
  @sa wci_get_u32()
  @sa wci_get_u64()
  @sa wci_get_f32()
  @sa wci_get_f64()
  @sa wci_set_u8()
  @sa wci_set_u16()
  @sa wci_set_u32()
  @sa wci_set_f32()
  @sa wci_set_f64()

************************************************************************** */

extern WCI_error wci_set_u64 ( WCI_worker h_worker,
                               WCI_u32 offset,
                               WCI_u64 value );

/**
  @brief
    Sets an 32-bit floating-point scalar property.

  The function wci_set_f32() sets an 32-bit floating-point scalar property
  at the specified offset into the worker's property space. The set operation
  is synchronous and the function will not return until the operation is
  complete, or until a timeout has occurred. If the worker produces and
  error in response to accepting this configuration value, an error code
  will be returned.

  @param [ in ] h_worker
                Session handle returned from wci_open().

  @param [ in ] offset
                Offset into the worker's property space.

  @param [ in ] value
                32-bit floating-point value to write into the worker's
                property space.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_get_u8()
  @sa wci_get_u16()
  @sa wci_get_u32()
  @sa wci_get_u64()
  @sa wci_get_f32()
  @sa wci_get_f64()
  @sa wci_set_u8()
  @sa wci_set_u16()
  @sa wci_set_u32()
  @sa wci_set_u64()
  @sa wci_set_f64()

************************************************************************** */

extern WCI_error wci_set_f32 ( WCI_worker h_worker,
                               WCI_u32 offset,
                               WCI_f32 value );

/**
  @brief
    Sets an 64-bit floating-point scalar property.

  The function wci_set_f64() sets an 64-bit floating-point scalar property
  at the specified offset into the worker's property space. The set operation
  is synchronous and the function will not return until the operation is
  complete, or until a timeout has occurred. If the worker produces and
  error in response to accepting this configuration value, an error code
  will be returned.

  @param [ in ] h_worker
                Session handle returned from wci_open().

  @param [ in ] offset
                Offset into the worker's property space.

  @param [ in ] value
                64-bit floating-point value to write into the worker's
                property space.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_get_u8()
  @sa wci_get_u16()
  @sa wci_get_u32()
  @sa wci_get_u64()
  @sa wci_get_f32()
  @sa wci_get_f64()
  @sa wci_set_u8()
  @sa wci_set_u16()
  @sa wci_set_u32()
  @sa wci_set_u64()
  @sa wci_set_f32()

************************************************************************** */

extern WCI_error wci_set_f64 ( WCI_worker h_worker,
                               WCI_u32 offset,
                               WCI_f64 value );

/**
  @brief
    Retrieves an 8-bit unsigned integer scalar property.

  The function wci_get_u8() returns an 8-bit unsigned integer scalar
  property from the specified offset into the worker's property space.
  The get operation is synchronous and the function will not return until
  the operation is complete, or until a timeout has occurred. If the worker
  encounters an error in carrying out the operation, an error code will be
  returned.

  @param [ in ] h_worker
                Session handle returned from wci_open().

  @param [ in ] offset
                Offset into the worker's property space.

  @param [ out ] p_value
                 The 8-bit unsigned integer scalar value read from the
                 worker's property space is returned in this parameter.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_get_u16()
  @sa wci_get_u32()
  @sa wci_get_u64()
  @sa wci_get_f32()
  @sa wci_get_f64()
  @sa wci_set_u8()
  @sa wci_set_u16()
  @sa wci_set_u32()
  @sa wci_set_u64()
  @sa wci_set_f32()
  @sa wci_set_f64()

************************************************************************** */

extern WCI_error wci_get_u8 ( WCI_worker h_worker,
                              WCI_u32 offset,
                              WCI_u8* p_value );

/**
  @brief
    Retrieves an 16-bit unsigned integer scalar property.

  The function wci_get_u16() returns an 16-bit unsigned integer scalar
  property from the specified offset into the worker's property space.
  The get operation is synchronous and the function will not return until
  the operation is complete, or until a timeout has occurred. If the worker
  encounters an error in carrying out the operation, an error code will be
  returned.

  @param [ in ] h_worker
                Session handle returned from wci_open().

  @param [ in ] offset
                Offset into the worker's property space.

  @param [ out ] p_value
                 The 16-bit unsigned integer scalar value read from the
                 worker's property space is returned in this parameter.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_get_u8()
  @sa wci_get_u32()
  @sa wci_get_u64()
  @sa wci_get_f32()
  @sa wci_get_f64()
  @sa wci_set_u8()
  @sa wci_set_u16()
  @sa wci_set_u32()
  @sa wci_set_u64()
  @sa wci_set_f32()
  @sa wci_set_f64()

************************************************************************** */

extern WCI_error wci_get_u16 ( WCI_worker h_worker,
                               WCI_u32 offset,
                               WCI_u16* p_value );

/**
  @brief
    Retrieves an 32-bit unsigned integer scalar property.

  The function wci_get_u32() returns an 32-bit unsigned integer scalar
  property from the specified offset into the worker's property space.
  The get operation is synchronous and the function will not return until
  the operation is complete, or until a timeout has occurred. If the worker
  encounters an error in carrying out the operation, an error code will be
  returned.

  @param [ in ] h_worker
                Session handle returned from wci_open().

  @param [ in ] offset
                Offset into the worker's property space.

  @param [ out ] p_value
                 The 32-bit unsigned integer scalar value read from the
                 worker's property space is returned in this parameter.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_get_u8()
  @sa wci_get_u16()
  @sa wci_get_u64()
  @sa wci_get_f32()
  @sa wci_get_f64()
  @sa wci_set_u8()
  @sa wci_set_u16()
  @sa wci_set_u32()
  @sa wci_set_u64()
  @sa wci_set_f32()
  @sa wci_set_f64()

************************************************************************** */

extern WCI_error wci_get_u32 ( WCI_worker h_worker,
                               WCI_u32 offset,
                               WCI_u32* p_value );
/**
  @brief
    Retrieves an 64-bit unsigned integer scalar property.

  The function wci_get_u64() returns an 64-bit unsigned integer scalar
  property from the specified offset into the worker's property space.
  The get operation is synchronous and the function will not return until
  the operation is complete, or until a timeout has occurred. If the worker
  encounters an error in carrying out the operation, an error code will be
  returned.

  @param [ in ] h_worker
                Session handle returned from wci_open().

  @param [ in ] offset
                Offset into the worker's property space.

  @param [ out ] p_value
                 The 64-bit unsigned integer scalar value read from the
                 worker's property space is returned in this parameter.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_get_u8()
  @sa wci_get_u16()
  @sa wci_get_u32()
  @sa wci_get_f32()
  @sa wci_get_f64()
  @sa wci_set_u8()
  @sa wci_set_u16()
  @sa wci_set_u32()
  @sa wci_set_u64()
  @sa wci_set_f32()
  @sa wci_set_f64()

************************************************************************** */

extern WCI_error wci_get_u64 ( WCI_worker h_worker,
                               WCI_u32 offset,
                               WCI_u64* p_value );

/**
  @brief
    Retrieves an 32-bit floating-point scalar property.

  The function wci_get_f32() returns an 32-bit floating-point scalar
  property from the specified offset into the worker's property space.
  The get operation is synchronous and the function will not return until
  the operation is complete, or until a timeout has occurred. If the worker
  encounters an error in carrying out the operation, an error code will be
  returned.

  @param [ in ] h_worker
                Session handle returned from wci_open().

  @param [ in ] offset
                Offset into the worker's property space.

  @param [ out ] p_value
                 The 32-bit floating-point scalar value read from the
                 worker's property space is returned in this parameter.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_get_u8()
  @sa wci_get_u16()
  @sa wci_get_u32()
  @sa wci_get_u64()
  @sa wci_get_f64()
  @sa wci_set_u8()
  @sa wci_set_u16()
  @sa wci_set_u32()
  @sa wci_set_u64()
  @sa wci_set_f32()
  @sa wci_set_f64()

************************************************************************** */

extern WCI_error wci_get_f32 ( WCI_worker h_worker,
                               WCI_u32 offset,
                               WCI_f32* p_value );
/**
  @brief
    Retrieves an 64-bit floating-point scalar property.

  The function wci_get_f64() returns an 64-bit floating-point scalar
  property from the specified offset into the worker's property space.
  The get operation is synchronous and the function will not return until
  the operation is complete, or until a timeout has occurred. If the worker
  encounters an error in carrying out the operation, an error code will be
  returned.

  @param [ in ] h_worker
                Session handle returned from wci_open().

  @param [ in ] offset
                Offset into the worker's property space.

  @param [ out ] p_value
                 The 64-bit floating-point scalar value read from the
                 worker's property space is returned in this parameter.

  @retval
    Returns WCI_SUCCESS on success and an error code on failure.

  @sa wci_get_u8()
  @sa wci_get_u16()
  @sa wci_get_u32()
  @sa wci_get_u64()
  @sa wci_get_f32()
  @sa wci_set_u8()
  @sa wci_set_u16()
  @sa wci_set_u32()
  @sa wci_set_u64()
  @sa wci_set_f32()
  @sa wci_set_f64()

************************************************************************** */

extern WCI_error wci_get_f64 ( WCI_worker h_worker,
                               WCI_u32 offset,
                               WCI_f64* p_value );

#ifdef __cplusplus
  }
#endif

#endif /* End: #ifndef INCLUDED_WCI_H */

