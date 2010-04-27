// -*- c++ -*-

/**
 * \brief Log to the Lightweight Logging Service
 */

#ifndef CPIUTILLWLOGGEROUTPUT_H__
#define CPIUTILLWLOGGEROUTPUT_H__

#include <string>
#include <CpiOsMutex.h>
#include <CpiUtilIOP.h>
#include <CpiUtilIIOP.h>
#include <CpiLoggerLogger.h>
#include <CpiUtilTcpClient.h>

namespace CPI {
  namespace Util {

    class LwLoggerOutput : public ::CPI::Logger::Logger {
    public:
      class Buf : public LogBuf {
      public:
	Buf (const CPI::Util::IOP::IOR &);
	~Buf ();

	void setLogLevel (unsigned short);
	void setProducerId (const char *);
	void setProducerName (const char *);

      protected:
	int sync ();
	int_type overflow (int_type = std::streambuf::traits_type::eof());
	std::streamsize xsputn (const char *, std::streamsize);

      protected:
	bool connectToLogService ();
	bool sendMessage ();

      protected:
	// logger information
	bool m_first;
	bool m_locked;
	unsigned short m_logLevel;
	std::string m_producerId;
	std::string m_producerName;
	std::string m_logMessage;
	CPI::OS::Mutex m_lock;

      protected:
	// connection information
	bool m_connected;
	bool m_byteOrder;
	unsigned int m_requestId;
	CPI::Util::IOP::IOR m_ior;
	CPI::Util::Tcp::Client m_conn;
	CPI::Util::IIOP::ProfileBody m_profile;
      };

    public:
      LwLoggerOutput (const CPI::Util::IOP::IOR & ior);
      ~LwLoggerOutput ();

    protected:
      Buf m_obuf;
    };

  }
}

#endif
