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
 *   This file contains the Interface for CPI tranport globals.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef CPI_DataTransport_TransportGlobal_H_
#define CPI_DataTransport_TransportGlobal_H_


#include <string>
#include <CpiList.h>
#include <CpiParentChild.h>
#include <CpiOsDataTypes.h>
#include <CpiTransportExceptions.h>
#include <CpiRDTInterface.h>
#include <DtSharedMemoryInternal.h>
#include <DtIntEventHandler.h>
#include <CpiTransportConstants.h>


// Forward references
namespace DataTransfer {
  struct ContainerComms;
  class CpiResourceServices;
  struct SMBResources;
}


class TransportTestBase;

namespace DataTransport {
  namespace Interface {
    class DataDistribution;    
  }
}

namespace CPI {
  namespace Container {
    class Factory;
  }
}

#define P_STATIC 

namespace CPI {

  namespace DataTransport {

    class TransferTemplateGenerator;
    class TransferTemplateGeneratorPattern1;
    class TransferTemplateGeneratorPattern1AFC;
    class TransferTemplateGeneratorPattern1AFCShadow;
    class TransferTemplateGeneratorPattern2;
    class TransferTemplateGeneratorPattern3;
    class TransferTemplateGeneratorPattern4;
    class TransferController;
    class TransferController1;
    class TransferController1AFCShadow;
    class TransferController2;
    class TransferController3;
    class TransferController4;

    class TransportGlobal 
      {

      public:

        /**********************************
         * Constructors
         *********************************/
        TransportGlobal( CPI::OS::int32_t argc, char** argv );
        TransportGlobal( int event_ordinal, bool use_events);

        /**********************************
         * Destructor
         *********************************/
        virtual ~TransportGlobal();

        /**********************************
         * Get the shared memory service by smb id
         *********************************/
        P_STATIC DataTransfer::SmemServices* getSmemFromLocation( DataTransfer::EndPoint& loc, bool create=0 );

        /**********************************
         * Clear the global cache
         *********************************/
        P_STATIC void clearGlobalCache();

        /**********************************
         * Get the eventmanager object
         *********************************/
        DataTransfer::EventManager* getEventManager();
        bool                        useEvents();
                        
        // polling or events
        P_STATIC bool m_useEvents;

        // Use out of process zero copy
        P_STATIC bool m_ooPZeroCopy;        
        P_STATIC bool m_Circuitinit;
        P_STATIC TransferTemplateGenerator                  *m_gen_temp_gen;
        P_STATIC TransferTemplateGeneratorPattern1          *m_gen_pat1;
        P_STATIC TransferTemplateGeneratorPattern1AFC       *m_gen_pat1AFC;
        P_STATIC TransferTemplateGeneratorPattern1AFCShadow *m_gen_pat1AFCShadow;
        P_STATIC TransferTemplateGeneratorPattern2 *m_gen_pat2;
        P_STATIC TransferTemplateGeneratorPattern3 *m_gen_pat3;
        P_STATIC TransferTemplateGeneratorPattern4 *m_gen_pat4;
        P_STATIC TransferController                *m_gen_control;
        P_STATIC TransferController1               *m_cont1;
        P_STATIC TransferController1AFCShadow      *m_cont1AFCShadow;
        P_STATIC TransferController2               *m_cont2;
        P_STATIC TransferController3               *m_cont3;
        P_STATIC TransferController4               *m_cont4;

        // Our list of transfer controllers                 s dist s part t dist t part shadow s role              t role
        P_STATIC TransferController* m_transferControllers  [2]    [2]    [2]    [2]    [2]      [CPI::RDT::MaxRole] [CPI::RDT::MaxRole];

        // Our list of template generators
        P_STATIC TransferTemplateGenerator* m_templateGenerators [2][2][2][2] [2] [CPI::RDT::MaxRole] [CPI::RDT::MaxRole];

      protected:

        // Our event handler
        P_STATIC DataTransfer::EventManager* m_event_manager;

        /**********************************
         * Parse args
         *********************************/
        void parseArgs( CPI::OS::int32_t argc, char** argv );

        /**********************************
         * Init method
         *********************************/
        void init();

      };

    /**********************************
     ****
     * inline declarations
     ****
     *********************************/
    inline bool TransportGlobal::useEvents(){return m_useEvents;}
    inline         DataTransfer::EventManager* TransportGlobal::getEventManager()
      {return m_event_manager;}

  }

}


#endif
