// -*- c++ -*-

#ifndef CPI_CFUTIL_LEGACY_ERROR_NUMBERS_H__
#define CPI_CFUTIL_LEGACY_ERROR_NUMBERS_H__

/**
 * \file
 * \brief Maps SCA 2.2.2 error numbers to SCA 2.2 error numbers.
 *
 * When this file is included, the code can use SCA 2.2.2 error numbers
 * such as CF::CF_E2BIG.  When we are building for SCA 2.2, these values
 * are then mapped to their SCA 2.2 equivalent.
 *
 * Revision History:
 *
 *     04/16/2009 - Frank Pilhofer
 *                  Initial version.
 *
 */

#if defined (CPI_USES_SCA22)

#define CF_NOTSET CFNOTSET
#define CF_E2BIG SCA_E2BIG
#define CF_EACCES SCA_EACCES
#define CF_EAGAIN SCA_EAGAIN
#define CF_EBADF SCA_EBADF
#define CF_EBADMSG SCA_EBADMSG
#define CF_EBUSY SCA_EBUSY
#define CF_ECANCELED SCA_ECANCELED
#define CF_ECHILD SCA_ECHILD
#define CF_EDEADLK SCA_EDEADLK
#define CF_EDOM SCA_EDOM
#define CF_EEXIST SCA_EEXIST
#define CF_EFAULT SCA_EFAULT
#define CF_EFBIG SCA_EFBIG
#define CF_EINPROGRESS SCA_EINPROGRESS
#define CF_EINTR SCA_EINTR
#define CF_EINVAL SCA_EINVAL
#define CF_EIO SCA_EIO
#define CF_EISDIR SCA_EISDIR
#define CF_EMFILE SCA_EMFILE
#define CF_EMLINK SCA_EMLINK
#define CF_EMSGSIZE SCA_EMSGSIZE
#define CF_ENAMETOOLONG SCA_ENAMETOOLONG
#define CF_ENFILE SCA_ENFILE
#define CF_ENODEV SCA_ENODEV
#define CF_ENOENT SCA_ENOENT
#define CF_ENOEXEC SCA_ENOEXEC
#define CF_ENOLCK SCA_ENOLCK
#define CF_ENOMEM SCA_ENOMEM
#define CF_ENOSPC SCA_ENOSPC
#define CF_ENOSYS SCA_ENOSYS
#define CF_ENOTDIR SCA_ENOTDIR
#define CF_ENOTEMPTY SCA_ENOTEMPTY
#define CF_ENOTSUP SCA_ENOTSUP
#define CF_ENOTTY SCA_ENOTTY
#define CF_ENXIO SCA_ENXIO
#define CF_EPERM SCA_EPERM
#define CF_EPIPE SCA_EPIPE
#define CF_ERANGE SCA_ERANGE
#define CF_EROFS SCA_EROFS
#define CF_ESPIPE SCA_ESPIPE
#define CF_ESRCH SCA_ESRCH
#define CF_ETIMEDOUT SCA_ETIMEDOUT
#define CF_EXDEV SCA_EXDEV

#endif
#endif
