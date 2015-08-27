
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
 *   This file contains the Interface for OCPI tranport globals.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DataTransport_TransportGlobal_H_
#define OCPI_DataTransport_TransportGlobal_H_


#include <string>
#include <OcpiList.h>
#include <OcpiParentChild.h>
#include <OcpiOsDataTypes.h>
#include <OcpiTransportExceptions.h>
#include <OcpiRDTInterface.h>
#include <DtSharedMemoryInternal.h>
#include <DtIntEventHandler.h>
#include <OcpiTransportConstants.h>


// Forward references
namespace DataTransfer {
  struct ContainerComms;
  class OcpiResourceServices;
  struct SMBResources;
}


class TransportTestBase;

namespace DataTransport {
  namespace Interface {
    class DataDistribution;    
  }
}

namespace OCPI {
  namespace Container {
    class Factory;
  }
}

#define P_STATIC 

namespace OCPI {

  namespace DataTransport {

    class TransferTemplateGenerator;
    class TransferTemplateGeneratorPattern1;
    class TransferTemplateGeneratorPattern1Passive;
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
        TransportGlobal( OCPI::OS::int32_t argc, char** argv );
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

	// Init flag
	static bool m_init;
                        
        // polling or events
        P_STATIC bool m_useEvents;

        // Use out of process zero copy
        P_STATIC bool m_ooPZeroCopy;        
        P_STATIC bool m_Circuitinit;
        P_STATIC TransferTemplateGenerator                  *m_gen_temp_gen;
        P_STATIC TransferTemplateGeneratorPattern1          *m_gen_pat1;
        P_STATIC TransferTemplateGeneratorPattern1          *m_gen_pat1passive;
        P_STATIC TransferTemplateGeneratorPattern1AFC       *m_gen_pat1AFC;
        P_STATIC TransferTemplateGeneratorPattern1AFCShadow *m_gen_pat1AFCShadow;
        P_STATIC TransferTemplateGeneratorPattern2 *m_gen_pat2;
        P_STATIC TransferTemplateGeneratorPattern3 *m_gen_pat3;
        P_STATIC TransferTemplateGeneratorPattern4 *m_gen_pat4;
        P_STATIC TransferController                *m_gen_control;
        P_STATIC TransferController1               *m_cont1;
        P_STATIC TransferController1               *m_cont1passive;
        P_STATIC TransferController1AFCShadow      *m_cont1AFCShadow;
        P_STATIC TransferController2               *m_cont2;
        P_STATIC TransferController3               *m_cont3;
        P_STATIC TransferController4               *m_cont4;

        // Our list of transfer controllers                 s dist s part t dist t part shadow s role              t role
        P_STATIC TransferController* m_transferControllers  [2]    [2]    [2]    [2]    [2]      [OCPI::RDT::MaxRole] [OCPI::RDT::MaxRole];

        // Our list of template generators
        P_STATIC TransferTemplateGenerator* m_templateGenerators [2][2][2][2] [2] [OCPI::RDT::MaxRole] [OCPI::RDT::MaxRole];

      protected:

        // Our event handler
        P_STATIC DataTransfer::EventManager* m_event_manager;

        /**********************************
         * Parse args
         *********************************/
        void parseArgs( OCPI::OS::int32_t argc, char** argv );

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
