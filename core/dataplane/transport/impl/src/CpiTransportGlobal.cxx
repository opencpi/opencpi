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

/*
 * Abstact:
 *   This class contains all global data used by the CPI transport layer.
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
#include <CpiOsAssert.h>
#include <DtHandshakeControl.h>
#include <DtTransferInternal.h>
#include <CpiTransportGlobal.h>
#include <CpiTransferController.h>
#include <CpiTemplateGenerators.h>

#include <CpiParallelDataDistribution.h>

using namespace CPI::DataTransport;
using namespace DataTransfer;
using namespace DtI;
using namespace CPI::Util;
using namespace CPI::OS;

#define EVENT_START 100
#define EVENT_RANGE 20

#ifdef PROCESS_WIDE_GLOBALS
// Test setup structure
#ifdef TEST_TRANSPORT_STANDALONE
ConnectionTest       TransportGlobal::m_testSetup;
#endif
CPI::OS::uint32_t      TransportGlobal::m_sharedMemorySize = 500000;
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


}


TransportGlobal::TransportGlobal( int argcp, char **argvp )
    : m_Circuitinit(false)

{
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

