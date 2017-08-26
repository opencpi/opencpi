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

/*
 * Abstact:
 *   This file contains the implementation for the Message based transfer driver
 *   base classes.
 *
 * Revision History:

   09/4/11 - John Miller
   Initial version.

 *
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ezxml.h>
#include <OcpiOsAssert.h>
#include <OcpiOsMisc.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiUtilEzxml.h>
#include <OcpiPValue.h>
#include <DtMsgDriver.h>

namespace OX = OCPI::Util::EzXml;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace OD = OCPI::Driver;
namespace DTM = DataTransfer::Msg;

using namespace DTM;

namespace DataTransfer {
  namespace Msg {
    const char *msg_transfer="ocpi_msg_tx";
  }
}

// These defaults are pre-configuration
DTM::FactoryConfig::
FactoryConfig(uint32_t retryCount)
  : m_retryCount(8)
{
  if (retryCount)
    m_retryCount = retryCount;
}

// Parse and default from parent
void 
DTM::FactoryConfig::
parse(FactoryConfig *parent, ezxml_t x) {
  if (parent)
    *this = *parent;
  m_xml = x;
  if (x) {
    const char *err;
    // Note we are not writing defaults here because they are set
    // in the constructor, and they need to be set even when there is no xml
    if ((err = OX::checkAttrs(x, "TxRetryCount", NULL)) ||
	(err = OX::getNumber(x, "TxRetryCount", &m_retryCount, NULL, 0, false)))
      throw err; // FIXME configuration api error exception class
  }
}


// Configure this manager.  The drivers will be configured by the base class
void 
DTM::XferFactoryManager::
configure( ezxml_t x)
{
  if (!m_configured) {
    m_configured = true;
    parse(NULL, x);

    /*
    // Allow the environment to override config files here
    const char* env = getenv("OCPI_SMB_SIZE");
    if ( env && OX::getUNum(env, &m_SMBSize))
      throw "Invalid OCPI_SMB_SIZE value";
    */

    // Now configure the drivers
    OD::Manager::configure(x);

  }
}

DTM::XferFactory* 
DTM::XferFactoryManager::
findFactory( const char* url,
	    const OCPI::Util::PValue *our_props,
	    const OCPI::Util::PValue *other_props )
{
  parent().configure();
  OU::AutoMutex guard ( m_mutex, true );
  for (XferFactory* d = firstDriver(); d; d = d->nextDriver())
    if (d->supportsTx(url,our_props,other_props))
      return d;
  return NULL;

}

DTM::XferFactoryManager::
XferFactoryManager() : m_configured(false)
{

}

DTM::XferFactoryManager::
~XferFactoryManager()
{


}



DTM::XferFactory::
XferFactory( const char* name)
  : OCPI::Driver::DriverType<DTM::XferFactoryManager, DTM::XferFactory>(name, *this)
{

}

void 
DTM::XferFactory::
configure(ezxml_t x) {
  // parse generic attributes and default from parent
  parse(&DTM::XferFactoryManager::getFactoryManager(), x);
  // base class does device config if present
  OD::Driver::configure(x); 
}


void 
DTM::Device::
configure(ezxml_t x)
{
  OD::Device::configure(x); // give the base class a chance to do generic configuration
  parse(&driverBase(), x);
}

DTM::XferFactory::
~XferFactory()
{


}




