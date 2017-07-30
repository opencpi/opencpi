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

#ifndef OCPIUTILHTTPCLIENT_H__
#define OCPIUTILHTTPCLIENT_H__

/**
 * \file
 * \brief Transport-agnostic HTTP client.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <OcpiUtilHttpMisc.h>
#include <OcpiUtilUri.h>
#include <OcpiUtilMisc.h>
#include <streambuf>
#include <iostream>
#include <string>
#include <map>
#include <ctime>

namespace OCPI {
  namespace Util {
    namespace Http {

      /**
       * \brief Exception thrown in case of a redirection.
       *
       * The OCPI::Util::Http::ClientStream class throws this class as
       * an exception when the server replies with a 3xx (redirection)
       * status code.
       */

      struct Redirection {
        /**
         * Constructor.
         *
         * \param[in] code The status code from the HTTP response status line.
         * \param[in] location The new location.
         */

        Redirection (int code, const std::string & location);

        /**
         * String conversion operator.  Returns newLocation.
         */

        operator const std::string & () const;

        /**
         * The status code from the HTTP response status line.
         */

        int statusCode;

        /**
         * The new location of the object, from the HTTP "Location:"
         * header.
         */

        std::string newLocation;
      };

      /**
       * \brief Base class for HTTP client exceptions.
       *
       * The OCPI::Util::Http::ClientStream class throws an exception of this
       * type when the server replies with a 4xx (client error) or 5xx
       * (server error) status code.  The exception is further specialized
       * in either case, so that they can be caught and handled separately.
       */

      struct HttpError {
        /**
         * Constructor.
         *
         * \param[in] code The status code from the HTTP response status line.
         * \param[in] reason The reason phrase from the HTTP response status
         *                   line.
         * \param[in] body The entity body from the HTTP response.
         *
         * \post The \a body is not copied and must continue to exist
         * for the life span of this object.
         */

        HttpError (int code, const std::string & reason, const std::string & body);

        /**
         * The status code from the HTTP response status line.
         */

        int statusCode;

        /**
         * The reason phrase from the HTTP response status line.
         */

        std::string reasonPhrase;

        /**
         * The entity body from the HTTP response.
         */

        const std::string & body;
      };

      /**
       * \brief Exception thrown by an HTTP client in case of a client error.
       *
       * This exception is thrown in case of a 4xx client error, such
       * as a 404 "file not found".
       */

      struct ClientError : HttpError {
        /**
         * Constructor.
         *
         * \param[in] code The status code from the HTTP response status line.
         * \param[in] reason The reason phrase from the HTTP response status
         *                   line.
         * \param[in] body The entity body from the HTTP response.
         *
         * \post The \a body is not copied and must continue to exist
         * for the life span of this object.
         */

        ClientError (int code, const std::string & reason, const std::string & body);
      };

      /**
       * \brief Exception thrown by an HTTP client in case of a server error.
       *
       * This exception is thrown in case of a 5xx server error.
       */

      struct ServerError : HttpError {
        /**
         * Constructor.
         *
         * \param[in] code The status code from the HTTP response status line.
         * \param[in] reason The reason phrase from the HTTP response status
         *                   line.
         * \param[in] body The entity body from the HTTP response.
         *
         * \post The \a body is not copied and must continue to exist
         * for the life span of this object.
         */

        ServerError (int code, const std::string & reason, const std::string & body);
      };

      /**
       * \brief Transport-agnostic HTTP client.
       *
       * This class is derived from std::iostream. After initiating an
       * HTTP request, e.g., using the get() or post() operations, the
       * std::iostream interface can be used to read or write the
       * request body.
       *
       * The HTTP protocol is supported over various transports.
       * OCPI::Util::Http::ClientStream is a transport-agnostic abstract
       * base class.  A derived class must implement the openConnection(),
       * shutdownConnection() and closeConnection() operations.
       *
       * The OCPI::Util::Http::ClientStream base class exists primarily so
       * that the OCPI::Util::Http::Client template class can be as small
       * as possible, so that as little code as necessary needs to be
       * generated for each template instantiation.
       *
       * See OCPI::Util::Http::Client for an implementation of this class.
       */

      class ClientStream : public std::iostream {
      public:
        /**
         * Defaults. Should probably be configurable.
         */

        enum {
          MAX_NUM_OF_HEADER_LINES = 256,
          MAX_LENGTH_OF_HEADER_LINE = 4096,
          MAX_LENGTH_OF_BODY = 65536
        };

        /**
         * The string to use for the User-Agent request-header field.
         */

        static const std::string userAgent;

        /**
         * The default content type to assume if the server reply does
         * not include a <em>Content-Type</em> header.
         */

        static const std::string defaultContentType; // application/octet-stream

        /**
         * The default charset to assume if the content is of type
         * <tt>text</tt> but the server reply does not include a
         * <em>Content-Type</em> header or if that header does not
         * contain a <em>charset</em> attribute.
         */

        static const std::string defaultCharset;     // iso-8859-1

      public:

        /** \cond */

        /*
         * Private helper class implementing the std::streambuf interface.
         */

        class ClientBuf : public std::streambuf {
        protected:
          enum { // State
            STATE_NOT_CONNECTED,
            STATE_SEND_REQUEST,
            STATE_RECEIVE_HEADERS,
            STATE_DOWNLOAD,
            STATE_UPLOAD,
            STATE_CLOSE
          };

        public:
          enum {
            REQUEST_GET,
            REQUEST_HEAD,
            REQUEST_POST,
            REQUEST_PUT,
            REQUEST_DELETE
          };

        public:
          ClientBuf ()
            throw ();

          ~ClientBuf ()
            throw ();

          void setConn (std::istream *, std::ostream *,
                        void (*) (void *) = 0, void * = 0)
            throw ();

          void resetConn ()
            throw ();

          /*
           * Send a request
           */

          void sendRequest (int requestType,
                            const OCPI::Util::Uri & uri,
                            const Headers & headers = Headers())
            throw (std::string);

          void completeUpload ()
            throw (std::string);

          int getStatusCode ()
            throw (std::string);

          std::string getReasonPhrase ()
            throw (std::string);

          const Headers & getResponseHeaders ()
            throw (std::string);

          unsigned long long getContentLength ()
            throw (std::string);

        private:
          void startRequest (int requestType)
            throw (std::string);

          void sendAnyRequest (const OCPI::Util::Uri & uri,
                               const std::string & request,
                               const Headers & requestHeaders)
            throw (std::string);

          void sendGetOrHeadOrDeleteRequest (int requestType,
                                             const OCPI::Util::Uri & uri,
                                             const Headers & headers = Headers())
            throw (std::string);

          void sendPutOrPostRequest (int requestType,
                                     const OCPI::Util::Uri & uri,
                                     const Headers & headers = Headers())
            throw (std::string);

          void receiveStatus ()
            throw (std::string);

          void receiveHeaders ()
            throw (std::string);

        protected:
          /*
           * Implementation of the streambuf interface
           */

          int sync ();
          std::streamsize showmanyc ();
          std::streamsize xsgetn (char *, std::streamsize);
          int_type underflow_common (bool);
          int_type underflow ();
          int_type uflow ();
          int_type pbackfail (int_type = std::streambuf::traits_type::eof());

          std::streamsize xsputn (const char *, std::streamsize);
          int_type overflow (int_type = std::streambuf::traits_type::eof());

          /*
           * Data
           */

        protected:
          int m_state;
          std::istream * m_inConn;       /* HTTP connection for reading */
          std::ostream * m_outConn;      /* HTTP connection for writing */
          void (*m_shutdown) (void *);   /* To shutdown the sending end */
          void * m_sdopaque;             /* Shutdown cookie */
          bool m_closeConnection;        /* Whether conn is persistent */

          /*
           * Request
           */

          int m_requestType;

          /*
           * Response header
           */

          std::string m_status;          /* HTTP Status-Line */
          int m_statusCode;              /* HTTP Status-Code */
          std::string m_reason;          /* HTTP Reason-Phrase */
          int m_majorVersion, m_minorVersion; /* HTTP-Version */
          Headers m_responseHeaders;     /* Response Headers */

          /*
           * For download
           */

          bool m_chunked;                /* Is chunking being used? */
          bool m_firstChunk;             /* About to read the first chunk? */
          unsigned long long m_chunkRemaining;   /* Remaining chunk length */
          unsigned long long m_contentLength;    /* Content length, or -1 */
          unsigned long long m_contentRemaining; /* Remaining content */

          /*
           * For upload
           */

          unsigned long long m_uploadLength;    /* Amount of data to upload */
          unsigned long long m_uploadRemaining; /* Remaining amount to upload */

          /*
           * For streambuf
           */

          bool m_haveLast;
          bool m_havePending;
          int_type m_pending;

        private:
          ClientBuf (const ClientBuf &);
          ClientBuf & operator= (const ClientBuf &);
        };

        /** \endcond */

        /*
         * Continue with the ClientStream definition.
         */

      public:
        /**
         * Constructor.
         *
         * \post The client stream is unconnected.
         */

        ClientStream ()
          throw ();

        /**
         * Destructor.
         */

        virtual ~ClientStream ()
          throw ();

        /**
         * \name Initiating HTTP requests.
         *
         * One of these functions must be called to initiate an HTTP
         * request, to prepare this object for communication, before data
         * can be read or sent.
         *
         * The head(), get(), put(), post() and remove() operations initiate
         * <tt>HEAD</tt>, <tt>GET</tt>, <tt>PUT</tt>, <tt>POST</tt> or
         * <tt>DELETE</tt> requests, respectively.
         *
         * Each of the operations may raise any of the following exceptions:
         *
         * An exception of type OCPI::Util::Http::Redirection is raised if
         * the server replies with a 3xx status code.
         *
         * An exception of type OCPI::Util::Http::ClientError is raised if
         * the server replies with a 4xx status code.
         *
         * An exception of type OCPI::Util::Http::ServerError is raised if
         * the server replies with a 5xx status code.
         */

        //@{

        /**
         * Initiate a <tt>HEAD</tt> request.
         *
         * Makes reply infromation available.  Attempting to read from
         * the stream signals end of file.  Writing to the stream fails.
         *
         * \param[in] uri The URI location to query for information.
         *
         * \throw Redirection Server replied with a 3xx response.
         * \throw ClientError Server replied with a 4xx response.
         * \throw ServerError Server replied with a 5xx response.
         * \throw std::string Other error, such as a failure to connect to
         * the server.
         *
         * \pre The client stream is unconnected.
         * \post The client stream is connected.
         * \post Reply information is available.
         */

        void head (const OCPI::Util::Uri & uri)
          throw (std::string, Redirection, ClientError, ServerError);

        /**
         * Initiate a <tt>GET</tt> request.
         *
         * Makes reply infromation available.  The body of the reply can
         * be read from the stream.  Writing to the stream fails.
         *
         * \param[in] uri The URI location to access.
         *
         * \throw Redirection Server replied with a 3xx response.
         * \throw ClientError Server replied with a 4xx response.
         * \throw ServerError Server replied with a 5xx response.
         * \throw std::string Other error, such as a failure to connect to
         * the server.
         *
         * \pre The client stream is unconnected.
         * \post The client stream is connected.
         * \post Reply information is available.
         */

        void get (const OCPI::Util::Uri & uri)
          throw (std::string, Redirection, ClientError, ServerError);

        /**
         * Initiate a <tt>PUT</tt> request.
         *
         * After this operation succeeds, data can be written to the
         * stream until the upload is completed.
         *
         * The upload can be completed by reading from the stream (if
         * \a mode & std::ios_base::in), accessing any reply information,
         * or by explicitly calling completeUpload().
         *
         * Reply information is available only after the upload is
         * completed.
         *
         * Once the upload is completed, writing to the stream fails.
         *
         * Equivalent to put (uri, std::string(), -1, mode).
         *
         * \param[in] uri  The URI to upload to.
         * \param[in] mode If the std::ios_base::out bit is set, allows to
         *             write the data that is to be uploaded into the
         *             stream.  If the std::ios_base::in bit is set,
         *             the reply body can be read from the stream after
         *             completing the upload.  Otherwise, the reply body
         *             will be read internally upon completing the upload,
         *             and is made available via the body() operation.
         *
         * \throw Redirection Server replied with a 3xx response.
         * \throw ClientError Server replied with a 4xx response.
         * \throw ServerError Server replied with a 5xx response.
         * \throw std::string Other error, such as a failure to connect to
         * the server.
         *
         * \pre The client stream is unconnected.
         * \post The client stream is connected.
         */

        void put (const OCPI::Util::Uri & uri, std::ios_base::openmode mode = std::ios_base::out)
          throw (std::string, Redirection, ClientError, ServerError);

        /**
         * Initiate a <tt>PUT</tt> request.
         *
         * After this operation succeeds, data can be written to the
         * stream until the upload is completed.
         *
         * The upload can be completed by reading from the stream (if
         * \a mode & std::ios_base::in), accessing any reply information,
         * or by explicitly calling completeUpload().
         *
         * Reply information is available only after the upload is
         * completed.
         *
         * Once the upload is completed, writing to the stream fails.
         *
         * \param[in] uri   The URI to upload to.
         * \param[in] contentType If not empty, this value is sent as the
         *             parameter of the <em>Content-Type</em> header field.
         * \param[in] size  If not -1, this value is sent as the parameter
         *             of the <em>Content-Length</em> header field.  Behavior
         *             is undefined unless exactly this number of octets are
         *             written to the stream.
         * \param[in] mode  If the std::ios_base::out bit is set, allows to
         *             write the data that is to be uploaded into the
         *             stream.  If the std::ios_base::in bit is set,
         *             the reply body can be read from the stream after
         *             completing the upload.  Otherwise, the reply body
         *             will be read internally upon completing the upload,
         *             and is made available via the body() operation.
         *
         * \throw Redirection Server replied with a 3xx response.
         * \throw ClientError Server replied with a 4xx response.
         * \throw ServerError Server replied with a 5xx response.
         * \throw std::string Other error, such as a failure to connect to
         * the server.
         *
         * \pre The client stream is unconnected.
         * \post The client stream is connected.
         */

        void put (const OCPI::Util::Uri & uri, const std::string & contentType, unsigned long long size, std::ios_base::openmode mode)
          throw (std::string, Redirection, ClientError, ServerError);

        /*
         * Initiate a <tt>POST</tt> request.
         *
         * After this operation succeeds, data can be written to the
         * stream until the upload is completed.
         *
         * The upload can be completed by reading from the stream (if
         * \a mode & std::ios_base::in), accessing any reply information,
         * or by explicitly calling completeUpload().
         *
         * Reply information is available only after the upload is
         * completed.
         *
         * Once the upload is completed, writing to the stream fails.
         *
         * Equivalent to post (uri, std::string(), -1, mode).
         *
         * \param[in] uri   The URI to post to.
         * \param[in] mode  If the std::ios_base::out bit is set, allows to
         *             write the data that is to be posted into the
         *             stream. If the std::ios_base::in bit is set,
         *             the reply body can be read from the stream after
         *             completing the upload. Otherwise, the reply body
         *             will be read internally upon completing the upload,
         *             and is made available via the body() operation.
         *
         * \param[in] uri  The URI to upload to.
         * \param[in] mode If the std::ios_base::out bit is set, allows to
         *             write the data that is to be uploaded into the
         *             stream.  If the std::ios_base::in bit is set,
         *             the reply body can be read from the stream after
         *             completing the upload.  Otherwise, the reply body
         *             will be read internally upon completing the upload,
         *             and is made available via the body() operation.
         *
         * \throw Redirection Server replied with a 3xx response.
         * \throw ClientError Server replied with a 4xx response.
         * \throw ServerError Server replied with a 5xx response.
         * \throw std::string Other error, such as a failure to connect to
         * the server.
         *
         * \pre The client stream is unconnected.
         * \post The client stream is connected.
         */

        void post (const OCPI::Util::Uri & uri, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
          throw (std::string, Redirection, ClientError, ServerError);

        /**
         * Initiate a <tt>POST</tt> request.
         *
         * After this operation succeeds, data can be written to the
         * stream until the upload is completed.
         *
         * The upload can be completed by reading from the stream (if
         * \a mode & std::ios_base::in), accessing any reply information,
         * or by explicitly calling completeUpload().
         *
         * Reply information is available only after the upload is
         * completed.
         *
         * Once the upload is completed, writing to the stream fails.
         *
         * \param[in] uri   The URI to post to.
         * \param[in] contentType If not empty, this value is sent as the
         *             parameter of the "Content-Type" header field.
         * \param[in] size  If not -1, this value is sent as the parameter
         *             of the <em>Content-Length</em> header field.  Behavior
         *             is undefined unless exactly this number of octets are
         *             written to the stream.
         * \param[in] mode  If the std::ios_base::out bit is set, allows to
         *             write the data that is to be posted into the
         *             stream. If the std::ios_base::in bit is set,
         *             the reply body can be read from the stream after
         *             completing the upload. Otherwise, the reply body
         *             will be read internally upon completing the upload,
         *             and is made available via the body() operation.
         *
         * \throw Redirection Server replied with a 3xx response.
         * \throw ClientError Server replied with a 4xx response.
         * \throw ServerError Server replied with a 5xx response.
         * \throw std::string Other error, such as a failure to connect to
         * the server.
         *
         * \pre The client stream is unconnected.
         * \post The client stream is connected.
         */

        void post (const OCPI::Util::Uri & uri, const std::string & contentType, unsigned long long size, std::ios_base::openmode mode)
          throw (std::string, Redirection, ClientError, ServerError);

        /**
         * Initiate a <tt>DELETE</tt> request.
         *
         * Makes reply infromation available.  The body of the reply can
         * be read from the stream, if \a mode & std::ios_base::in.
         * Writing to the stream fails.
         *
         * \param[in] uri   The URI location to delete.
         * \param[in] mode  If the std::ios_base::in bit is set, the reply
         *             body can be read from the stream after completing the
         *             upload.  Otherwise, the reply body will be read
         *             internally upon completing the upload, and is made
         *             available via the body() operation.
         *
         * \throw Redirection Server replied with a 3xx response.
         * \throw ClientError Server replied with a 4xx response.
         * \throw ServerError Server replied with a 5xx response.
         * \throw std::string Other error, such as a failure to connect to
         * the server.
         *
         * \pre The client stream is unconnected.
         * \post The client stream is connected.
         * \post Reply information is available.
         */

        void remove (const OCPI::Util::Uri & uri, std::ios_base::openmode mode = (std::ios_base::openmode) 0)
          throw (std::string, Redirection, ClientError, ServerError);

        //@}

        /**
         * \name Connection Management
         */

        //@{

        /**
         * Complete an upload, when processing a <tt>PUT</tt> or
         * <tt>POST</tt> request, after all data has been sent.
         *
         * Makes reply information available.  If the \a mode parameter to
         * the put() or post() operation had the std::ios_base::in bit set,
         * the reply body can be to read from the stream.  Otherwise, the
         * body is made available via the body() operation.
         *
         * Since a server may defer error processing until it has received
         * the complete request body, this operation may raise Redirection,
         * ClientError or ServerError exceptions.
         *
         * This operation has no effect when processing a <tt>HEAD</tt>,
         * <tt>GET</tt> or <tt>DELETE</tt> request.
         *
         * \throw Redirection Server replied with a 3xx response.
         * \throw ClientError Server replied with a 4xx response.
         * \throw ServerError Server replied with a 5xx response.
         * \throw std::string If the server fails without sending a response,
         * of if the response can not be parsed.
         *
         * \pre The client stream is connected.
         * \post Reply information is available.
         */

        void completeUpload ()
          throw (std::string, Redirection, ClientError, ServerError);

        /**
         * Completes the request and closes the stream.
         *
         * After closing the stream, a new request may be initiated.
         *
         * If a put() or post() operation is processed, and upload has
         * not completed yet, calls completeUpload(). If completeUpload()
         * fails, the exception is propagated to the caller after closing
         * the stream.
         *
         * May raise an exception of type std::string if the server fails
         * before all buffered data could be uploaded.
         *
         * \throw Redirection Server replied with a 3xx response.
         * \throw ClientError Server replied with a 4xx response.
         * \throw ServerError Server replied with a 5xx response.
         * \throw std::string If the server fails without sending a response,
         * or if the response can not be parsed.
         *
         * \pre The client stream is connected.
         * \post The client stream is unconnected.
         * \post Reply information is available.
         */

        void close ()
          throw (std::string, Redirection, ClientError, ServerError);

        //@}

        /**
         * \name Reply Information
         *
         * Reply information is available after initiating a head(), get()
         * or remove() request, or, in case of a put() or post() request,
         * after successfully calling completeUpload().
         *
         * If called during an upload operation, i.e., after initiating
         * a put() or post() request, but before calling completeUpload(),
         * these operations implicitly call completeUpload().
         *
         * Because these functions may implicitly call completeUpload(),
         * they may raise the same errors.
         */

        //@{

        /**
         * The HTTP status code.
         *
         * Status code values are defined in section 6.1.1 of the HTTP 1.1
         * specification (RFC 2616).
         *
         * Implicitly calls completeUpload() if necessary.  See its
         * documentation for error information.
         *
         * \return The HTTP status code from the response status line.
         */

        int statusCode ()
          throw (std::string, Redirection, ClientError, ServerError);

        /*
         * The set of header fields in the response.
         *
         * The header fields include general header fields (section 4.5),
         * response header fields (section 6.2) and entity header fields
         * (section 7.1).
         *
         * Implicitly calls completeUpload() if necessary.  See its
         * documentation for error information.
         *
         * \return The set of header fields in the response.
         */

        const Headers & responseHeaders ()
          throw (std::string, Redirection, ClientError, ServerError);

        /**
         * The content type of the reply body.
         *
         * Returns the value of the "Content-Type" header field (excluding
         * any parameters), or defaultContentType if the header field is not
         * present in the response headers.
         *
         * Implicitly calls completeUpload() if necessary.  See its
         * documentation for error information.
         *
         * \return The content type.
         */

        std::string contentType ()
          throw (std::string, Redirection, ClientError, ServerError);

        /*
         * The character set used by the reply body.
         *
         * Returns the value of the <em>charset</em> attribute to the
         * <em>Content-Type</em> header field.
         *
         * Returns defaultCharset if the server response includes the
         * <em>Content-Type</em> header field, its <em>media type</em>
         * is <tt>text</tt>, but is lacking the <em>charset</em>
         * attribute.
         *
         * Implicitly calls completeUpload() if necessary.  See its
         * documentation for other error information.
         *
         * Returns the default character set "iso-8859-1" and if the
         * Content-Type field is present, and if the "type" attribute is
         * "text", but there is no "charset" parameter.
         *
         * \return The character set.
         *
         * \throw std::string If the <em>Content-Type</em> header field
         * is not present in the response headers, implying binary content,
         * or if it is present and indicates a media type different than
         * <tt>text</tt>.
         */

        std::string charset ()
          throw (std::string, Redirection, ClientError, ServerError);

        /**
         * The length of the reply body, as indicated by the
         * <em>Content-Length</em> response header field, if present
         * and proper.
         *
         * Implicitly calls completeUpload() if necessary.  See its
         * documentation for other error information.
         *
         * \return The length of the reply body, in octets.
         *
         * \throw std::string If the <em>Content-Length</em> field is not
         * present in the response headers, or if the HTTP protocol instructs
         * clients to ignore the value (e.g., when HTTP chunking is used).
         */

        unsigned long long contentLength ()
          throw (std::string, Redirection, ClientError, ServerError);

        /**
         * The last-modification timestamp, as indicated by the
         * <em>Last-Modified</em> response header field, if present
         * and proper.
         *
         * Implicitly calls completeUpload() if necessary.  See its
         * documentation for other error information.
         *
         * \return The last-modification timestamp.
         *
         * \throw std::string If the <em>Last-Modified</em> field is not
         * present in the response headers, or if its value can not be
         * parsed.
         */

        std::time_t lastModified ()
          throw (std::string, Redirection, ClientError, ServerError);

        /**
         * The reply body, if it was read internally.
         *
         * The reply body is only made available if there was an HTTP
         * error (4xx or 5xx status code), or if the \a mode parameter
         * to the post(), put() or remove() operation did not have the
         * "in" bit set.  In the latter case, the reply body can be
         * read from the stream.
         *
         * \return The reply body.
         *
         * \throw std::string If the reply body is not available, or
         * if an error occured while reading the body.
         */

        const std::string & body ()
          throw (std::string, Redirection, ClientError, ServerError);

        //@}

      protected:
        /**
         * \name Communication Channel API for derived classes.
         *
         * These functions must be provided by a derived class.
         */

        //@{

        /**
         * Open a communication channel to a server that serves a given URI.
         *
         * Implementations may choose to have inStream and outStream point
         * to the same iostream instance.
         *
         * The implementation shall raise an exception of type std::string if
         * the connection can not be established.
         *
         * Only one connection will be opened at a time.  closeConnection()
         * will be called before openConnection() is called again.
         *
         * \param[in] uri The URI that is to be accessed.  The implementation
         *                should evaluate the <em>authority</em> URI
         *                component, and establish a connection to the server.
         * \param[out] inStream  The input channel of the connection to
         *                  read data from.
         * \param[out] outStream The output channel of the connection to
         *                  write data to.
         *
         * \throw std::string If the connection can not be opened.
         */

        virtual void openConnection (const OCPI::Util::Uri & uri,
                                     std::istream * & inStream,
                                     std::ostream * & outStream)
          throw (std::string) = 0;

        /**
         * Perform a half-close of the connection that was previously
         * returned from openConnection().
         *
         * \param[in] mode If std::ios_base::out, the output channel shall
         *             be closed, so that an end of file is signaled to the
         *             server.  If std::ios_base::in, the input channel
         *             shall be closed.  The "opposite" direction shall
         *             continue to be usable.
         */

        virtual void shutdownConnection (std::ios_base::openmode mode = std::ios_base::out)
          throw (std::string) = 0;

        /**
         * Close the input and output channels that were previously
         * returned from openConnection().
         *
         * \throw std::string If an error occurs while closing the
         * connection, such as a failure to flush buffered data.
         */

        virtual void closeConnection ()
          throw (std::string) = 0;

        //@}

        /*
         * Private API.
         */

        /** \cond */

      protected:
        void connect (const OCPI::Util::Uri &)
          throw (std::string);

        void reset ()
          throw ();

        void checkRelocation ()
          throw (std::string, Redirection);

        void checkServerError ()
          throw (std::string, ClientError, ServerError);

        void readBody ()
          throw ();

      protected:
        void headOrDelete (int, const OCPI::Util::Uri &, std::ios_base::openmode)
          throw (std::string, Redirection, ClientError, ServerError);

        void putOrPost (int, const OCPI::Util::Uri &, const std::string &, unsigned long long, std::ios_base::openmode)
          throw (std::string, Redirection, ClientError, ServerError);

      protected:
        static void shutdownForBuf (void *)
          throw (std::string);

      protected:
        bool m_connected;
        bool m_uploading;
        bool m_errorReported;
        ClientBuf m_buf;
        std::ios_base::openmode m_mode;
        std::string m_body;

        /** \endcond */

      private:
        /**
         * Not implemented.
         */

        ClientStream (const ClientStream &);

        /**
         * Not implemented.
         */

        ClientStream & operator= (const ClientStream &);
      };

      /**
       * \brief HTTP client.
       *
       * This class extends the abstract OCPI::Util::Http::ClientStream class,
       * delegating connections to a transport provider ("Connector")
       * template parameter.  See OCPI::Util::Tcp::Connector for a suitable
       * TCP-based transport provider class.
       *
       * See the OCPI::Util::Http::ClientStream for more information about
       * this class' functionality.
       *
       * This code fragment illustrates the use of the Client class using
       * the TCP connector.
       *
       * \code
       *   OCPI::Util::Http::Client<OCPI::Util::Tcp::Connector> mc;
       *   mc.get ("http://www.mc.com/");
       *   // read the mc.com homepage from the "mc" stream
       *   mc.close ();
       * \endcode
       */

      template<class Connector>
      class Client : public ClientStream {
      private:
        Connector m_conn;

      public:
        /**
         * Constructor.
         *
         * After construction, a request may be initiated using the
         * operations in the ClientStream base class.
         */

        inline Client ()
          throw ();

        /**
         * Constructor.
         *
         * Calls OCPI::Util::Http::ClientStream::get (uri).
         *
         * \param[in] uri A URI to open for reading.
         *
         * \throw Redirection Server replied with a 3xx response.
         * \throw ClientError Server replied with a 4xx response.
         * \throw ServerError Server replied with a 5xx response.
         * \throw std::string Other error, such as a failure to connect to
         * the server.
         */

        inline Client (const OCPI::Util::Uri & uri)
          throw (std::string, Redirection, ClientError, ServerError);

        /**
         * Destructor.
         */

        inline ~Client ()
          throw ();

      protected:
        /**
         * \name Implementation of the communication channel API.
         */

        //@{

        void openConnection (const OCPI::Util::Uri & uri,
                             std::istream * & inConn,
                             std::ostream * & outConn)
          throw (std::string);

        void shutdownConnection (std::ios_base::openmode mode = std::ios_base::out)
          throw (std::string);

        void closeConnection ()
          throw (std::string);

        //@}

      private:
        /**
         * Not implemented.
         */

        Client (const Client &);

        /**
         * Not implemented.
         */

        Client & operator= (const Client &);
      };

    }
  }
}

/*
 * ----------------------------------------------------------------------
 * OCPI::Util::Http::Client<Connector>
 * ----------------------------------------------------------------------
 */

/** \cond */

template<class Connector>
inline
OCPI::Util::Http::Client<Connector>::Client ()
  throw ()
{
}

template<class Connector>
inline
OCPI::Util::Http::Client<Connector>::Client (const OCPI::Util::Uri & uri)
  throw (std::string, Redirection, ClientError, ServerError)
{
  get (uri);
}

template<class Connector>
inline
OCPI::Util::Http::Client<Connector>::~Client ()
  throw ()
{
  try {
    close ();
  }
  catch (...) {
  }
}

template<class Connector>
inline
void
OCPI::Util::Http::Client<Connector>::
openConnection (const OCPI::Util::Uri & uri,
                std::istream * & inConn,
                std::ostream * & outConn)
  throw (std::string)
{
  std::string authority = OCPI::Util::Uri::decode (uri.getHostport());
  std::iostream * str = m_conn.connect (authority);
  inConn = str;
  outConn = str;
}

template<class Connector>
inline
void
OCPI::Util::Http::Client<Connector>::
shutdownConnection (std::ios_base::openmode mode)
  throw (std::string)
{
  m_conn.shutdown (mode);
}

template<class Connector>
inline
void
OCPI::Util::Http::Client<Connector>::
closeConnection ()
  throw (std::string)
{
  m_conn.close ();
}

/** \endcond */

#endif
