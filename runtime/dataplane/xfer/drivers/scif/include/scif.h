/*
 * Copyright 2010-2013 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, version 2.1.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * Disclaimer: The codes contained in these modules may be specific
 * to the Intel Software Development Platform codenamed Knights Ferry,
 * and the Intel product codenamed Knights Corner, and are not backward
 * compatible with other Intel products. Additionally, Intel will NOT
 * support the codes or instruction set in future products.
 *
 * Intel offers no warranty of any kind regarding the code. This code is
 * licensed on an "AS IS" basis and Intel is not obligated to provide
 * any support, assistance, installation, training, or other services
 * of any kind. Intel is also not obligated to provide any updates,
 * enhancements or extensions. Intel specifically disclaims any warranty
 * of merchantability, non-infringement, fitness for any particular
 * purpose, and any other warranty.
 *
 * Further, Intel disclaims all liability of any kind, including but
 * not limited to liability for infringement of any proprietary rights,
 * relating to the use of the code, even if Intel is notified of the
 * possibility of such liability. Except as expressly stated in an Intel
 * license agreement provided with this code and agreed upon with Intel,
 * no license, express or implied, by estoppel or otherwise, to any
 * intellectual property rights is granted herein.
 */

/*
 * Revised 15:05 11/24/2010
 * Derived from SCIF SAS v0.41 with additional corrections
 */

#ifndef __SCIF_H__
#define __SCIF_H__

#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <poll.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCIF_ACCEPT_SYNC	1
#define SCIF_SEND_BLOCK		1
#define SCIF_RECV_BLOCK		1

/* Start: Deprecated Temporary definition for compatability */
#define ACCEPT_SYNC		SCIF_ACCEPT_SYNC
#define SEND_BLOCK		SCIF_SEND_BLOCK
#define RECV_BLOCK		SCIF_RECV_BLOCK
/* End: Deprecated Temporary definition for compatability */

enum {
	SCIF_PROT_READ  = (1<<0),
	SCIF_PROT_WRITE = (1<<1)
};

enum {
	SCIF_MAP_FIXED = 0x10,
	SCIF_MAP_KERNEL	= 0x20
};

enum {
	SCIF_FENCE_INIT_SELF = (1<<0),
	SCIF_FENCE_INIT_PEER = (1<<1)
};

enum {
	SCIF_FENCE_RAS_SELF = (1<<2),
	SCIF_FENCE_RAS_PEER = (1<<3)
};

enum {
	SCIF_SIGNAL_LOCAL = (1<<4),
	SCIF_SIGNAL_REMOTE = (1<<5)
};

#define SCIF_RMA_USECPU     1
#define SCIF_RMA_USECACHE   (1<<1)
#define SCIF_RMA_SYNC       (1<<2)
#define SCIF_RMA_ORDERED    (1<<3)
//! @cond (Prevent doxygen from including these)
#define SCIF_POLLIN		POLLIN
#define SCIF_POLLOUT		POLLOUT
#define SCIF_POLLERR		POLLERR
#define SCIF_POLLHUP		POLLHUP
#define SCIF_POLLNVAL		POLLNVAL

/* SCIF Reserved Ports */
/* COI */
#define SCIF_COI_PORT_0		40
#define SCIF_COI_PORT_1		41
#define SCIF_COI_PORT_2		42
#define SCIF_COI_PORT_3		43
#define SCIF_COI_PORT_4		44
#define SCIF_COI_PORT_5		45
#define SCIF_COI_PORT_6		46
#define SCIF_COI_PORT_7		47
#define SCIF_COI_PORT_8		48
#define SCIF_COI_PORT_9		49

/* OFED */
#define SCIF_OFED_PORT_0	60
#define SCIF_OFED_PORT_1	61
#define SCIF_OFED_PORT_2	62
#define SCIF_OFED_PORT_3	63
#define SCIF_OFED_PORT_4	64
#define SCIF_OFED_PORT_5	65
#define SCIF_OFED_PORT_6	66
#define SCIF_OFED_PORT_7	67
#define SCIF_OFED_PORT_8	68
#define SCIF_OFED_PORT_9	69

/* NETDEV */
#define SCIF_NETDEV_PORT_0	80
#define SCIF_NETDEV_PORT_1	81
#define SCIF_NETDEV_PORT_2	82
#define SCIF_NETDEV_PORT_3	83
#define SCIF_NETDEV_PORT_4	84
#define SCIF_NETDEV_PORT_5	85
#define SCIF_NETDEV_PORT_6	86
#define SCIF_NETDEV_PORT_7	87
#define SCIF_NETDEV_PORT_8	88
#define SCIF_NETDEV_PORT_9	89

/* RAS */
#define SCIF_RAS_PORT_0		100
#define SCIF_RAS_PORT_1		101
#define SCIF_RAS_PORT_2		102
#define SCIF_RAS_PORT_3		103
#define SCIF_RAS_PORT_4		104
#define SCIF_RAS_PORT_5		105
#define SCIF_RAS_PORT_6		106
#define SCIF_RAS_PORT_7		107
#define SCIF_RAS_PORT_8		108
#define SCIF_RAS_PORT_9		109

/* Power Management */
#define SCIF_PM_PORT_0		120
#define SCIF_PM_PORT_1		121
#define SCIF_PM_PORT_2		122
#define SCIF_PM_PORT_3		123
#define SCIF_PM_PORT_4		124
#define SCIF_PM_PORT_5		125
#define SCIF_PM_PORT_6		126
#define SCIF_PM_PORT_7		127
#define SCIF_PM_PORT_8		128
#define SCIF_PM_PORT_9		129

/* Board Tools */
#define SCIF_BT_PORT_0		130
#define SCIF_BT_PORT_1		131
#define SCIF_BT_PORT_2		132
#define SCIF_BT_PORT_3		133
#define SCIF_BT_PORT_4		134
#define SCIF_BT_PORT_5		135
#define SCIF_BT_PORT_6		136
#define SCIF_BT_PORT_7		137
#define SCIF_BT_PORT_8		138
#define SCIF_BT_PORT_9		139

/* MIC Boot/Configuration support */
#define MPSSD_MONRECV		160
#define MIC_NOTIFY		161
#define MPSSD_CRED		162
#define MPSSD_MONSEND		163
#define MPSSD_MICCTRL		164
#define MPSSD_RESV5		165
#define MPSSD_RESV6		166
#define MPSSD_RESV7		167
#define MPSSD_RESV8		168
#define MPSSD_RESV9		169

#define SCIF_ADMIN_PORT_END	1024

/* MYO */
#define SCIF_MYO_PORT_0		1025
#define SCIF_MYO_PORT_1		1026
#define SCIF_MYO_PORT_2		1027
#define SCIF_MYO_PORT_3		1028
#define SCIF_MYO_PORT_4		1029
#define SCIF_MYO_PORT_5		1030
#define SCIF_MYO_PORT_6		1031
#define SCIF_MYO_PORT_7		1032
#define SCIF_MYO_PORT_8		1033
#define SCIF_MYO_PORT_9		1034

/* SSG Tools */
#define SCIF_ST_PORT_0		1044
#define SCIF_ST_PORT_1		1045
#define SCIF_ST_PORT_2		1046
#define SCIF_ST_PORT_3		1047
#define SCIF_ST_PORT_4		1048
#define SCIF_ST_PORT_5		1049
#define SCIF_ST_PORT_6		1050
#define SCIF_ST_PORT_7		1051
#define SCIF_ST_PORT_8		1052
#define SCIF_ST_PORT_9		1053

/* End of SCIF Reserved Ports */
#define SCIF_PORT_RSVD		1088
//! @endcond

typedef int scif_epd_t;

struct scif_pollepd {
	scif_epd_t epd;   /* endpoint descriptor */
	short events;     /* requested events */
	short revents;    /* returned events */
};


#define SCIF_OPEN_FAILED ((scif_epd_t)-1)
#define SCIF_REGISTER_FAILED ((off_t)-1)
#define SCIF_MMAP_FAILED ((void *)-1)

struct scif_portID {
	uint16_t node; /* node on which port resides */
	uint16_t port; /* Local port number */
};

/* Start: Deprecated Temporary definition for compatability */
#define portID scif_portID
typedef struct portID portID_t;
/* End: Deprecated Temporary definition for compatability */

/**
 * scif_open - Create an endpoint
 *
 *\return
 * The scif_open() function creates a new endpoint.
 *
 * Upon successful completion, scif_open() returns an endpoint descriptor to
 * be used in subsequent SCIF functions calls to refer to that endpoint;
 * otherwise: in user mode SCIF_OPEN_FAILED (that is ((scif_epd_t)-1)) is
 * returned and errno is set to indicate the error; in kernel mode a NULL
 * scif_epd_t is returned.
 *
 *\par Errors:
 *- ENOMEM
 * - Insufficient kernel memory was available.
 *- ENXIO
 * - Version mismatch between micscif driver and libscif.
 */
scif_epd_t scif_open(void);

/**
 * scif _bind - Bind an endpoint to a port
 *	\param epd			endpoint descriptor
 *	\param pn			port number
 *
 * scif_bind() binds endpoint epd to port pn, where pn is a port number on the
 * local node. If pn is zero, a port number greater than or equal to
 * SCIF_PORT_RSVD is assigned and returned. Each endpoint may be bound to
 * exactly one local port. Ports less than 1024 when requested can only be bound
 * by system (or root) processes or by processes executed by privileged users.
 *
 *\return
 * Upon successful completion, scif_bind() returns the port number to which epd
 * is bound; otherwise: in user mode -1 is returned and errno is set to
 * indicate the error; in kernel mode the negative of one of the following
 * errors is returned.
 *
 *\par Errors:
 *- EBADF
 * - epd is not a valid endpoint descriptor
 *- EINVAL
 * - epd is not a valid endpoint descriptor, or
 * - The endpoint or the port are already bound.
 *- EISCONN
 * - The endpoint is already connected.
 *- ENOSPC
 * - No port number available for assignment (when pn==0).
 *- ENOTTY
 * - epd is not a valid endpoint descriptor
 *- EACCES
 * - The port requested is protected and the user is not the superuser.
*/
int scif_bind(scif_epd_t epd, uint16_t pn);

/**
 * scif_listen - Listen for connections on an endpoint
 *
 *	\param epd		endpoint descriptor
 *	\param backlog		maximum pending connection requests
 *
 * scif_listen() marks the endpoint epd as a listening endpoint - that is, as
 * an endpoint that will be used to accept incoming connection requests. Once
 * so marked, the endpoint is said to be in the listening state and may not be
 * used as the endpoint of a connection.
 *
 * The endpoint, epd, must have been bound to a port.
 *
 * The backlog argument defines the maximum length to which the queue of
 * pending connections for epd may grow.  If a connection request arrives when
 * the queue is full, the client may receive an error with an indication that
 * the connection was refused.
 *
 *\return
 * Upon successful completion, scif_listen() returns 0; otherwise: in user mode
 * -1 is returned and errno is set to indicate the error; in kernel mode the
 * negative of one of the following errors is returned.
 *
 *\par Errors:
 *- EBADF
 * - epd is not a valid endpoint descriptor
 *- EINVAL
 * - epd is not a valid endpoint descriptor, or
 * - The endpoint is not bound to a port
 *- EISCONN
 * - The endpoint is already connected or listening
 *- ENOTTY
 * - epd is not a valid endpoint descriptor
*/
int scif_listen(scif_epd_t epd, int backlog);

/**
 * scif_connect - Initiate a connection on a port
 *	\param epd		endpoint descriptor
 *	\param dst		global id of port to which to connect
 *
 * The scif_connect() function requests the connection of endpoint epd to remote
 * port dst. If the connection is successful, a peer endpoint, bound to dst, is
 * created on node dst.node. On successful return, the connection is complete.
 *
 * If the endpoint epd has not already been bound to a port, scif_connect()
 * will bind it to an unused local port.
 *
 * A connection is terminated when an endpoint of the connection is closed,
 * either explicitly by scif_close(), or when a process that owns one of the
 * endpoints of a connection is terminated.
 *
 * On Linux:
 * scif_connect is non blocking if the file is set in non blocking mode using
 * fcntl. The file handle passed to fcntl can be obtained using
 * scif_get_fd().
 *
 * The initiating thread can either check for connection status
 * by calling connect in a loop while errno is set to EINPROGRESS
 * or block on a poll()/select() system call with POLLOUT as the requested
 * event. Once unblocked, calling connect again will return 0
 * if the connection was successful, or return -1 with errno set appropriately
 *
 *\return
 * Upon successful completion, scif_connect() returns the port ID to which the
 * endpoint, epd, is bound; otherwise: in user mode -1 is returned and errno is
 * set to indicate the error; in kernel mode the negative of one of the
 * following errors is returned.
 *
 *\par Errors:
 *- EBADF
 * - epd is not a valid endpoint descriptor
 *- ECONNREFUSED
 * - The destination was not listening for connections or refused the
 *   connection request.
 *- EINTR
 * - Interrupted function
 *- EINVAL
 * - epd is not a valid endpoint descriptor, or
 * - dst.port is not a valid port ID
 *- EISCONN
 * - The endpoint is already connected
 *- ENOBUFS
 * - No buffer space is available
 *- ENODEV
 * - The destination node does not exist, or
 * - The node is lost.
 *- ENOSPC
 * - No port number available for assignment (when pn==0).
 *- ENOTTY
 * - epd is not a valid endpoint descriptor
 *- EOPNOTSUPP
 * - The endpoint is listening and cannot be connected
*/
int scif_connect(scif_epd_t epd, struct scif_portID *dst);

/**
 * scif_accept - Accept a connection on an endpoint
 *	\param epd		endpoint descriptor
 *	\param peer		global id of port to which connected
 *	\param newepd		new connected endpoint descriptor
 *	\param flags		flags
 *
 * The scif_accept() call extracts the first connection request on the queue of
 * pending connections for the port on which epd is listening. scif_accept()
 * creates a new endpoint, bound to the same port as epd, and allocates a new
 * SCIF endpoint descriptor, returned in newepd, for the endpoint.  The new
 * endpoint is connected to the endpoint through which the connection was
 * requested. epd is unaffected by this call, and remains in the listening
 * state.
 *
 * On successful return, peer holds the global port identifier (node id and
 * local port number) of the port which requested the connection.
 *
 * If the peer endpoint which requested the connection is closed, the endpoint
 * returned by scif_accept() is closed.
 *
 * The number of connections that can (subsequently) be accepted on epd is only
 * limited by system resources (memory).
 *
 * The flags argument is formed by OR'ing together zero or more of the
 * following values:
 *- SCIF_ACCEPT_SYNC: block until a connection request is presented. If
 *		      SCIF_ACCEPT_SYNC is not in flags, and no pending
 *		      connections are present on the queue, scif_accept()fails
 *		      with an EAGAIN error
 *
 * On Linux in user mode, the select() and poll() functions can be used to
 * determine when there is a connection request. On Microsoft Windows* and on
 * Linux in kernel mode, the scif_poll() function may be used for this purpose.
 * A readable event will be delivered when a connection is requested.
 *
 *\return
 * Upon successful completion, scif_accept() returns 0; otherwise: in user mode
 * -1 is returned and errno is set to indicate the error; in kernel mode the
 *  negative of one of the following errors is returned.
 *
 *\par Errors:
 *- EAGAIN
 * - SCIF_ACCEPT_SYNC is not set and no connections are present to be accepted, or
 * - SCIF_ACCEPT_SYNC is not set and remote node failed to complete its
 *   connection request
 *- EBADF
 * - epd is not a valid endpoint descriptor
 *- EINTR
 * - Interrupted function
 *- EINVAL
 * - epd is not a valid endpoint descriptor, or
 * - epd is not a listening endpoint
 * - flags is invalid
 * - peer is NULL
 * - newepd is NULL
 *- ENOBUFS
 * - No buffer space is available
 *- ENODEV
 * - The requesting node is lost.
 *- ENOMEM
 * - Not enough space
 *- ENOTTY
 * - epd is not a valid endpoint descriptor
 *- ENOENT
 * - Secondary part of epd registeration failed.
*/
int scif_accept(scif_epd_t epd, struct scif_portID *peer, scif_epd_t
*newepd, int flags);

/**
 * scif_close - Close an endpoint
 *	\param epd	endpoint descriptor
 *
 * scif_close() closes an endpoint and performs necessary teardown of
 * facilities associated with that endpoint.
 *
 * If epd is a listening endpoint then it will no longer accept connection
 * requests on the port to which it is bound. Any pending connection requests
 * are rejected.
 *
 * If epd is a connected endpoint, then its peer endpoint is also closed. RMAs
 * which are in-process through epd or its peer endpoint will complete before
 * scif_close() returns. Registered windows of the local and peer endpoints are
 * released as if scif_unregister() was called against each window.
 *
 * Closing an endpoint does not affect mappings to remote memory. These remain
 * until explicitly removed by calling scif_munmap().
 *
 * If the peer endpoint's receive queue is not empty at the time that epd is
 * closed, then the peer endpoint can be passed as the endpoint parameter to
 * scif_recv() until the receive queue is empty.
 *
 * If epd is bound to a port, then the port is returned to the pool of
 * available ports.
 *
 * epd is freed and may no longer be accessed.
 *
 *\return
 * Upon successful completion, scif_close() returns 0; otherwise: in user mode
 * -1 is returned and errno is set to indicate the error; in kernel mode the
 * negative of one of the following errors is returned.
 *
 *\par Errors:
 *- EBADF
 * - epd is not a valid endpoint descriptor
 *- EINVAL
 * - epd is not a valid endpoint descriptor
 */
int scif_close(scif_epd_t epd);

/**
 * scif_send - Send a message
 *	\param epd		endpoint descriptor
 *	\param msg		message buffer address
 *	\param len		message length
 *	\param flags		blocking mode flags
 *
 * scif_send() sends data to the peer of endpoint epd. Up to len bytes of data
 * are copied from memory starting at address msg. On successful execution the
 * return value of scif_send() is the number of bytes that were sent, and is
 * zero if no bytes were sent because len was zero. scif_send() may be called
 * only when the endpoint is in a connected state.
 *
 * If a scif_send() call is non-blocking, then it sends only those bytes which
 * can be sent without waiting, up to a maximum of len bytes.
 *
 * If a scif_send() call is blocking, then it normally returns after sending
 * all len bytes. If a blocking call is interrupted or the connection is
 * forcibly closed, the call is considered successful if some bytes were sent
 * or len is zero, otherwise the call is considered unsuccessful.
 *
 * On Linux in user mode, the select() and poll() functions can be used to
 * determine when the send queue is not full. On Microsoft Windows* and on
 * Linux in kernel mode, the scif_poll() function may be used for this purpose.
 *
 * It is recommended that scif_send()/scif_recv() only be used for short
 * control-type message communication between SCIF endpoints. The SCIF RMA
 * APIs are expected to provide better performance for transfer sizes of
 * 1024 bytes or longer.
 *
 * The flags argument is formed by ORing together zero or more of the following
 * values:
 *- SCIF_SEND_BLOCK: block until the entire message is sent.
 *
 *\return
 * Upon successful completion, scif_send() returns the number of bytes sent;
 * otherwise: in user mode -1 is returned and errno is set to indicate the
 * error; in kernel mode the negative of one of the following errors is
 * returned.
 *
 *\par Errors:
 *- EBADF
 * - epd is not a valid endpoint descriptor
 *- ECONNRESET
 * - A connection was forcibly closed by a peer.
 *- EFAULT
 * - An invalid address was specified for a parameter.
 *- EINTR
 * - epd was closed by scif_close()
 *- EINVAL
 * - epd is not a valid endpoint descriptor, or
 * - flags is invalid
 * - len is negative
 *- ENODEV
 * - The remote node is lost.
 *- ENOMEM
 * - Not enough space
 *- ENOTCONN
 * - The endpoint is not connected
 *- ENOTTY
 * - epd is not a valid endpoint descriptor
 */
int scif_send(scif_epd_t epd, void *msg, int len, int flags);

/**
 * scif_recv - Receive a message
 *	\param epd		endpoint descriptor
 *	\param msg		message buffer address
 *	\param len		message buffer length
 *	\param flags		blocking mode flags
 *
 * scif_recv() receives data from the peer of endpoint epd. Up to len bytes of
 * data are copied to memory starting at address msg. On successful execution
 * the return value of scif_recv() is the number of bytes that were received,
 * and is zero if no bytes were received because len was zero. scif_recv() may
 * be called only when the endpoint is in a connected state.
 *
 * If a scif_recv() call is non-blocking, then it receives only those bytes
 * which can be received without waiting, up to a maximum of len bytes.
 *
 * If a scif_recv() call is blocking, then it normally returns after receiving
 * all len bytes. If a blocking call is interrupted or the connection is
 * forcibly closed, the call is considered successful if some bytes were
 * received or len is zero, otherwise the call is considered unsuccessful;
 * subsequent calls to scif_recv() will successfully receive all data sent
 * through peer endpoint interruption or the connection was forcibly closed.
 *
 * On Linux in user mode, the select() and poll() functions can be used to
 * determine when data is available to be received. On Microsoft Windows* and
 * on Linux in kernel mode, the scif_poll() function may be used for this
 * purpose.
 *
 * It is recommended that scif_send()/scif_recv() only be used for short
 * control-type message communication between SCIF endpoints. The SCIF RMA
 * APIs are expected to provide better performance for transfer sizes of
 * 1024 bytes or longer.
 *
 * The flags argument is formed by ORing together zero or more of the following
 * values:
 *- SCIF_RECV_BLOCK: block until the entire message is received.
 *
 *\return
 * Upon successful completion, scif_recv() returns the number of bytes
 * received; otherwise: in user mode -1 is returned and errno is set to
 * indicate the error; in kernel mode the negative of one of the following
 * errors is returned.
 *
 *\par Errors:
 *- EAGAIN
 * - The destination node is returning from a low power state.
 *- EBADF
 * - epd is not a valid endpoint descriptor .
 *- ECONNRESET
 * - A connection was forcibly closed by a peer.
 *- EFAULT
 * - An invalid address was specified for a parameter.
 *- EINVAL
 * - epd is not a valid endpoint descriptor, or
 * - flags  is invalid, or
 * - len is negative.
 *- ENODEV
 * - The remote node is lost.
 *- ENOMEM
 * - Not enough space.
 *- ENOTCONN
 * - The endpoint is not connected.
 *- ENOTTY
 * - epd is not a valid endpoint descriptor
 */
int scif_recv(scif_epd_t epd, void *msg, int len, int flags);

/**
 * scif_register - Mark a memory region for remote access.
 *	\param epd		endpoint descriptor
 *	\param addr		starting virtual address
 *	\param len		length of range
 *	\param offset		offset of window
 *	\param prot_flags	read/write protection flags
 *	\param map_flags	mapping flags
 *
 * The scif_register() function opens a window, a range of whole pages of the
 * registered address space of the endpoint epd, starting at offset po and
 * continuing for len bytes. The value of po, further described below, is a
 * function of the parameters offset and len, and the value of map_flags. Each
 * page of the window represents the physical memory page which backs the
 * corresponding page of the range of virtual address pages starting at addr
 * and continuing for len bytes. addr and len are constrained to be multiples
 * of the page size. addr is interpreted as a user space address. A successful
 * scif_register() call returns po as the return value.
 *
 * When SCIF_MAP_FIXED is set in the map_flags argument, po will be offset
 * exactly, and offset is constrained to be a multiple of the page size. The
 * mapping established by scif_register() will not replace any existing
 * registration; an error is returned if any page within the range [offset,
 * offset+len-1] intersects an existing window.
 * Note: When SCIF_MAP_FIXED is set the current implementation limits
 * offset to the range [0..2^62-1] and returns EADDRINUSE if the offset
 * requested with SCIF_MAP_FIXED is in the range [2^62..2^63-1].
 *
 * When SCIF_MAP_FIXED is not set, the implementation uses offset in an
 * implementation-defined manner to arrive at po. The po value so chosen will
 * be an area of the registered address space that the implementation deems
 * suitable for a mapping of len bytes. An offset value of 0 is interpreted as
 * granting the implementation complete freedom in selecting po, subject to
 * constraints described below. A non-zero value of offset is taken to be a
 * suggestion of an offset near which the mapping should be placed. When the
 * implementation selects a value for po, it does not replace any extant
 * window. In all cases, po will be a multiple of the page size.
 *
 * The physical pages which are so represented by a window are available for
 * access in calls to scif_mmap(), scif_readfrom(), scif_writeto(),
 * scif_vreadfrom(), and scif_vwriteto(). While a window is registered, the
 * physical pages represented by the window will not be reused by the memory
 * subsystem for any other purpose. Note that the same physical page may be
 * represented by multiple windows.
 *
 * Subsequent operations which change the memory pages to which virtual
 * addresses are mapped (such as mmap(), munmap(), scif_mmap() and
 * scif_munmap()) have no effect on existing windows.
 *
 * On Linux, if the process will fork(), it is recommended that the registered
 * virtual address range be marked with MADV_DONTFORK. Doing so will prevent
 * problems due to copy-on-write semantics.
 *
 * The prot_flags argument is formed by OR'ing together one or more of the
 * following values:
 *- SCIF_PROT_READ: allow read operations from the window
 *- SCIF_PROT_WRITE: allow write operations to the window
 *
 * The map_flags argument is formed by OR'ing together zero or more of
 * the following values:
 *- SCIF_MAP_FIXED: interpret offset exactly
 *
 *\return
 * Upon successful completion, scif_register() returns the offset at which the
 * mapping was placed (po); otherwise: in user mode SCIF_REGISTER_FAILED (that
 * is (off_t *)-1) is returned and errno is set to indicate the error; in
 * kernel mode the negative of one of the following errors is returned.
 *
 *\par Errors:
 *- EADDRINUSE
 * - SCIF_MAP_FIXED is set in map_flags, and pages in the range [offset,
 *   offset+len-1] are already registered
 *- EAGAIN
 * - The mapping could not be performed due to lack of resources
 *- EBADF
 * - epd is not a valid endpoint descriptor
 *- ECONNRESET
 * - A connection was forcibly closed by a peer.
 *- EFAULT
 * - Addresses in the range [addr , addr + len - 1] are invalid
 *- EINVAL
 * - epd is not a valid endpoint descriptor, or
 * - map_flags is invalid, or
 * - prot_flags is invalid, or
 * - SCIF_MAP_FIXED is set in flags, and offset is not a multiple of
 *   the page size, or
 * - addr is not a multiple of the page size, or
 * - len is not a multiple of the page size, or is 0, or
 * - offset is negative
 *- ENODEV
 * - The remote node is lost.
 *- ENOMEM
 * - Not enough space
 *- ENOTCONN
 * - The endpoint is not connected
 *- ENOTTY
 * - epd is not a valid endpoint descriptor
 */
off_t scif_register(scif_epd_t epd, void *addr, size_t len, off_t offset,
int prot_flags, int map_flags);

/**
 * scif_unregister - Mark a memory region for remote access.
 *	\param epd		endpoint descriptor
 *	\param offset		start of range to unregister
 *	\param len		length of range to unregister
 *
 * The scif_unregister() function closes those previously registered windows
 * which are entirely within the range [offset,offset+len-1]. It is an error to
 * specify a range which intersects only a subrange of a window.
 *
 * On a successful return, pages within the window may no longer be specified
 * in calls to scif_mmap(), scif_readfrom(), scif_writeto(), scif_vreadfrom(),
 * scif_vwriteto(), scif_get_pages, and scif_fence_signal(). The window, however,
 * continues to exist until all previous references against it are removed. A
 * window is referenced if there is a mapping to it created by scif_mmap(), or if
 * scif_get_pages() was called against the window (and the pages have not been
 * returned via scif_put_pages()). A window is also referenced while an RMA, in
 * which some range of the window is a source or destination, is in progress.
 * Finally a window is referenced while some offset in that window was specified
 * to scif_fence_signal(), and the RMAs marked by that call to
 * scif_fence_signal() have not completed. While a window is in this state, its
 * registered address space pages are not available for use in a new registered
 * window.
 *
 * When all such references to the window have been removed, its references to
 * all the physical pages which it represents are removed. Similarly, the
 * registered address space pages of the window become available for
 * registration in a new window.
 *
 *\return
 * Upon successful completion, scif_unregister() returns 0; otherwise: in user
 * mode -1 is returned and errno is set to indicate the error; in kernel mode
 * the negative of one of the following errors is returned. In the event of an
 * error, no windows are unregistered.
 *
 *\par Errors:
 *- EBADF
 * - epd is not a valid endpoint descriptor
 *- ECONNRESET
 * - A connection was forcibly closed by a peer.
 *- EINVAL
 * - epd  is not a valid endpoint descriptor, or
 * - The range [offset,offset+len-1] intersects a subrange of a window, or
 * - offset is negative
 *- ENODEV
 * -The remote node is lost.
 *- ENOTCONN
 * - The endpoint is not connected
 *- ENOTTY
 * - epd is not a valid endpoint descriptor
 *- ENXIO
 * - Addresses in the range [offset,offset+len-1] are invalid for the
 *   registered address space of epd.
 */
int scif_unregister(scif_epd_t epd, off_t offset, size_t len);

/**
 * scif_mmap - Map pages in virtual address space to a remote window
 *	\param addr		virtual address range base address
 *	\param len		length of range to be mapped
 *	\param prot_flags	read/write protection flags
 *	\param map_flags	mapping flags
 *	\param epd		endpoint descriptor
 *	\param offset		offset into remote registered address space
 *
 * The scif_mmap() function establishes a mapping between those whole pages of
 * the process starting at address pa and continuing for len bytes, and those
 * whole physical pages represented by pages of the registered address space of
 * the peer of the endpoint epd, starting at offset and continuing for len
 * bytes. The value of pa, further described below, is a function of the
 * parameters addr and len, and the value of map_flags. offset and len are
 * constrained to be multiples of the page size. A successful scif_mmap() call
 * returns pa as its result. Due to Microsoft Windows limitation, On
 * Microsoft Windows 7, len must be 4096. On Microsoft Windows 8 and later,
 * len must be less than or equal to 4GB-4KB.
 *
 * Each of the pages in the specified range [offset,offset+len-1] must be
 * within some registered window on the remote node. The range may intersect
 * multiple registered windows, but only if those windows are contiguous in the
 * registered address space.
 *
 * When SCIF_MAP_FIXED is set in the flags argument, pa shall be addr exactly,
 * and addr is constrained to be a multiple of the page size. The mapping
 * established by scif_mmap() will replace any existing mappings for those
 * pages of the address space of the process starting at addr and continuing
 * for len bytes.
 *
 * When SCIF_MAP_FIXED is not set, the implementation uses addr in an
 * implementation-defined manner to arrive at pa. The pa so chosen will be an
 * area of the address space that the implementation deems suitable for a
 * mapping of len bytes. An addr value of 0 is interpreted as granting the
 * implementation complete freedom in selecting pa, subject to constraints
 * described below. A non-zero value of addr is taken to be a suggestion
 * of a process address near which the mapping should be placed. When the
 * implementation selects a value for pa, it never places a mapping at address
 * 0, nor does it replace any extant mapping. In all cases, pa will be a
 * multiple of the page size.
 *
 * On successful return, CPU accesses to addresses within the mapped virtual
 * address range will read or write data at corresponding memory locations of
 * the remote node.
 *
 * The remote physical pages of a mapping created by scif_mmap() remain
 * available, and are not reused by the memory subsystem of the remote node,
 * until the mapping is changed by a subsequent call to scif_mmap(),
 * scif_munmap(), or standard functions such as mmap() and munmap().
 *
 * Mapped regions of a process are automatically unmapped when the process is
 * terminated. However, closing an endpoint does not automatically unmap any
 * region.
 *
 * prot_flags has one or more of the following possible values:
 *- SCIF_PROT_READ: allow the mapping if the referenced window has the
 *  SCIF_PROT_READ flag.
 *- SCIF_PROT_WRITE: allow the mapping if the referenced window has the
 *  SCIF_PROT_WRITE flag.
 *
 * The map_flags argument is formed by OR'ing together zero or more of the
 * following values:
 *- SCIF_MAP_FIXED: interpret addr exactly
 *
 *\return
 * Upon successful completion, scif_mmap() returns the address at which the
 * mapping was placed (pa); otherwise SCIF_MMAP_FAILED (that is (void *)-1) is
 * returned and errno is set to indicate the error.
 *\par Errors:
 *- EACCESS
 * - prot flags is not compatible with registered window protection
 *- EBADF
 * - epd is not a valid endpoint descriptor
 *- ECONNRESET
 * - A connection was forcibly closed by a peer.
 *- ENOMEM
 * - Insufficient kernel memory was available.
 *- EINVAL
 * - epd is not a valid endpoint descriptor, or
 * - map_flags is invalid, or
 * - prot_flags is invalid , or
 * - SCIF_MAP_FIXED is set and addr is not a multiple of the page size, or
 * - offset is not a multiple of the page size, or
 * - len is not a multiple of the page size, or
 * - len is not 4096 (Microsoft Windows 7), or
 * - len is greater than 4GB-4KB (Microsoft Windows 8 or later)
 *- ENODEV
 * - The remote node is lost.
 *- ENOTCONN
 * - The endpoint is not connected
 *- ENOTTY
 * - epd is not a valid endpoint descriptor
 *- ENXIO
 * - Addresses in the range [offset,offset+len-1] are invalid for the
registered
 *		address space of the peer of epd, or
 * - offset is negative
 */
void *scif_mmap(void *addr, size_t len, int prot_flags, int map_flags,
scif_epd_t epd, off_t offset);

/**
 * scif_munmap - Remove the mapping to a remote window
 *	\param addr			starting address of range to unmap
 *	\param len			length of range to unmap
 *
 * scif_munmap() removes any mapping to those entire pages containing any
 * portion of the address space, starting at addr and continuing for len bytes.
 * Subsequent reference to those pages may result in the generation of a signal
 * or error.
 *
 * If a page in the specified range was not mapped by scif_mmap(), the effect
 * will be as if the standard mmap() function were called on that page.
 *
 * All mapped regions of a process are automatically unmapped when the process
 * is terminated.
 *
 *\return
 * Upon successful completion, scif_unmap() returns 0. Otherwise -1 is
 * returned, and errno is set to indicate the error.
 *
 *\par Errors:
 *- EINVAL
 * - addr is not a multiple of the page size, or
 * - len is not a multiple of the page size
 */
int scif_munmap(void *addr, size_t len);

/**
 * scif_readfrom - Copy from a remote address space
 *	\param epd		endpoint descriptor
 *	\param loffset		offset in local registered address space to
 *				which to copy
 *	\param len		length of range to copy
 *	\param roffset		offset in remote registered address space
 *				from which to copy
 *	\param rma_flags	transfer mode flags
 *
 * scif_readfrom() copies len bytes from the remote registered address space of
 * the peer of endpoint epd, starting at the offset roffset to the local
 * registered address space of epd, starting at the offset loffset.
 *
 * Each of the specified ranges [loffset,loffset+len-1] and [roffset,roffset+
 * len-1] must be within some registered window or windows of the local and
 * remote nodes respectively. A range  may intersect multiple registered
 * windows, but only if those windows are contiguous in the registered address
 * space.
 *
 * If rma_flags includes SCIF_RMA_USECPU, then the data is copied using
 * programmed read/writes. Otherwise the data is copied using DMA. If rma_-
 * flags includes SCIF_RMA_SYNC, then scif_readfrom() will return after the
 * transfer is complete. Otherwise, the transfer may be performed asynchron-
 * ously. The order in which any two aynchronous RMA operations complete
 * is non-deterministic. The synchronization functions, scif_fence_mark()/
 * scif_fence_wait() and scif_fence_signal(), can be used to synchronize to
 * the completion of asynchronous RMA operations.
 *
 * The DMA transfer of individual bytes is not guaranteed to complete in
 * address order. If rma_flags includes SCIF_RMA_ORDERED, then the last
 * cacheline or partial cacheline of the source range will become visible on
 * the destination node after all other transferred data in the source
 * range has become visible on the destination node.
 *
 * The optimal DMA performance will likely be realized if both
 * loffset and roffset are cacheline aligned (are a multiple of 64). Lower
 * performance will likely be realized if loffset  and roffset are not
 * cacheline aligned but are separated by some multiple of 64. The lowest level
 * of performance is likely if loffset and roffset are not separated by a
 * multiple of 64.
 *
 * The rma_flags argument is formed by ORing together zero or more of the
 * following values:
 *- SCIF_RMA_USECPU: perform the transfer using the CPU, otherwise use the DMA
 *                   engine.
 *- SCIF_RMA_SYNC: perform the transfer synchronously, returning after the
 *                 transfer has completed. Passing this flag might result in
 *                 the API busy waiting and consuming CPU cycles while the DMA
 *                 transfer is in progress.
 *- SCIF_RMA_ORDERED: ensure that the last cacheline or partial cacheline of
 *                    the source range becomes visible on the destination node
 *                    after all other transferred data in the source range has
 *                    become visible on the destination
 *
 *\return
 * Upon successful completion, scif_readfrom() returns 0; otherwise: in user
 * mode -1 is returned and errno is set to indicate the error; in kernel mode
 * the negative of one of the following errors is returned.
 *
 *\par Errors
 *- EACCESS
 * - Attempt to write to a read-only range or read from a write-only range
 *- EBADF
 * - epd is not a valid endpoint descriptor
 *- ECONNRESET
 * - A connection was forcibly closed by a peer.
 *- EINVAL
 * - epd is not a valid endpoint descriptor, or
 * - rma_flags is invalid
 *- ENODEV
 * -The remote node is lost.
 *- ENOTCONN
 * - The endpoint is not connected
 *- ENOTTY
 * - epd is not a valid endpoint descriptor
 *- ENXIO
 * - The range [loffset,loffset+len-1] is invalid for the registered address
 *   space of epd, or,
 * - The range [roffset,roffset+len-1] is invalid for the registered address
 *   space of the peer of epd, or
 * - loffset or roffset is negative
*/
int scif_readfrom(scif_epd_t epd, off_t loffset, size_t len, off_t
roffset, int rma_flags);

/**
 * scif_writeto - Copy to a remote address space
 *	\param epd		endpoint descriptor
 *	\param loffset		offset in local registered address space
 *				from which to copy
 *	\param len		length of range to copy
 *	\param roffset		offset in remote registered address space to
 *				which to copy
 *	\param rma_flags	transfer mode flags
 *
 * scif_writeto() copies len bytes from the local registered address space of
 * epd, starting at the offset loffset to the remote registered address space
 * of the peer of endpoint epd, starting at the offset roffset.
 *
 * Each of the specified ranges [loffset,loffset+len-1] and [roffset,roffset+
 * len-1] must be within some registered window or windows of the local and
 * remote nodes respectively. A range may intersect multiple registered
 * windows, but only if those windows are contiguous in the registered address
 * space.
 *
 * If rma_flags includes SCIF_RMA_USECPU, then the data is copied using
 * programmed read/writes. Otherwise the data is copied using DMA. If rma_-
 * flags includes SCIF_RMA_SYNC, then scif_writeto() will return after the
 * transfer is complete. Otherwise, the transfer may be performed asynchron-
 * ously. The order in which any two aynchronous RMA operations complete
 * is non-deterministic. The synchronization functions, scif_fence_mark()/
 * scif_fence_wait() and scif_fence_signal(), can be used to synchronize to
 * the completion of asynchronous RMA operations.
 *
 * The DMA transfer of individual bytes is not guaranteed to complete in
 * address order. If rma_flags includes SCIF_RMA_ORDERED, then the last
 * cacheline or partial cacheline of the source range will become visible on
 * the destination node after all other transferred data in the source
 * range has become visible on the destination node.
 *
 * The optimal DMA performance will likely be realized if both
 * loffset and roffset are cacheline aligned (are a multiple of 64). Lower
 * performance will likely be realized if loffset and roffset are not cacheline
 * aligned but are separated by some multiple of 64. The lowest level of
 * performance is likely if loffset and roffset are not separated by a multiple
 * of 64.
 *
 * The rma_flags argument is formed by ORing together zero or more of the
 * following values:
 *- SCIF_RMA_USECPU: perform the transfer using the CPU, otherwise use the DMA
 *                   engine.
 *- SCIF_RMA_SYNC: perform the transfer synchronously, returning after the
 *                 transfer has completed. Passing this flag might result in
 *                 the API busy waiting and consuming CPU cycles while the DMA
 *                 transfer is in progress.
 *- SCIF_RMA_ORDERED: ensure that the last cacheline or partial cacheline of
 *                    the source range becomes visible on the destination node
 *                    after all other transferred data in the source range has
 *                    become visible on the destination
 *
 *\return
 * Upon successful completion, scif_readfrom() returns 0; otherwise: in user
 * mode -1 is returned and errno is set to indicate the error; in kernel mode
 * the negative of one of the following errors is returned.
 *
 *\par Errors:
 *- EACCESS
 * - Attempt to write to a read-only range or read from a write-only range
 *- EBADF
 * - epd is not a valid endpoint descriptor
 *- ECONNRESET
 * - A connection was forcibly closed by a peer.
 *- EINVAL
 * - epd is not a valid endpoint descriptor, or
 * - rma_flags is invalid
 *- ENODEV
 * - The remote node is lost.
 *- ENOTCONN
 * - The endpoint is not connected
 *- ENOTTY
 * - epd is not a valid endpoint descriptor
 *- ENXIO
 * - The range [loffset,loffset+len-1] is invalid for the registered address
 *   space of epd, or,
 * - The range [roffset , roffset + len -1] is invalid for the registered
 *   address space of the peer of epd, or
 * - loffset or roffset is negative
 */
int scif_writeto(scif_epd_t epd, off_t loffset, size_t len, off_t
roffset, int rma_flags);

/**
 * scif_vreadfrom - Copy from a remote address space
 *	\param epd		endpoint descriptor
 *	\param addr		address to which to copy
 *	\param len		length of range to copy
 *	\param roffset		offset in remote registered address space
 *				from which to copy
 *	\param rma_flags	transfer mode flags
 *
 * scif_vreadfrom() copies len bytes from the remote registered address
 * space of the peer of endpoint epd, starting at the offset roffset, to local
 * memory, starting at addr. addr is interpreted as a user space address.
 *
 * The specified range [roffset,roffset+len-1] must be within some registered
 * window or windows of the remote nodes respectively. The range may intersect
 * multiple registered windows, but only if those windows are contiguous in the
 * registered address space.
 *
 * If rma_flags includes SCIF_RMA_USECPU, then the data is copied using
 * programmed read/writes. Otherwise the data is copied using DMA. If rma_-
 * flags includes SCIF_RMA_SYNC, then scif_vreadfrom() will return after the
 * transfer is complete. Otherwise, the transfer may be performed asynchron-
 * ously. The order in which any two aynchronous RMA operations complete
 * is non-deterministic. The synchronization functions, scif_fence_mark()/
 * scif_fence_wait() and scif_fence_signal(), can be used to synchronize to
 * the completion of asynchronous RMA operations.
 *
 * The DMA transfer of individual bytes is not guaranteed to complete in
 * address order. If rma_flags includes SCIF_RMA_ORDERED, then the last
 * cacheline or partial cacheline of the source range will become visible on
 * the destination node after all other transferred data in the source
 * range has become visible on the destination node.
 *
 * If rma_flags includes SCIF_RMA_USECACHE, then the physical pages which back
 * the specified local memory range may be remain in a pinned state even after
 * the specified transfer completes. This may reduce overhead if some or all of
 * the same virtual address range is referenced in a subsequent call of
 * scif_vreadfrom() or scif_vwriteto().
 *
 * The optimal DMA performance will likely be realized if both
 * loffset and roffset are cacheline aligned (are a multiple of 64). Lower
 * performance will likely be realized if loffset  and roffset are not
 * cacheline aligned but are separated by some multiple of 64. The lowest level
 * of performance is likely if loffset and roffset are not separated by a
 * multiple of 64.
 *
 * The rma_flags argument is formed by ORing together zero or more of the
 * following values:
 *- SCIF_RMA_USECPU: perform the transfer using the CPU, otherwise use the DMA
 *                   engine.
 *- SCIF_RMA_USECACHE: enable registration caching
 *- SCIF_RMA_SYNC: perform the transfer synchronously, returning after the
 *                 transfer has completed. Passing this flag might result in
 *                 the API busy waiting and consuming CPU cycles while the DMA
 *                 transfer is in progress.
 *- SCIF_RMA_ORDERED: ensure that the last cacheline or partial cacheline of
 *                    the source range becomes visible on the destination node
 *                    after all other transferred data in the source range has
 *                    become visible on the destination
 *
 *\return
 * Upon successful completion, scif_vreadfrom() returns 0; otherwise: in user
 * mode -1 is returned and errno is set to indicate the error; in kernel mode
 * the negative of one of the following errors is returned.
 *
 *\par Errors:
 *- EACCESS
 * - Attempt to write to a read-only range or read from a write-only range
 *- EBADF
 * - epd is not a valid endpoint descriptor
 *- ECONNRESET
 * - A connection was forcibly closed by a peer.
 *- EFAULT
 * - Addresses in the range [addr,addr+len-1] are invalid
 *- EINVAL
 * - epd is not a valid endpoint descriptor, or
 * - rma_flags is invalid
 *- ENODEV
 * - The remote node is lost.
 *- ENOTCONN
 * - The endpoint is not connected
 *- ENOTTY
 * - epd is not a valid endpoint descriptor
 *- ENXIO
 * - Addresses in the range [roffset,roffset+len-1] are invalid for the
 *   registered address space of epd.
 */
int scif_vreadfrom(scif_epd_t epd, void *addr, size_t len, off_t offset,
int rma_flags);

/**
 * scif_vwriteto - Copy to a remote address space
 *	\param epd		endpoint descriptor
 *	\param addr		address from which to copy
 *	\param len		length of range to copy
 *	\param roffset		offset in remote registered address space to
 *				which to copy
 *	\param rma_flags	transfer mode flags
 *
 * scif_vwriteto() copies len bytes from the local memory, starting at addr, to
 * the remote registered address space of the peer of endpoint epd, starting at
 * the offset roffset. addr is interpreted as a user space address.
 *
 * The specified range [roffset,roffset+len-1] must be within some registered
 * window or windows of the remote nodes respectively. The range may intersect
 * multiple registered windows, but only if those windows are contiguous in the
 * registered address space.
 *
 * If rma_flags includes SCIF_RMA_USECPU, then the data is copied using
 * programmed read/writes. Otherwise the data is copied using DMA. If rma_-
 * flags includes SCIF_RMA_SYNC, then scif_vwriteto() will return after the
 * transfer is complete. Otherwise, the transfer may be performed asynchron-
 * ously. The order in which any two aynchronous RMA operations complete
 * is non-deterministic. The synchronization functions, scif_fence_mark()/
 * scif_fence_wait() and scif_fence_signal(), can be used to synchronize to
 * the completion of asynchronous RMA operations.
 *
 * The DMA transfer of individual bytes is not guaranteed to complete in
 * address order. If rma_flags includes SCIF_RMA_ORDERED, then the last
 * cacheline or partial cacheline of the source range will become visible on
 * the destination node after all other transferred data in the source
 * range has become visible on the destination node.
 *
 * If rma_flags includes SCIF_RMA_USECACHE, then the physical pages which back
 * the specified local memory range may be remain in a pinned state even after
 * the specified transfer completes. This may reduce overhead if some or all of
 * the same virtual address range is referenced in a subsequent call of
 * scif_vreadfrom() or scif_vwriteto().
 *
 * The optimal DMA performance will likely be realized if both
 * addr and offset are cacheline aligned (are a multiple of 64). Lower
 * performance will likely be realized if addr  and offset are not cacheline
 * aligned but are separated by some multiple of 64. The lowest level of
 * performance is likely if addr and offset are not separated by a multiple of
 * 64.
 *
 * The rma_flags argument is formed by ORing together zero or more of the
 * following values:
 *- SCIF_RMA_USECPU: perform the transfer using the CPU, otherwise use the DMA
 *                   engine.
 *- SCIF_RMA_USECACHE: allow registration caching
 *- SCIF_RMA_SYNC: perform the transfer synchronously, returning after the
 *                 transfer has completed. Passing this flag might result in
 *                 the API busy waiting and consuming CPU cycles while the DMA
 *                 transfer is in progress.
 *- SCIF_RMA_ORDERED: ensure that the last cacheline or partial cacheline of
 *                    the source range becomes visible on the destination node
 *                    after all other transferred data in the source range has
 *                    become visible on the destination
 *
 *\return
 * Upon successful completion, scif_vwriteto () returns 0; otherwise: in user
 * mode -1 is returned and errno is set to indicate the error; in kernel mode
 * the negative of one of the following errors is returned.
 *
 *\par Errors:
 *- EACCESS
 * - Attempt to write to a read-only range or read from a write-only range
 *- EBADF
 * - epd is not a valid endpoint descriptor
 *- ECONNRESET
 * - A connection was forcibly closed by a peer.
 *- EFAULT
 * - Addresses in the range [addr,addr+len-1] are invalid
 *- EINVAL
 * - epd is not a valid endpoint descriptor, or
 * - rma_flags is invalid
 *- ENODEV
 * - The remote node is lost.
 *- ENOTCONN
 * - The endpoint is not connected
 *- ENOTTY
 * - epd is not a valid endpoint descriptor
 *- ENXIO
 * - Addresses in the range [roffset,roffset+len-1] are invalid for the
 *   registered address space of epd.
 */
int scif_vwriteto(scif_epd_t epd, void *addr, size_t len, off_t offset,
int rma_flags);

/**
 * scif_fence_mark - Mark previously issued RMAs
 * 	\param epd		endpoint descriptor
 * 	\param flags		control flags
 * 	\param mark		marked handle returned as output.
 *
 * scif_fence_mark() returns after marking the current set of all uncompleted
 * RMAs initiated through the endpoint epd or the current set of all
 * uncompleted RMAs initiated through the peer of endpoint epd. The RMAs are
 * marked with a value returned at mark. The application may subsequently call
 * scif_fence_wait(), passing the value returned at mark, to await completion
 * of all RMAs so marked.
 *
 * The flags argument has exactly one of the following values:
 *- SCIF_FENCE_INIT_SELF: RMA operations initiated through endpoint
 *  epd are marked
 *- SCIF_FENCE_INIT_PEER: RMA operations initiated through the peer
 *  of endpoint epd are marked
 *
 * \return
 * Upon successful completion, scif_fence_mark() returns 0; otherwise: in user
 * mode -1 is returned and errno is set to indicate the error; in kernel mode
 * the negative of one of the following errors is returned.
 *
 *\par Errors:
 *- EBADF
 * - epd is not a valid endpoint descriptor
 *- ECONNRESET
 * - A connection was forcibly closed by a peer.
 *- EINVAL
 * - flags is invalid, or
 * - epd is not a valid endpoint descriptor, or
 *- ENODEV
 * - The remote node is lost.
 *- ENOTCONN
 * - The endpoint is not connected
 *- ENOMEM
 * - Insufficient kernel memory was available.
 *- ENOTTY
 * - epd is not a valid endpoint descriptor
 */
int scif_fence_mark(scif_epd_t epd, int flags, int *mark);

/**
 * scif_fence_wait - Wait for completion of marked RMAs
 *
 * 	\param epd		endpoint descriptor
 * 	\param mark		mark request
 *
 * scif_fence_wait() returns after all RMAs marked with mark have completed.
 * The value passed in mark must have been obtained in a previous call to
 * scif_fence_mark().
 *
 *\return
 * Upon successful completion, scif_fence_wait() returns 0; otherwise: in user
 * mode -1 is returned and errno is set to indicate the error; in kernel mode
 * the negative of one of the following errors is returned.
 *
 *\par Errors:
 *- EBADF
 * - epd is not a valid endpoint descriptor
 *- ECONNRESET
 * - A connection was forcibly closed by a peer.
 *- EINVAL
 * - epd is not a valid endpoint descriptor, or
 *- ENODEV
 * - The remote node is lost.
 *- ENOTCONN
 * - The endpoint is not connected
 *- ENOMEM
 * - Insufficient kernel memory was available.
 *- ENOTTY
 * - epd is not a valid endpoint descriptor
 */
int scif_fence_wait(scif_epd_t epd, int mark);

/**
 * scif_fence_signal - Request a signal on completion of RMAs
 * 	\param loff		local offset
 * 	\param lval		local value to write to loffset
 * 	\param roff		remote offset
 * 	\param rval		remote value to write to roffset
 * 	\param flags		flags
 *
 * scif_fence_signal() returns after marking the current set of all uncompleted
 * RMAs initiated through the endpoint epd or marking the current set of all
 * uncompleted RMAs initiated through the peer of endpoint epd.
 *
 * If flags includes SCIF_SIGNAL_LOCAL, then on completion of the RMAs in the
 * marked set, lval is written to memory at the address corresponding to offset
 * loff in the local registered address space of epd. loff must be within a
 * registered window. If flags includes SCIF_SIGNAL_REMOTE, then on completion
 * of the RMAs in the marked set, rval is written to memory at the  * address
 * corresponding to offset roff in the remote registered address space of epd.
 * roff must be within a remote registered window of the peer of epd. Note
 * that any specified offset must be DWORD (4 byte / 32 bit) aligned.
 *
 * The flags argument is formed by OR'ing together the following:
 *- Exactly one of the following values:
 * - SCIF_FENCE_INIT_SELF: RMA operations initiated through endpoint
 *   epd are marked
 * - SCIF_FENCE_INIT_PEER: RMA operations initiated through the peer
 *   of endpoint epd are marked
 *- One or more of the following values:
 * - SCIF_SIGNAL_LOCAL: On completion of the marked set of RMAs, write lval to
 *   memory at the address corresponding to offset loff in the local registered
 *   address space of epd.
 * - SCIF_SIGNAL_REMOTE: On completion of the marked set of RMAs, write lval to
 *   memory at the address corresponding to offset roff in the remote registered
 *   address space of epd.
 *
 *\return
 * Upon successful completion, scif_fence_signal() returns 0; otherwise: in
 * user mode -1 is returned and errno is set to indicate the error; in kernel
 * mode the negative of one of the following errors is returned.
 *\par Errors:
 *- EBADF
 * - epd is not a valid endpoint descriptor
 *- ECONNRESET
 * - A connection was forcibly closed by a peer.
 *- EINVAL
 * - epd is not a valid endpoint descriptor, or
 * - flags is invalid, or
 * - loff or roff are not DWORD aligned
 *- ENODEV
 * - The remote node is lost.
 *- ENOTCONN
 * - The endpoint is not connected
 *- ENOTTY
 * - epd is not a valid endpoint descriptor
 *- ENXIO
 * - loff is invalid for the registered address of epd, or
 * - roff is invalid for the registered address space, of the peer of epd
 */
int scif_fence_signal(scif_epd_t epd, off_t loff, uint64_t lval, off_t roff,
uint64_t rval, int flags);

/**
 * scif_get_nodeIDs - Return information about online nodes
 * 	\param nodes 		array in which to return online node IDs
 * 	\param len	 	number of entries in the nodes array
 * 	\param self 		address to place the node ID of the local node
 *
 * scif_get_nodeIDs() fills in the nodes array with up to len node IDs of the
 * nodes in the SCIF network. If there is not enough space in nodes, as
 * indicated by the len parameter, only len node IDs are returned in nodes. The
 * return value of scif_get_nodeID() is the total number of nodes currently in
 * the SCIF network. By checking the return value against the len parameter, the user may
 * determine if enough space for nodes was allocated.
 *
 * The node ID of the local node is returned at self.
 *
 *\return
 * Upon successful completion, scif_get_nodeIDs() returns the actual number of
 * online nodes in the SCIF network including 'self'; otherwise: in user mode
 * -1 is returned and errno is set to indicate the error; in kernel mode no
 * errors are returned.
 *
 *\par Errors:
 *- EFAULT
 * - Bad address
 */
int scif_get_nodeIDs(uint16_t *nodes, int len, uint16_t *self);

/**
 * scif_get_fd - Get file descriptor from endpoint descriptor
 *	\param epd		endpoint descriptor
 *
 * scif_get_fd() returns the file descriptor which backs a specified endpoint
 * descriptor, epd. The file descriptor returned should only be used as a
 * parameter to poll() or select().
 *
 *\return
 * scif_get_fd() returns the file descriptor.  There are no errors.
 */
static inline int scif_get_fd(scif_epd_t epd)
{
	return (int) epd;
}


/**
 * scif_poll - Wait for some event on an endpoint
 * 	\param epds		Array of endpoint descriptors
 * 	\param nepds		Length of epds
 * 	\param timeout		Upper limit on time for which scif_poll() will
 * 				block
 *
 * scif_poll() waits for one of a set of endpoints to become ready to perform
 * an I/O operation. scif_poll() exposes a subset of the functionality of the
 * POSIX standard poll() function.
 *
 * The epds argument specifies the endpoint descriptors to be examined and the
 * events of interest for each endpoint descriptor. epds is a pointer to an
 * array with one member for each open endpoint descriptor of interest.
 *
 * The number of items in the epds array is specified in nepds. The epd field
 * of scif_pollepd is an endpoint descriptor of an open endpoint. The field
 * events is a bitmask specifying the events which the application is
 * interested in. The field revents is an output parameter, filled by the
 * kernel with the events that actually occurred. The bits returned in revents
 * can include any of those specified in events, or one of the values
 * SCIF_POLLERR, SCIF_POLLHUP, or SCIF_POLLNVAL. (These three bits are
 * meaningless in the events field, and will be set in the revents field
 * whenever the corresponding condition is true.)
 *
 * If none of the events requested (and no error) has occurred for any of the
 * endpoint descriptors, then scif_poll() blocks until one of the events occurs.
 *
 * The timeout argument specifies an upper limit on the time for which
 * scif_poll() will block, in milliseconds. Specifying a negative value in
 * timeout means an infinite timeout.
 *
 * The following bits may be set in events and returned in revents:
 *- SCIF_POLLIN: Data may be received without blocking. For a connected
 *  endpoint, this means that scif_recv() may be called without blocking. For a
 *  listening endpoint, this means that scif_accept() may be called without
 *  blocking.
 *- SCIF_POLLOUT: Data may be sent without blocking. For a connected endpoint,
 *  this means that scif_send() may be called without blocking. This bit value
 *  has no meaning for a listening endpoint and is ignored if specified.
 *
 * The following bits are only returned in revents, and are ignored if set in
 * events:
 *- SCIF_POLLERR: An error occurred on the endpoint
 *- SCIF_POLLHUP: The connection to the peer endpoint was disconnected
 *- SCIF_POLLNVAL: The specified endpoint descriptor is invalid.
 *
 *\return
 * Upon successful completion, scif_poll()returns a non-negative value. A
 * positive value indicates the total number of endpoint descriptors that have
 * been selected (that is, endpoint descriptors for which the revents member is
 * non-zero. A value of 0 indicates that the call timed out and no endpoint
 * descriptors have been selected. Otherwise: in user mode -1 is returned and
 * errno is set to indicate the error; in kernel mode the negative of one of
 * the following errors is returned.
 *
 *\par Errors:
 *- EFAULT
 * - The array given as argument was not contained in the calling program's
 *   address space.
 *- EINTR
 * - A signal occurred before any requested event.
 *- EINVAL
 * - The nepds argument is greater than {OPEN_MAX}
 *- ENOMEM
 * - There was no space to allocate file descriptor tables.
*/
int
scif_poll(
	struct scif_pollepd *epds,
	unsigned int nepds,
	long timeout);

/**
 * scif_event_register - Register an event handler
 *	\param handler		Event handler to be registered
 *
 * scif_event_register() registers a routine, handler, to be called when some
 * event occurs. The event parameter to handler indicates the type of event
 * which has occurred, and the corresponding component of the data parameter to
 * handler provides additional data about the event.
 *
 * The following events are defined:
 *- SCIF_NODE_ADDED: A node has been added to the SCIF network. The
 *  scif_node_added component of the data parameter to handler identifies the
 *  node. This event is informational. There are no requirements on the event
 *  handler.
 *- SCIF_NODE_REMOVED: A node is being removed from the SCIF network. The
 *  scif_node_removed component of the data parameter to handler identifies the
 *  node. Upon being called, and before returning, the event handler must
 *  return, using scif_put_pages(), all structures obtained using
 *  scif_get_pages() against an endpoint connected to the lost node. It is
 *  recommended and expected that the handler will also scif_close() all
 *  endpoints connected to the lost node.
 *
 *\return
 * Upon successful completion scif_event_register() returns 0.
 *
 *\par Errors:
 *- ENOMEM
 * - There was no space to allocate file descriptor tables.
*/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __SCIF_H__ */
