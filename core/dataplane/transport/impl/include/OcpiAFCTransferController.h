
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
 *   This file contains the OCPI transfer controller interface.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#error


#ifndef OCPI_AFC_DataTransport_TransferController_H_
#define OCPI_AFC_DataTransport_TransferController_H_

#include <OcpiTransferController.h>

namespace OCPI {

  namespace DataTransport {


    /*****
     *  This controller is what manages the port buffers and determines what 
     ****/
    class AFCTransferController : public TransferController {
    public:

      enum TransferType {
        SOURCE,
        TARGET
      };

      /**********************************
       * Constructor
       *********************************/
      AFCTransferController();

      /**********************************
       * Destructor
       *********************************/
      virtual ~AFCTransferController();


      /**********************************
       * This method with this role requires a copy to occur from the source buffer to the
       * target buffer.  
       *********************************/
      virtual void modifySourceOffsets( Buffer* me, Buffer* new_buffer, bool reverse );

      /**********************************
       * Creates an instance of a controller
       *********************************/
      virtual TransferController* createController( 
                                                   OCPI::DataTransport::PortSet* source, 
                                                   OCPI::DataTransport::PortSet* target, 
                                                   bool whole_source_set)=0;

      /**********************************
       * Get/Set transfer template
       *********************************/
      virtual OcpiTransferTemplate* getTemplate(
                                       OCPI::OS::uint32_t sp,                        // In - Source port id
                                       OCPI::OS::uint32_t stid,                // In - Source buffer id
                                       OCPI::OS::uint32_t tp,                        // In - Target port id
                                       OCPI::OS::uint32_t ttid,                // In - Target buffer id
                                       bool bcast,         // In - Broadcast template ?
                                       TransferType tt); // In - Transfer type

      /******************************
       * This method is used to determine if a transfer can be started while
       * a previous transfer is queued.
       *********************************/
      virtual bool canTransferBufferWhileOthersAreQueued();

      /**********************************
       * This method gets the next available buffer from the specified source port
       *********************************/
      virtual Buffer* getNextSourceBuffer(  OCPI::DataTransport::Port* src_port                        // In - Source port
                                            );

      /**********************************
       * This method determines if there is an available buffer, but does not affect the
       * state of the object.
       *********************************/
      virtual bool hasAvailableSourceBuffer(
                                            OCPI::DataTransport::Port* port
                                            )const;

      /**********************************
       * This method determines if there is data available, but does not affect the
       * state of the object.
       *********************************/
      virtual bool hasFullTargetBuffer(
                                       OCPI::DataTransport::Port* port,               
                                       TargetBuffer**
                                       )const;
                                
      /**********************************
       * This is used to indicate that a buffer has been filled. Since we manage circular
       * buffers, the actual buffer is implied (next)
       *
       *********************************/
      virtual void bufferFull(
                              OCPI::DataTransport::Port* port                                                
                              );
                                                
                                                
      /**********************************
       * This is used to indicate that a remote target 
       * buffer has been freed. Since we manage circular
       * buffers, the actual buffer is implied (next)
       *
       *********************************/
      virtual void freeBuffer(
                              OCPI::DataTransport::Port* port                                                
                              );

      /**********************************
       * This is used to indicate that all remote target 
       * buffer has been emptied.
       *
       *********************************/
      virtual void freeAllBuffersLocal(
                                       OCPI::DataTransport::Port* port                                                
                                       );


      /**********************************
       * This method gets the next available buffer from the specified source port
       *********************************/
      virtual bool useThisSourceBuffer( 
                                       OCPI::DataTransport::Port* src_port                        // In - Source port
                                       );

      /**********************************
       * This method gets the next available buffer from the specified target port
       *********************************/
      virtual Buffer* getNextTargetBuffer( 
                                               OCPI::DataTransport::Port* target_port                        // In - Source port
                                               );

      /**********************************
       * This method determines if we can produce from the indicated buffer
       *********************************/
      virtual bool canProduce( 
                              Buffer* buffer                        // InOut - Buffer to produce from
                              )=0;

      /**********************************
       * This method determines if we can produce from the indicated buffer
       *********************************/
      virtual bool canBroadcast( 
                                Buffer* buffer                        // InOut - Buffer to produce from
                                );

      /**********************************
       * This method determines if we have the source barrier token
       *********************************/
      virtual bool haveSourceBarrierToken( SourceBuffer* src_buf );

      /**********************************
       * This initiates a data transfer from the source buffer.  If the transfer can take place, 
       * it will be initiated, if not it will be queued in the circuit.
       *********************************/
      virtual int produce( 
                          Buffer* buffer,                        // InOut - Buffer to produce from
                          bool      broadcast=false        // In    - Broadcast the buffer
                          )=0;

      /**********************************
       * This initiates a broadcastdata transfer from the source buffer.
       *********************************/
      virtual void broadCastSource(
                                   Buffer* buffer                        // InOut - Buffer to produce from
                                   );

      /**********************************
       * This marks the target buffer as "Empty" and informs all interested sources that
       * the target is now available.
       *********************************/
      virtual Buffer* consume( 
                              Buffer* buffer                        // InOut - Buffer to consume
                              )=0;

      /**********************************
       * This marks the target buffer as "Empty" and informs all interested sources that
       * the target is now available.
       *********************************/
      virtual void consumeAllBuffersLocal( 
                                          OCPI::DataTransport::Port* port                          
                                          );

      /**********************************
       * Add a template
       *********************************/
      void addTemplate( OcpiTransferTemplate* temp, 
                        OCPI::OS::uint32_t sp, 
                        OCPI::OS::uint32_t stid, 
                        OCPI::OS::uint32_t tp, 
                        OCPI::OS::uint32_t ttid, 
                        bool broadcast=false,
                        TransferType tt = SOURCE);


    protected:

      /**********************************
       * Init
       *********************************/
      void init( OCPI::DataTransport::PortSet* source, OCPI::DataTransport::PortSet* target, bool whole_ss );

      // Next target temporal id
      OCPI::OS::int32_t        m_nextTid;
      OCPI::OS::int32_t        m_FillQPtr;
      OCPI::OS::int32_t        m_EmptyQPtr;                        

      // Our port
      OCPI::DataTransport::Port* m_port;

      // Our source port set
      OCPI::DataTransport::PortSet* m_source;

      // Our target port set
      OCPI::DataTransport::PortSet* m_target;

      // Is this a whole source set ?
      bool m_wholeSourceSet;

      // List of transfer templates     source port          source buf tid         target port        target buf tid     broadcast   target/source
      OcpiTransferTemplate* m_templates[MAX_SOURCE_PORTS] [MAX_BUFFERS] [MAX_TARGET_PORTS] [MAX_BUFFERS] [2]  [2];

    };



    /**********************************
     ****
     * inline declarations
     ****
     *********************************/

    /******************************
     * This method is used to determine if a transfer can be started while
     * a previous transfer is queued.
     *********************************/
    inline bool TransferController::canTransferBufferWhileOthersAreQueued(){return false;}

    /**********************************
     * This method determines if we have the source barrier token
     *********************************/
    inline bool TransferController::haveSourceBarrierToken(SourceBuffer* ){return true;}

    /**********************************
     * Add a template
     *********************************/
    inline void TransferController::addTemplate( 
                                                OcpiTransferTemplate* temp, OCPI::OS::uint32_t sp, 
                                                OCPI::OS::uint32_t stid, 
                                                OCPI::OS::uint32_t tp, 
                                                OCPI::OS::uint32_t ttid, 
                                                bool bcast,TransferType tt )
      {
        m_templates[sp][stid][tp][ttid][bcast?1:0][tt] = temp;
      }


    /**********************************
     * Get/Set transfer template
     *********************************/
    inline OcpiTransferTemplate* TransferController::getTemplate(
                                                                OCPI::OS::uint32_t sp, 
                                                                OCPI::OS::uint32_t stid, 
                                                                OCPI::OS::uint32_t tp, 
                                                                OCPI::OS::uint32_t ttid, 
                                                                bool bcast,
                                                                TransferType tt)
      {return m_templates[sp][stid][tp][ttid][bcast?1:0][tt];}




    /*
     *  This controller is used for the floowing patterns
     *
     *     WP/PW
     */
    class TransferController1 : public TransferController
    {
    public:

      TransferController1(){};

      TransferController1( OCPI::DataTransport::PortSet* source, OCPI::DataTransport::PortSet* target, bool whole_ss );

      virtual TransferController* createController( 
                                                   OCPI::DataTransport::PortSet* source, 
                                                   OCPI::DataTransport::PortSet* target,
                                                   bool whole_source_set);

  
      /**********************************
       * This method determines if we can produce from the indicated buffer
       *********************************/
      virtual bool canProduce( 
                              Buffer* buffer                        // InOut - Buffer to produce from
                              );

      /**********************************
       * This initiates a data transfer from the source buffer.  If the transfer can take place, 
       * it will be initiated, if not it will be queued in the circuit.
       *********************************/
      virtual int produce( Buffer* buffer, bool bcast=false );


      /**********************************
       * This marks the target buffer as "Empty" and informs all interested sources that
       * the target is now available.
       *********************************/
      virtual Buffer*  consume( Buffer* buffer );


    protected:

    };


    /*
     *  This controller is used for the floowing patterns
     *
     *     WP/S(lb)W
     */
    class TransferController2 : public TransferController
    {
    public:

      TransferController2(){};

      TransferController2( 
                          OCPI::DataTransport::PortSet* source, 
                          OCPI::DataTransport::PortSet* target,
                          bool whole_source_set);


      virtual TransferController* 
        createController( OCPI::DataTransport::PortSet* source, OCPI::DataTransport::
                          PortSet* target, bool whole_source_set);

 
      /**********************************
       * This method determines if we can produce from the indicated buffer
       *********************************/
      virtual bool canProduce( 
                              Buffer* buffer                        // InOut - Buffer to produce from
                              );

      /**********************************
       * This initiates a data transfer from the source buffer.  If the transfer can take place, 
       * it will be initiated, if not it will be queued in the circuit.
       *********************************/
      virtual int produce( Buffer* buffer, bool bcast=false );

      /**********************************
       * This marks the target buffer as "Empty" and informs all interested sources that
       * the target is now available.
       *********************************/
      virtual Buffer*  consume( Buffer* buffer );


      /**********************************
       * This method gets the next available buffer from the specified target port
       *********************************/
      virtual Buffer* getNextTargetBuffer( 
                                               OCPI::DataTransport::Port* target_port                        // In - Source port
                                               );

    private:

      // Port to produce to
      OCPI::DataTransport::Port *m_targetPort;

    };


    /*
     *  This controller is used for the floowing patterns
     *
     *     WS(rr)/S(lb)W
     */
    class TransferController3 : public TransferController2
    {
    public:

      TransferController3(){};

      TransferController3( 
                          OCPI::DataTransport::PortSet* source, 
                          OCPI::DataTransport::PortSet* target,
                          bool whole_source_set);

      virtual TransferController* createController( 
                                                   OCPI::DataTransport::PortSet* source, 
                                                   OCPI::DataTransport::PortSet* target,
                                                   bool whole_source_set);

 
      /******************************
       * This method is used to determine if a transfer can be started while
       * a previous transfer is queued.
       *********************************/
      virtual bool canTransferBufferWhileOthersAreQueued(){return false;}

      /**********************************
       * This method determines if we have the source barrier token
       *********************************/
      virtual bool haveSourceBarrierToken( SourceBuffer* src_buf );


    private:


    };



    /**********************************
     ****
     * inline declarations
     ****
     *********************************/

    inline TransferController3::TransferController3( 
                                                    OCPI::DataTransport::PortSet* source, 
                                                    OCPI::DataTransport::PortSet* target,
                                                    bool whole_source_set)
      :TransferController2(source,target,whole_source_set){}


      inline TransferController* 
        TransferController3::createController( 
                                              OCPI::DataTransport::PortSet* source, 
                                              OCPI::DataTransport::PortSet* target,
                                              bool wss ){return new TransferController3(source,target,wss);}



      /*
       *  
       *   Controller pattern 4
       *    
       */


      /*
       *  This controller is used for the following patterns
       *
       *     WP/P(Parts)
       */
      class TransferController4 : public TransferController1
      {
      public:

        TransferController4(){};

        TransferController4( OCPI::DataTransport::PortSet* source, OCPI::DataTransport::PortSet* target, bool whole_ss );


        virtual TransferController* createController( 
                                                     OCPI::DataTransport::PortSet* source, 
                                                     OCPI::DataTransport::PortSet* target,
                                                     bool whole_source_set);

        /**********************************
         * This method determines if we can produce from the indicated buffer
         *********************************/
        virtual bool canProduce( 
                                Buffer* buffer                        // InOut - Buffer to produce from
                                );

        /**********************************
         * This initiates a data transfer from the source buffer.  If the transfer can take place, 
         * it will be initiated, if not it will be queued in the circuit.
         *********************************/
        virtual int produce( Buffer* buffer, bool bcast=false );


        /**********************************
         * This method gets the next available buffer from the specified target port
         *********************************/
        virtual Buffer* getNextTargetBuffer( 
                                                 OCPI::DataTransport::Port* target_port                        // In - Source port
                                                 );


      protected:


      };
  }



}

#endif

