
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
    Contains the declarations of the RCC Worker interface as specified in
    the CP289U specification.

************************************************************************** */


#ifndef CPI_RCC_WORKER__INTERFACE_H
#define CPI_RCC_WORKER__INTERFACE_H

#include <stddef.h>

#if defined (WIN32)
    /**
     * 8 bit signed integer data type.
     */

    typedef signed char int8_t;

    /**
     * 8 bit unsigned integer data type.
     */

    typedef unsigned char uint8_t;

    /**
     * 16 bit signed integer data type.
     */

    typedef short int16_t;

    /**
     * 16 bit unsigned integer data type.
     */

    typedef unsigned short uint16_t;

    /**
     * 32 bit signed integer data type.
     */

    typedef int int32_t;

    /**
     * 32 bit unsigned integer data type.
     */

    typedef unsigned int uint32_t;

    /**
     * 64 bit signed integer data type.
     */

    typedef long long int64_t;

    /**
     * 64 bit unsigned integer data type.
     */

    typedef unsigned long long uint64_t;
#else
#ifndef _WRS_KERNEL
#include <stdint.h>
#endif
#endif

#ifdef WORKER_INTERNAL
#define RCC_CONST
#else
#define RCC_CONST const
#endif

typedef uint16_t  RCCOrdinal;
typedef uint8_t   RCCBoolean;
typedef void      *RCCBufferId;
typedef float     RCCFloat;
typedef double    RCCDouble;
typedef char      RCCChar;

// do compile time checks for float, double, and char
#define RCC_VERSION 1
#define RCC_NO_EXCEPTION (0)
#define RCC_SYSTEM_EXCEPTION (1)
#define RCC_NO_ORDINAL ((RCCOrdinal)(-1))

typedef enum {
  RCC_OK,
  RCC_ERROR,
  RCC_FATAL,
  RCC_DONE,
  RCC_ADVANCE
} RCCResult;

typedef uint32_t RCCPortMask;
typedef struct RCCWorker RCCWorker;
typedef struct RCCPort RCCPort;

typedef struct {
    RCCPortMask *portMasks;
    RCCBoolean  timeout;
    uint32_t    usecs;
} RCCRunCondition;

typedef RCCResult RCCMethod(RCCWorker *_this);
typedef RCCResult RCCRunMethod(RCCWorker *_this,
			       RCCBoolean timedout,
			       RCCBoolean *newRunCondition);
typedef RCCResult RCCPortMethod(RCCWorker *_this,
				RCCPort *port,
				RCCResult reason);

typedef struct {
  void *data;
  uint32_t maxLength;

  /* private member for container use */
  void *id_; 
} RCCBuffer;

struct RCCPort {
  RCCBuffer current;

  RCC_CONST struct {
    uint32_t length;
    union {
      RCCOrdinal operation;
      RCCOrdinal exception;
    } u;
  } input;
  struct {
    uint32_t length;
    union {
      RCCOrdinal operation;
      RCCOrdinal exception;
    } u;
  } output;
  RCCPortMethod *callBack;

  /* Used by the container */
  void* opaque;
};

typedef struct {
  void (*release)(RCCBuffer *);
  void (*send)(RCCPort *, RCCBuffer*, RCCOrdinal op, uint32_t length);
  RCCBoolean (*request)(RCCPort *port, uint32_t maxlength);
  RCCBoolean (*advance)(RCCPort *port, uint32_t maxlength);
  RCCBoolean (*wait)(RCCPort *, uint32_t max, uint32_t usecs);
  void (*take)(RCCPort *,RCCBuffer *old_buffer, RCCBuffer *new_buffer);
} RCCContainer;

struct RCCWorker {
  void * RCC_CONST         properties;
  void * RCC_CONST       * RCC_CONST memories;
  RCC_CONST RCCContainer * container;
  RCCRunCondition        * runCondition;
  RCCPortMask              connectedPorts;
  char                   * errorString;
  RCCPort                  ports[1];
};


typedef struct {
  RCCOrdinal port;
  uint32_t maxLength;
  uint32_t minBuffers;
  // others...
} RCCPortInfo;

typedef struct {
  /* Information for consistency checking */
  uint32_t version;
  uint16_t numInputs, numOutputs;
  uint32_t propertySize, *memSizes;
  RCCBoolean threadProfile;
  /* Methods */
  RCCMethod *initialize, *start, *stop, *release, *test,
    *afterConfigure, *beforeQuery;
  RCCRunMethod *run;
  /* Implementation information for container behavior */
  RCCRunCondition *runCondition;
  RCCPortInfo *portInfo;

  /* Mask indicating which ports may be left unconnected */
  RCCPortMask optionallyConnectedPorts;

} RCCDispatch;

typedef struct {
  const char *name;
  RCCDispatch *dispatch;
} RCCEntryTable;

#endif

