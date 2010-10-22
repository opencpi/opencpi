
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
 *   This file contains the OCPI template generator interface.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DataTransport_TemplateGenerator_H_
#define OCPI_DataTransport_TemplateGenerator_H_

#include <OcpiTransferTemplate.h>
#include <OcpiTransport.h>

namespace  OCPI {
  namespace DataTransport {
    class TransferController;
  }
}

namespace DtI = ::DataTransport::Interface;
namespace OCPI {

  namespace DataTransport {

    class PortSet;
    class Port;

    // this needs to move to its own file
    class TransferTemplateGenerator {

    public:

      TransferTemplateGenerator();
      virtual ~TransferTemplateGenerator();

      // This method creates all 
      virtual TransferController* createTemplates ( Transport* transport,
                                                    PortSet* output, 
                                                    PortSet* input, 
                                                    TransferController* cont );

    protected:

      // This structure is used to pass additional template information to the 
      // "addTransfer" hooks.
      struct TDataInterface {
        OCPI::DataTransport::Port*            s_port;
        OCPI::OS::int32_t                     s_tid;
        OCPI::DataTransport::Port*            t_port;
        OCPI::OS::int32_t                     t_tid;
        TDataInterface(OCPI::DataTransport::Port* sp, OCPI::OS::int32_t st, 
                       OCPI::DataTransport::Port* tp,OCPI::OS::int32_t tt)
          :s_port(sp),s_tid(st),t_port(tp),t_tid(tt){};
      };

      // Call appropriate creator
      void create( Transport* t, PortSet* output, PortSet* input, TransferController* cont );

      // Create transfers for input port
      virtual void createInputTransfers( 
                                          PortSet* output, 
                                          Port* input,
                                          TransferController* cont );


      // Create transfers for output port
      virtual void createOutputTransfers(OCPI::DataTransport::Port* output, 
                                         OCPI::DataTransport::PortSet* input, TransferController* cont  )=0;

      // These methods are hooks to add additional transfers
      virtual DataTransfer::XferRequest* addTransferPreData( 
                                                            TDataInterface& tdi);
      virtual DataTransfer::XferRequest* addTransferPostData(         
                                                             TDataInterface& tdi);
      virtual DataTransfer::XferRequest* addTransferPreState(         
                                                             TDataInterface& tdi);

      // Create the output broadcast template for this set
      virtual void createOutputBroadcastTemplates(OCPI::DataTransport::Port* output, 
                                                  OCPI::DataTransport::PortSet* input, TransferController* cont);

      // Create the input broadcast template for this set
      virtual void createInputBroadcastTemplates(OCPI::DataTransport::PortSet* output, OCPI::DataTransport::Port* input, 
                                                  TransferController* cont  );

      // Our controller
      TransferController* m_controller;

      // Zero copy enabled
      bool m_zcopyEnabled;

    };



    // This class is used to detect transfer pattern generation requests for patterns that are not
    // supported
    class TransferTemplateGeneratorNotSupported : public TransferTemplateGenerator
    {
    public:
      virtual ~TransferTemplateGeneratorNotSupported(){}

    protected:

      // Create transfers for output port
      void createOutputTransfers(OCPI::DataTransport::Port* s_port, OCPI::DataTransport::PortSet* input,
                                         TransferController* cont )
      {
        ( void ) s_port;
        ( void ) input;
        ( void ) cont;
        ocpiAssert(!"Unsupported data transfer request rejected !!\n");
        throw OCPI::Util::EmbeddedException("Unsupported data transfer request rejected !!\n");
      }

      // Create transfers for input port
      void createInputTransfers( 
                                 PortSet* output, 
                                 Port* input,
                                 TransferController* cont )
      {
        ( void ) input;
        ( void ) output;
        ( void ) cont;
        ocpiAssert(!"Unsupported data transfer request rejected !!\n");
        throw OCPI::Util::EmbeddedException("Unsupported data transfer request rejected !!\n");
      }

    };





    // This transfer template generator is used for the following transfer types
    // 1-1, 3-1, 1-3, 3-3.  Basically it is used for whole parellel /parellel whole
    // patterns.
    class TransferTemplateGeneratorPattern1 : public TransferTemplateGenerator
    {
    public:

      // Constructor
      TransferTemplateGeneratorPattern1( )
        :TransferTemplateGenerator(){};

        // Destructor
        virtual ~TransferTemplateGeneratorPattern1();

    protected:

        // Create transfers for output port
        virtual void createOutputTransfers(OCPI::DataTransport::Port* s_port, OCPI::DataTransport::PortSet* input,
                                           TransferController* cont );

        // Create transfers for input port
        virtual void createInputTransfers(OCPI::DataTransport::PortSet* output, OCPI::DataTransport::Port* input,
                                           TransferController* cont );

    };




    // This is the class that is used for pattern 1 when the port role is ActiveFlowControl for the
    // real port
    class TransferTemplateGeneratorPattern1AFC : public TransferTemplateGeneratorPattern1
    {
    public:

      // Constructor
      TransferTemplateGeneratorPattern1AFC( )
        :TransferTemplateGeneratorPattern1(){};

    protected:

      // Create transfers for output port.  In this case the output port only tells the remote input that 
      // data is available, it does not push and data or meta-data.
      virtual void createOutputTransfers(OCPI::DataTransport::Port* s_port, OCPI::DataTransport::PortSet* input,
                                         TransferController* cont );

      // Create transfers for input port. When a real input port "consumes" data, it only needs set the local
      // flag since all transfers from remote output to local input are managed locally when the remote output
      // has a ActiveFlowControl role.
      virtual void createInputTransfers(OCPI::DataTransport::PortSet* output, OCPI::DataTransport::Port* input,
                                         TransferController* cont );


    };


    // This is the class that is used for pattern 1 when the port role is ActiveFlowControl for the 
    // shadow port
    class TransferTemplateGeneratorPattern1AFCShadow : public TransferTemplateGeneratorPattern1
    {
    public:

      // Constructor
      TransferTemplateGeneratorPattern1AFCShadow( )
        :TransferTemplateGeneratorPattern1(){};

    protected:

        // Create transfers for output port
        virtual void createOutputTransfers(OCPI::DataTransport::Port* s_port, OCPI::DataTransport::PortSet* input,
                                           TransferController* cont );




    };



    // This transfer template generator is used for the following transfer types
    // x-x  Basically it is used for whole parellel / sequential(dynamic) whole transfers
    class TransferTemplateGeneratorPattern2 : public TransferTemplateGenerator
    {
    public:

      TransferTemplateGeneratorPattern2( )
        :TransferTemplateGenerator(){};

        // Destructor
        virtual ~TransferTemplateGeneratorPattern2(){};

    protected:

        // Create transfers for input port
        virtual void createInputTransfers(OCPI::DataTransport::PortSet* output, OCPI::DataTransport::Port* input, 
                                           TransferController* cont );



        // Create transfers for output port
        virtual void createOutputTransfers(OCPI::DataTransport::Port* output, OCPI::DataTransport::PortSet* input,
                                           TransferController* cont  );

    };



    // This transfer template generator is used for the following transfer types
    // x-x  Basically it is used for whole sequential(rr) / sequential(dynamic) whole transfers
    class TransferTemplateGeneratorPattern3 : public TransferTemplateGeneratorPattern2
    {
    public:

      TransferTemplateGeneratorPattern3( )
        :TransferTemplateGeneratorPattern2(){};

        // Destructor
        virtual ~TransferTemplateGeneratorPattern3(){};

    protected:

        // These methods are hooks to add additional transfers
        virtual DataTransfer::XferRequest* addTransferPreState( 
                                                               TDataInterface& tdi);

    };




    // This transfer template generator is used for the following transfer types
    // x-x  Basically it is used for Whole Parallel -> Parallel Parts
    class TransferTemplateGeneratorPattern4 : public TransferTemplateGenerator
    {
    public:

      TransferTemplateGeneratorPattern4(  )
        : TransferTemplateGenerator(),m_markEndOfWhole(true){m_zcopyEnabled=false;};

        // Destructor
        virtual ~TransferTemplateGeneratorPattern4(){};

    protected:

        // Create transfers for input port
        virtual void createInputTransfers(OCPI::DataTransport::PortSet* output, OCPI::DataTransport::Port* input, 
                                           TransferController* cont );


        // Create transfers for output port
        virtual void createOutputTransfers(OCPI::DataTransport::Port* output, OCPI::DataTransport::PortSet* input,
                                           TransferController* cont  );

        // Inform all ports when an end of whole has been reached
        bool m_markEndOfWhole;

    };

  }

}

#endif

