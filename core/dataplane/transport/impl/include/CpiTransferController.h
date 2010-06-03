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
 *   This file contains the CPI transfer controller interface.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef CPI_DataTransport_TransferController_H_
#define CPI_DataTransport_TransferController_H_

#include <CpiUtilMisc.h>
#include <CpiTransferTemplate.h>
#include <CpiBuffer.h>
#include <CpiOutputBuffer.h>
#include <CpiUtilRefCounter.h>

namespace CPI {

  namespace DataTransport {

    class CpiPortSet;
    class Port;

    /*****
     *  This controller is what manages the port buffers and determines what 
     ****/
    class TransferController {
    public:

      enum TransferType {
        OUTPUT,
        INPUT
      };

      /**********************************
       * Constructor
       *********************************/
      TransferController();

      /**********************************
       * Destructor
       *********************************/
      virtual ~TransferController();


      /**********************************
       * Modify
       *********************************/
      virtual void modifyOutputOffsets( Buffer* me, Buffer* new_buffer, bool reverse );

      /**********************************
       * Creates an instance of a controller
       *********************************/
      virtual TransferController* createController( 
                                                   CPI::DataTransport::PortSet* output, 
                                                   CPI::DataTransport::PortSet* input, 
                                                   bool whole_output_set)=0;


      /******************************
       * This method is used to determine if a transfer can be started while
       * a previous transfer is queued.
       *********************************/
      virtual bool canTransferBufferWhileOthersAreQueued();

      /**********************************
       * This method gets the next available buffer from the specified output port
       *********************************/
      virtual Buffer* getNextEmptyOutputBuffer( 
                                               CPI::DataTransport::Port* src_port                        // In - Output port
                                               );

      /**********************************
       * This method determines if there is an available buffer, but does not affect the
       * state of the object.
       *********************************/
      virtual bool hasEmptyOutputBuffer(
                                            CPI::DataTransport::Port* port
                                            )const;

      /**********************************
       * This method determines if there is data available, but does not affect the
       * state of the object.
       *********************************/
      virtual bool hasFullInputBuffer(
                                       CPI::DataTransport::Port* port,               
                                       InputBuffer**
                                       )const;
                                
      /**********************************
       * This is used to indicate that a buffer has been filled. Since we manage circular
       * buffers, the actual buffer is implied (next)
       *
       *********************************/
      virtual void bufferFull(
                              CPI::DataTransport::Port* port                                                
                              );
                                                
                                                
      /**********************************
       * This is used to indicate that a remote input 
       * buffer has been freed. Since we manage circular
       * buffers, the actual buffer is implied (next)
       *
       *********************************/
      virtual void freeBuffer(
                              CPI::DataTransport::Port* port                                                
                              );

      /**********************************
       * This is used to indicate that all remote input 
       * buffer has been emptied.
       *
       *********************************/
      virtual void freeAllBuffersLocal(
                                       CPI::DataTransport::Port* port                                                
                                       );


      /**********************************
       * This method gets the next available buffer from the specified input port
       *********************************/
      virtual Buffer* getNextFullInputBuffer( 
                                               CPI::DataTransport::Port* input_port                        // In - Output port
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
       * This method determines if we have the output barrier token
       *********************************/
      virtual bool haveOutputBarrierToken( OutputBuffer* src_buf );

      /**********************************
       * This initiates a data transfer from the output buffer.  If the transfer can take place, 
       * it will be initiated, if not it will be queued in the circuit.
       *********************************/
      virtual int produce( 
                          Buffer* buffer,                        // InOut - Buffer to produce from
                          bool      broadcast=false        // In    - Broadcast the buffer
                          )=0;

      /**********************************
       * This initiates a broadcastdata transfer from the output buffer.
       *********************************/
      virtual void broadCastOutput(
                                   Buffer* buffer                        // InOut - Buffer to produce from
                                   );

      /**********************************
       * This marks the input buffer as "Empty" and informs all interested outputs that
       * the input is now available.
       *********************************/
      virtual Buffer* consume( 
                              Buffer* buffer                        // InOut - Buffer to consume
                              )=0;

      /**********************************
       * This marks the input buffer as "Empty" and informs all interested outputs that
       * the input is now available.
       *********************************/
      virtual void consumeAllBuffersLocal( 
                                          CPI::DataTransport::Port* port                          
                                          );

      /**********************************
       * Add a template
       *********************************/
      void addTemplate( CpiTransferTemplate* temp, 
                        CPI::OS::uint32_t sp, 
                        CPI::OS::uint32_t stid, 
                        CPI::OS::uint32_t tp, 
                        CPI::OS::uint32_t ttid, 
                        bool broadcast=false,
                        TransferType tt = OUTPUT);


    protected:

      /**********************************
       * Init
       *********************************/
      void init( CPI::DataTransport::PortSet* output, CPI::DataTransport::PortSet* input, bool whole_ss );

      // Next input temporal id
      CPI::OS::int32_t        m_nextTid;
      CPI::OS::int32_t        m_FillQPtr;
      CPI::OS::int32_t        m_EmptyQPtr;                        

      // Our port
      CPI::DataTransport::Port* m_port;

      // Our output port set
      CPI::DataTransport::PortSet* m_output;

      // Our input port set
      CPI::DataTransport::PortSet* m_input;

      // Is this a whole output set ?
      bool m_wholeOutputSet;

      // List of transfer templates     output port          output buf tid         input port        input buf tid     broadcast   input/output

      CpiTransferTemplate* m_templates[MAX_PCONTRIBS] [MAX_BUFFERS] [MAX_PCONTRIBS] [MAX_BUFFERS] [2]  [2];

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
     * This method determines if we have the output barrier token
     *********************************/
    inline bool TransferController::haveOutputBarrierToken(OutputBuffer* ){return true;}

    /**********************************
     * Add a template
     *********************************/
    inline void TransferController::addTemplate( 
                                                CpiTransferTemplate* temp, CPI::OS::uint32_t sp, 
                                                CPI::OS::uint32_t stid, 
                                                CPI::OS::uint32_t tp, 
                                                CPI::OS::uint32_t ttid, 
                                                bool bcast,TransferType tt )
      {
        m_templates[sp][stid][tp][ttid][bcast?1:0][tt] = temp;
      }


    // Invalid controller used to ensure that unsupported transfers get caught early.
    class TransferControllerNotSupported : public TransferController
    {
    public:
      virtual ~TransferControllerNotSupported(){}
      TransferController* createController( 
                                           CPI::DataTransport::PortSet* output, 
                                           CPI::DataTransport::PortSet* input,
                                           bool whole_output_set)
      {
        cpiAssert(!"Unsupported data transfer request rejected !!\n");
        throw CPI::Util::EmbeddedException("Unsupported data transfer request rejected !!\n");
        return NULL;
      }
      bool canProduce( Buffer* buffer ){return true;}
      int produce( Buffer* buffer, bool bcast=false ){return 0;}
      Buffer*  consume( Buffer* buffer ){return NULL;}
    };


    /*
     *  This controller is used for the following patterns
     *
     *     WP/PW
     */
    class TransferController1 : public TransferController
    {
    public:
      TransferController1(){};
      virtual ~TransferController1(){};
      TransferController1( CPI::DataTransport::PortSet* output, CPI::DataTransport::PortSet* input, bool whole_ss );
      virtual TransferController* createController( 
                                                   CPI::DataTransport::PortSet* output, 
                                                   CPI::DataTransport::PortSet* input,
                                                   bool whole_output_set);
  
      /**********************************
       * This method determines if we can produce from the indicated buffer
       *********************************/
      virtual bool canProduce( Buffer* buffer );

      /**********************************
       * This initiates a data transfer from the output buffer.  If the transfer can take place, 
       * it will be initiated, if not it will be queued in the circuit.
       *********************************/
      virtual int produce( Buffer* buffer, bool bcast=false );

      /**********************************
       * This marks the input buffer as "Empty" and informs all interested outputs that
       * the input is now available.
       *********************************/
      virtual Buffer*  consume( Buffer* buffer );

    protected:

    };

    // This controller is used for pattern1 when either the output or input port(s) are  ActiveFlowControl
    class TransferController1AFCShadow : public TransferController1
    {
    public:
      TransferController1AFCShadow(){};
      virtual ~TransferController1AFCShadow(){};
      TransferController1AFCShadow( CPI::DataTransport::PortSet* output, CPI::DataTransport::PortSet* input, bool whole_ss );
      virtual TransferController* createController( 
                                                   CPI::DataTransport::PortSet* output, 
                                                   CPI::DataTransport::PortSet* input,
                                                   bool whole_output_set);

      /**********************************
       * This method gets the next available buffer from the specified output port
       *********************************/
      virtual Buffer* getNextEmptyOutputBuffer( CPI::DataTransport::Port* src_port );


      /**********************************
       * This method gets the next available buffer from the specified input port
       *********************************/
      Buffer* getNextFullInputBuffer( CPI::DataTransport::Port* input_port );


      /**********************************
       * This method determines if there is data available, but does not affect the
       * state of the object.
       *********************************/
       bool hasFullInputBuffer(
                                CPI::DataTransport::Port* port,               
                                InputBuffer**
                                )const;
  
      /**********************************
       * This method determines if we can produce from the indicated buffer
       *********************************/
      virtual bool canProduce( Buffer* buffer );

      /**********************************
       * This initiates a data transfer from the output buffer.  If the transfer can take place, 
       * it will be initiated, if not it will be queued in the circuit.
       *********************************/
      virtual int produce( Buffer* buffer, bool bcast=false );

      /**********************************
       * Modify
       *********************************/
      virtual void modifyOutputOffsets( Buffer* me, Buffer* new_buffer, bool reverse );

      /**********************************
       * This marks the input buffer as "Empty" and informs all interested outputs that
       * the input is now available.
       *********************************/
      virtual Buffer*  consume( Buffer* buffer );

    };

    /*
     *  This controller is used for the following patterns
     *
     *     WP/S(lb)W
     */
    class TransferController2 : public TransferController
    {
    public:
      TransferController2(){};
      virtual ~TransferController2(){};
      TransferController2( 
                          CPI::DataTransport::PortSet* output, 
                          CPI::DataTransport::PortSet* input,
                          bool whole_output_set);

      virtual TransferController* 
        createController( CPI::DataTransport::PortSet* output, CPI::DataTransport::
                          PortSet* input, bool whole_output_set);
 
      /**********************************
       * This method determines if we can produce from the indicated buffer
       *********************************/
      virtual bool canProduce( 
                              Buffer* buffer                        // InOut - Buffer to produce from
                              );

      /**********************************
       * This initiates a data transfer from the output buffer.  If the transfer can take place, 
       * it will be initiated, if not it will be queued in the circuit.
       *********************************/
      virtual int produce( Buffer* buffer, bool bcast=false );

      /**********************************
       * This marks the input buffer as "Empty" and informs all interested outputs that
       * the input is now available.
       *********************************/
      virtual Buffer*  consume( Buffer* buffer );


      /**********************************
       * This method gets the next available buffer from the specified input port
       *********************************/
      virtual Buffer* getNextFullInputBuffer( 
                                               CPI::DataTransport::Port* input_port                        // In - Output port
                                               );

    private:

      // Port to produce to
      CPI::DataTransport::Port *m_inputPort;

    };


    /*
     *  This controller is used for the following patterns
     *
     *     WS(rr)/S(lb)W
     */
    class TransferController3 : public TransferController2
    {
    public:
      TransferController3(){};
      virtual ~TransferController3(){};
      TransferController3( 
                          CPI::DataTransport::PortSet* output, 
                          CPI::DataTransport::PortSet* input,
                          bool whole_output_set);

      virtual TransferController* createController( 
                                                   CPI::DataTransport::PortSet* output, 
                                                   CPI::DataTransport::PortSet* input,
                                                   bool whole_output_set);
 
      /******************************
       * This method is used to determine if a transfer can be started while
       * a previous transfer is queued.
       *********************************/
      virtual bool canTransferBufferWhileOthersAreQueued(){return false;}

      /**********************************
       * This method determines if we have the output barrier token
       *********************************/
      virtual bool haveOutputBarrierToken( OutputBuffer* src_buf );


    private:


    };



    /**********************************
     ****
     * inline declarations
     ****
     *********************************/

    inline TransferController3::TransferController3( 
                                                    CPI::DataTransport::PortSet* output, 
                                                    CPI::DataTransport::PortSet* input,
                                                    bool whole_output_set)
      :TransferController2(output,input,whole_output_set){}

      inline TransferController* 
        TransferController3::createController( 
                                              CPI::DataTransport::PortSet* output, 
                                              CPI::DataTransport::PortSet* input,
                                              bool wss ){return new TransferController3(output,input,wss);}

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

        TransferController4( CPI::DataTransport::PortSet* output, CPI::DataTransport::PortSet* input, bool whole_ss );


        virtual TransferController* createController( 
                                                     CPI::DataTransport::PortSet* output, 
                                                     CPI::DataTransport::PortSet* input,
                                                     bool whole_output_set);

        /**********************************
         * This method determines if we can produce from the indicated buffer
         *********************************/
        virtual bool canProduce( 
                                Buffer* buffer                        // InOut - Buffer to produce from
                                );

        /**********************************
         * This initiates a data transfer from the output buffer.  If the transfer can take place, 
         * it will be initiated, if not it will be queued in the circuit.
         *********************************/
        virtual int produce( Buffer* buffer, bool bcast=false );


        /**********************************
         * This method gets the next available buffer from the specified input port
         *********************************/
        virtual Buffer* getNextFullInputBuffer( 
                                                 CPI::DataTransport::Port* input_port                        // In - Output port
                                                 );


      protected:


      };
  }



}

#endif

