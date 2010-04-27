// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

#include <CpiLoggerLogger.h>
#include <CpiLoggerNullOutput.h>
#include <streambuf>
#include <iostream>

/*
 * ----------------------------------------------------------------------
 * NullOutput: throw away log messages
 * ----------------------------------------------------------------------
 */

CPI::Logger::NullOutput::NullOutputBuf::~NullOutputBuf ()
{
}

void
CPI::Logger::NullOutput::NullOutputBuf::setLogLevel (unsigned short)
{
}

void
CPI::Logger::NullOutput::NullOutputBuf::setProducerId (const char *)
{
}

void
CPI::Logger::NullOutput::NullOutputBuf::setProducerName (const char *)
{
}

std::streambuf::int_type
CPI::Logger::NullOutput::NullOutputBuf::overflow (int_type c)
{
  if (traits_type::eq (c, traits_type::eof())) {
    return traits_type::not_eof (c);
  }

  return c;
}

std::streamsize
CPI::Logger::NullOutput::NullOutputBuf::xsputn (const char *, std::streamsize count)
{
  return count;
}

CPI::Logger::NullOutput::NullOutput ()
  : Logger (m_obuf)
{
}

CPI::Logger::NullOutput::~NullOutput ()
{
}


