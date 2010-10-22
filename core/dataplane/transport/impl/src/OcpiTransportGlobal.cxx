
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


/*
 * Abstact:
 *   This class contains all global data used by the OCPI transport layer.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */
#include <stdio.h>
#include <string>
#include <OcpiOsAssert.h>
#include <DtHandshakeControl.h>
#include <DtTransferInternal.h>
#include <OcpiTransportGlobal.h>
#include <OcpiTransferController.h>
#include <OcpiTemplateGenerators.h>

#include <OcpiParallelDataDistribution.h>

using namespace OCPI::DataTransport;
using namespace DataTransfer;
using namespace DtI;
using namespace OCPI::Util;
using namespace OCPI::OS;

#define EVENT_START 100
#define EVENT_RANGE 20

#ifdef PROCESS_WIDE_GLOBALS
// Test setup structure
#ifdef TEST_TRANSPORT_STANDALONE
ConnectionTest       TransportGlobal::m_testSetup;
#endif
OCPI::OS::uint32_t      TransportGlobal::m_sharedMemorySize = 500000;
bool    TransportGlobal::m_ooPZeroCopy = false;

VList TransportGlobal::m_localEndpoints;
VList TransportGlobal::m_remoteEndpoints;
#endif



void TransportGlobal::clearGlobalCache()
{
  // remove me

}



/**********************************
 * Init method
 *********************************/
void TransportGlobal::init()
{

}


void TransportGlobal::parseArgs( int argc, char **argv )
{
  ( void ) argc;
  ( void ) argv;
}


TransportGlobal::TransportGlobal( int argcp, char **argvp )
    : m_Circuitinit(false)

{
  ( void ) argcp;
  ( void ) argvp;
  init();
}



TransportGlobal::TransportGlobal( int event_ordinal, bool asyc )
  : m_useEvents(asyc),m_Circuitinit(false)
{
  int low = EVENT_START + event_ordinal*EVENT_RANGE;
  int high = low+EVENT_RANGE-1;
  if ( asyc ) {
#ifndef NDEBUG
    printf("GPP: Using events\n");        
#endif
    //        Create a asyc event handler object
    m_event_manager = new DataTransfer::EventManager(low,high);
  }
  else {
#ifndef NDEBUG
    printf("GPP: Not Using events\n");                
#endif
    m_event_manager = NULL;
  }
        
  init();
}

TransportGlobal::~TransportGlobal()
{
  if ( m_event_manager ) delete m_event_manager;

  delete m_gen_pat1AFC;
  delete m_gen_pat1AFCShadow;
  delete m_gen_temp_gen;
  delete m_gen_pat1;
  delete m_gen_pat2;
  delete m_gen_pat3;
  delete m_gen_pat4;
  delete m_gen_control;
  delete m_cont1;
  delete m_cont1AFCShadow;
  delete m_cont2;
  delete m_cont3;
  delete m_cont4;


  clearGlobalCache();

}

