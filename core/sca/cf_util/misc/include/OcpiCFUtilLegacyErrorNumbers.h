
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
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

// -*- c++ -*-

#ifndef OCPI_CFUTIL_LEGACY_ERROR_NUMBERS_H__
#define OCPI_CFUTIL_LEGACY_ERROR_NUMBERS_H__

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

#if defined (OCPI_USES_SCA22)

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
