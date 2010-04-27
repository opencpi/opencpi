// -*- c++ -*-

#ifndef CPIUTILHTTPFS_H__
#define CPIUTILHTTPFS_H__

/**
 * \file
 * \brief Vfs implementation using a HTTP client.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <CpiUtilVfs.h>
#include <CpiUtilHttpClient.h>
#include <CpiOsMutex.h>
#include <iostream>
#include <string>
#include <ctime>

namespace CPI {
  namespace Util {
    namespace Http {

      /**
       * \brief Vfs implementation using a HTTP client.
       *
       * This class implements the CPI::Util::Vfs::Vfs file system
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
       * CPI::Util::Http::HttpFsBase is a transport-agnostic abstract
       * base class.  A derived class must implement the makeConnection()
       * operation.
       *
       * This CPI::Util::Http::HttpFsBase base class exists primarily so
       * that the CPI::Util::Http::HttpFs template class can be as small
       * as possible, so that as little code as necessary needs to be
       * generated for each template instantiation.
       *
       * See CPI::Util:Http::HttpFs for an implementation of this class.
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

      class HttpFsBase : public CPI::Util::Vfs::Vfs {
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
	 * \name Implementation of the CPI::Util::Vfs::Vfs interface.
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

	CPI::Util::Vfs::Iterator * list (const std::string & dir,
					 const std::string & pattern = "*")
	  throw (std::string);

	void closeIterator (CPI::Util::Vfs::Iterator *)
	  throw (std::string);

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

	virtual CPI::Util::Http::ClientStream * makeConnection ()
	  throw (std::string) = 0;

	/** \cond */

      protected:
	CPI::Util::Http::ClientStream * hgpr (const std::string &, bool, bool, bool, bool)
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
	mutable CPI::OS::Mutex m_lock;

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
       * This class implements the CPI::Util::Vfs::Vfs file system
       * interface, delegating file access to a remote HTTP server.
       *
       * It specializes the abstract CPI::Util::Http::HttpFsBase
       * base class, delegating connections to a transport provider
       * ("Connector") template parameter.  See
       * CPI::Util::Tcp::Connector for a suitable TCP-based transport
       * provider class.
       *
       * See the CPI::Util::Http::HttpFsBase documentation for more
       * information about the HTTP file system.
       *
       * This code fragment illustrates the use of the HttpFs class
       * using the TCP connector.
       *
       * \code
       *   CPI::Util::Http::HttpFs<CPI::Util::Tcp::Connector> mc;
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
	 * Calls CPI::Util::Http::HttpFsBase ("http", "").
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
	 * Calls CPI::Util::Http::HttpFsBase ("http", \a root).
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

	inline CPI::Util::Http::ClientStream * makeConnection ()
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
 * CPI::Util::Http::HttpFs<Connector>
 * ----------------------------------------------------------------------
 */

template<class Connector>
inline
CPI::Util::Http::HttpFs<Connector>::HttpFs ()
  throw (std::string)
  : HttpFsBase (Connector::g_scheme, "")
{
}

template<class Connector>
inline
CPI::Util::Http::HttpFs<Connector>::HttpFs (const std::string & root)
  throw (std::string)
  : HttpFsBase (Connector::g_scheme, root)
{
}

template<class Connector>
inline
CPI::Util::Http::HttpFs<Connector>::~HttpFs ()
  throw ()
{
}

template<class Connector>
inline
CPI::Util::Http::ClientStream *
CPI::Util::Http::HttpFs<Connector>::makeConnection ()
  throw (std::string)
{
  return new CPI::Util::Http::Client<Connector> ();
}

/** \endcond */

#endif
