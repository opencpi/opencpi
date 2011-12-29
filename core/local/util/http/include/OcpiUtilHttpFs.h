
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

#ifndef OCPIUTILHTTPFS_H__
#define OCPIUTILHTTPFS_H__

/**
 * \file
 * \brief Vfs implementation using a HTTP client.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <OcpiUtilVfs.h>
#include <OcpiUtilHttpClient.h>
#include <OcpiOsMutex.h>
#include <iostream>
#include <string>
#include <ctime>

namespace OCPI {
  namespace Util {
    namespace Http {

      /**
       * \brief Vfs implementation using a HTTP client.
       *
       * This class implements the OCPI::Util::Vfs::Vfs file system
       * interface, delegating file access to a remote server that
       * implements the HTTP protocol.
       *
       * Opening a file for reading will perform a GET, opening a file for
       * writing will perform a PUT. Querying file attributes is done via
       * HEAD.
       *
       * This file system pretends to have a directory structure in which
       * the authority (<em>hostname[:port</em>]) is the first path component
       * in an absolute path.  That makes it possible to, e.g., change to
       * directory <tt>/www.mc.com</tt> and then to open the file
       * <tt>index.html</tt>.  Note that a relative file name may be empty,
       * i.e., open("") is valid, referring to the server-defined "index"
       * file.
       *
       * HTTP is supported over various transports.
       * OCPI::Util::Http::HttpFsBase is a transport-agnostic abstract
       * base class.  A derived class must implement the makeConnection()
       * operation.
       *
       * This OCPI::Util::Http::HttpFsBase base class exists primarily so
       * that the OCPI::Util::Http::HttpFs template class can be as small
       * as possible, so that as little code as necessary needs to be
       * generated for each template instantiation.
       *
       * See OCPI::Util:Http::HttpFs for an implementation of this class.
       *
       * \note All operations expect file names, not URLs.  To access a
       * URL, it must be converted to a file name first, using the
       * nameToURI() operation.
       *
       * \note Files can not be opened for reading and writing.  The
       * open() operation always fails.
       *
       * \note File streams are not seekable.
       *
       * \note Directory contents can not be listed, i.e., the list()
       * always fails.  (Some HTTP servers render a human-readable,
       * HTML-formatted "index" page containing directory listings, but
       * since there is no well-defined format, it can not be parsed.)
       */

      class HttpFsBase : public OCPI::Util::Vfs::Vfs {
      public:
        /**
         * Constructor.
         *
         * \param[in] scheme The URI scheme supported by this instance,
         *                   e.g., "http" or "https".  This is used in
         *                   composing the file system's base URI and for
         *                   mapping between URIs and file names.
         * \param[in] root   A fixed absolute path name prefix for all
         *                   filenames.  E.g., if \a root is set to
         *                   "/www.mc.com", only locations within this
         *                   domain can be accessed.
         */

        HttpFsBase (const std::string & scheme, const std::string & root)
          throw (std::string);

        /**
         * Destructor.
         */

        ~HttpFsBase ()
          throw ();

        /**
         * \name Implementation of the OCPI::Util::Vfs::Vfs interface.
         */

        //@{

        /*
         * File Name URI Mapping
         */

        std::string baseURI () const
          throw ();

        std::string nameToURI (const std::string &) const
          throw (std::string);

        std::string URIToName (const std::string &) const
          throw (std::string);

        /*
         * Directory Management
         */

        std::string cwd () const
          throw (std::string);

        void cd (const std::string &)
          throw (std::string);

        void mkdir (const std::string &)
          throw (std::string);

        void rmdir (const std::string &)
          throw (std::string);

        /*
         * Directory Listing
         */

#if 0
        OCPI::Util::Vfs::Iterator * list (const std::string & dir,
                                         const std::string & pattern = "*")
          throw (std::string);

        void closeIterator (OCPI::Util::Vfs::Iterator *)
          throw (std::string);
#endif
	OCPI::Util::Vfs::Dir &openDir(const std::string&) throw(std::string);

        /*
         * File Information
         */

        bool exists (const std::string &, bool * = 0)
          throw (std::string);

        unsigned long long size (const std::string &)
          throw (std::string);

        std::time_t lastModified (const std::string &)
          throw (std::string);

        /*
         * File I/O
         */

        std::iostream * open (const std::string &, std::ios_base::openmode = std::ios_base::in | std::ios_base::out)
          throw (std::string);

        std::istream * openReadonly (const std::string &, std::ios_base::openmode = std::ios_base::in)
          throw (std::string);

        std::ostream * openWriteonly (const std::string &, std::ios_base::openmode = std::ios_base::out | std::ios_base::trunc)
          throw (std::string);

        void close (std::ios *)
          throw (std::string);

        /*
         * File System Operations
         */

        void remove (const std::string &)
          throw (std::string);

        //@}

      protected:
        /**
         * Make a transport-specific HTTP connection.
         */

        virtual OCPI::Util::Http::ClientStream * makeConnection ()
          throw (std::string) = 0;

        /** \cond */

      protected:
        OCPI::Util::Http::ClientStream * hgpr (const std::string &, bool, bool, bool, bool)
          throw (std::string);

        void testFilenameForValidity (const std::string &) const
          throw (std::string);

        std::string nativeFilename (const std::string &) const
          throw (std::string);

        std::string absoluteNameLocked (const std::string &) const
          throw (std::string);

      protected:
        std::string m_scheme;
        std::string m_baseURI;
        std::string m_root;
        std::string m_cwd;
        mutable OCPI::OS::Mutex m_lock;

        /** \endcond */

      private:
        /**
         * Not implemented.
         */

        HttpFsBase (const HttpFsBase &);

        /**
         * Not implemented.
         */

        HttpFsBase & operator= (const HttpFsBase &);
      };

      /**
       * \brief Vfs implementation using a HTTP client.
       *
       * This class implements the OCPI::Util::Vfs::Vfs file system
       * interface, delegating file access to a remote HTTP server.
       *
       * It specializes the abstract OCPI::Util::Http::HttpFsBase
       * base class, delegating connections to a transport provider
       * ("Connector") template parameter.  See
       * OCPI::Util::Tcp::Connector for a suitable TCP-based transport
       * provider class.
       *
       * See the OCPI::Util::Http::HttpFsBase documentation for more
       * information about the HTTP file system.
       *
       * This code fragment illustrates the use of the HttpFs class
       * using the TCP connector.
       *
       * \code
       *   OCPI::Util::Http::HttpFs<OCPI::Util::Tcp::Connector> mc;
       *   mc.cd ("/www.mc.com");
       *   std::istream * home = mc.openReadonly ("");
       *   // read the mc.com homepage from the "home" stream
       *   mc.close (home);
       * \endcode
       */

      template<class Connector>
      class HttpFs : public HttpFsBase {
      public:
        /**
         * Constructor.
         *
         * This constructor uses an empty "root", i.e., the first path
         * component of absolute file names is interpreted as the URL
         * authority.
         *
         * Calls OCPI::Util::Http::HttpFsBase ("http", "").
         */

        inline HttpFs ()
          throw (std::string);

        /**
         * Constructor.
         *
         * \param[in] root   A fixed absolute path name prefix for all
         *                   filenames.  E.g., if \a root is set to
         *                   "/www.mc.com", only locations within this
         *                   domain can be accessed.
         *
         * Calls OCPI::Util::Http::HttpFsBase ("http", \a root).
         */

        inline HttpFs (const std::string & root)
          throw (std::string);

        /**
         * Destructor.
         */

        inline ~HttpFs ()
          throw ();

      protected:
        /**
         * Creates an HTTP client.
         */

        inline OCPI::Util::Http::ClientStream * makeConnection ()
          throw (std::string);

      private:
        /**
         * Not implemented.
         */

        HttpFs (const HttpFs &);

        /**
         * Not implemented.
         */

        HttpFs & operator= (const HttpFs &);
      };

    }
  }
}

/** \cond */

/*
 * ----------------------------------------------------------------------
 * OCPI::Util::Http::HttpFs<Connector>
 * ----------------------------------------------------------------------
 */

template<class Connector>
inline
OCPI::Util::Http::HttpFs<Connector>::HttpFs ()
  throw (std::string)
  : HttpFsBase (Connector::g_scheme, "")
{
}

template<class Connector>
inline
OCPI::Util::Http::HttpFs<Connector>::HttpFs (const std::string & root)
  throw (std::string)
  : HttpFsBase (Connector::g_scheme, root)
{
}

template<class Connector>
inline
OCPI::Util::Http::HttpFs<Connector>::~HttpFs ()
  throw ()
{
}

template<class Connector>
inline
OCPI::Util::Http::ClientStream *
OCPI::Util::Http::HttpFs<Connector>::makeConnection ()
  throw (std::string)
{
  return new OCPI::Util::Http::Client<Connector> ();
}

/** \endcond */

#endif
