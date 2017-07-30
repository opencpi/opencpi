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

// -*- c++ -*-

#ifndef OCPIOSDOXYGEN_H__
#define OCPIOSDOXYGEN_H__

/**
 * \file
 * \brief Documentation only.
 *
 * This file does not contain any functional content, but only documentation
 * intended for consumption by doxygen.  It shall not be included by any user
 * code.
 *
 * Revision History:
 *
 *     02/25/2008 - Frank Pilhofer
 *                  Initial version.
 *
 *     10/02/2008 - Frank Pilhofer
 *                  Added ezxml.
 *
 */

/**
 * \mainpage The OCPI::Util Library
 *
 * This library provides a number of useful facilities.
 *
 * \section util-vfs Virtual File System
 *
 * OCPI::Util::Vfs namespace &mdash; VFS overview and various utilities.\n
 * OCPI::Util::Vfs::Vfs &mdash; The abstract base class for all virtual
 * file systems.\n
 *
 * \subsection util-vfs-impl Virtual File System implementations
 *
 * OCPI::Util::FileFs::FileFs &mdash; File system based virtual file system.\n
 * OCPI::Util::ZipFs::ZipFs &mdash; ZIP-file based virtual file system.\n
 * OCPI::Util::Http::HttpFs &mdash; HTTP client based virtual file system.\n
 * OCPI::Util::MemFs::StaticMemFs &mdash; In-memory read-only file system.\n
 *
 * \subsection util-vfs-delegating Delegating file systems
 *
 * OCPI::Util::Vfs::FilterFs &mdash; Filter file system access.\n
 * OCPI::Util::Vfs::ReadOnlyFs &mdash; Read-only file system access.\n
 * OCPI::Util::Vfs::UriFs &mdash; URI-based file system.\n
 *
 * \section util-internet Internet Access
 *
 * OCPI::Util::Tcp::Stream &mdash; Data streaming over TCP/IP sockets.\n
 * OCPI::Util::Tcp::Client &mdash; TCP/IP client socket.\n
 * OCPI::Util::Tcp::Server &mdash; TCP/IP server socket.\n
 * OCPI::Util::Tcp::Connector &mdash; Connector pattern for HTTP connections.\n
 * OCPI::Util::Http::Client &mdash; HTTP client.\n
 * OCPI::Util::Http::Server &mdash; HTTP server.\n
 *
 * \section util-corba CORBA
 *
 * OCPI::Util::IOP namespace &mdash; Classes and types related to IORs.\n
 * OCPI::Util::IIOP namespace &mdash; Classes and types related to IIOP IORs.\n
 * OCPI::Util::IOP::IOR &mdash; Interoperable Object Reference.\n
 * OCPI::Util::CDR::Encoder &mdash; Minimal CDR Encoder.\n
 * OCPI::Util::CDR::Decoder &mdash; Minimal CDR Decoder.\n
 *
 * \section util-mt Multithreading
 *
 * OCPI::Util::AutoMutex &mdash; Auto-release a OCPI::OS::Mutex.\n
 * OCPI::Util::AutoRDLock &mdash; Auto-release a OCPI::OS::RWLock read lock.\n
 * OCPI::Util::AutoWRLock &mdash; Auto-release a OCPI::OS::RWLock write lock.\n
 * OCPI::Util::WorkerThread &mdash; Worker thread for asynchronous job
 * processing.\n
 * OCPI::Util::Thread &mdash; Thread class for asynchronous tasks.
 *
 * \section util-misc Miscellaneous
 *
 * OCPI::Util::EzXml::Doc &mdash; Wrapper for parsing XML files using ezxml.\n
 * OCPI::Util::Uri &mdash; RFC 2396 Universal Resource Identifiers.\n
 * OCPI::Util::CommandLineConfiguration &mdash; Parse command-line options.\n
 * OCPI::Util::Misc namespace &mdash; Useful functions.\n
 *
 * \section util-credits Credits
 *
 * The OCPI::Util::ZipFs::ZipFs class uses the zlib library from
 * http://www.gzip.org/zlib/ and the minizip library from
 * http://www.winimage.com/zLibDll/. 
 *
 * zlib was written by Jean-loup Gailly (compression) and Mark Adler
 * (decompression).  The license is provided in the file
 * util/zip/minizip/README.zlib.  It reads:
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute
 * it freely, subject to the following restrictions:
 * - The origin of this software must not be misrepresented; you must not
 *   claim that you wrote the original software.  If you use this software
 *   in a product, an acknowledgment in the product documentation would
 *   be appreciated but is not required.
 * - Altered source versions must be plainly marked as such, and must
 *   not be misrepresented as being the original software.
 * - This notice may not be removed or altered from any source distribution.
 *
 * The minizip library is copyright &copy; 1999-2003 Giles Vollant.
 * Its license terms, provided at the top of the source code files
 * util/zip/minizip/zip.h and util/zip/minizip/unzip.h, are the same
 * as for the zlib library.
 *
 * The OCPI::Util::Vfs::md5 functions uses the MD5 implementation by
 * L. Peter Deutsch.  It is copyright &copy; 1999, 2002 Aladdin
 * Enterprises.  Its license terms, provided at the top of the
 * source code files util/vfs/md5.h, are the same as for the zlib
 * library.
 *
 * The ezxml library is copyright &copy; 2004-2006 Aaron Voisine, see
 * http://ezxml.sourceforge.net/.  Its license terms are similar in
 * spirit to the other licenses above.  A full copy of the license can
 * be found in the file util/ezxml/license.txt.
 */

namespace OCPI {
  /**
   * \brief A number of useful facilities.
   */

  namespace Util {
  }
}

#endif
