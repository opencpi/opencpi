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

#include <OcpiLoggerLogger.h>
#include <OcpiLoggerNullOutput.h>
#include <streambuf>
#include <iostream>

/*
 * ----------------------------------------------------------------------
 * NullOutput: throw away log messages
 * ----------------------------------------------------------------------
 */

OCPI::Logger::NullOutput::NullOutputBuf::~NullOutputBuf ()
{
}

void
OCPI::Logger::NullOutput::NullOutputBuf::setLogLevel (unsigned short)
{
}

void
OCPI::Logger::NullOutput::NullOutputBuf::setProducerId (const char *)
{
}

void
OCPI::Logger::NullOutput::NullOutputBuf::setProducerName (const char *)
{
}

std::streambuf::int_type
OCPI::Logger::NullOutput::NullOutputBuf::overflow (int_type c)
{
  if (traits_type::eq_int_type (c, traits_type::eof())) {
    return traits_type::not_eof (c);
  }

  return c;
}

std::streamsize
OCPI::Logger::NullOutput::NullOutputBuf::xsputn (const char *, std::streamsize count)
{
  return count;
}

OCPI::Logger::NullOutput::NullOutput ()
  : Logger (m_obuf)
{
}

OCPI::Logger::NullOutput::~NullOutput ()
{
}


