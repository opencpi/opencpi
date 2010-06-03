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
 *   This file contains the implementation for the CPI transfer template class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#include <DtHandshakeControl.h>
#include <CpiTransferTemplate.h>
#include <CpiPortSet.h>
#include <CpiOsAssert.h>

using namespace CPI::DataTransport;
using namespace DataTransfer;


/**********************************
 * init 
 *********************************/
void 
CpiTransferTemplate::
init()
{
  n_transfers = 0;
  m_zCopy = NULL;
  m_maxSequence = 0;
}


/**********************************
 * Constructors
 *********************************/
CpiTransferTemplate::
CpiTransferTemplate(CPI::OS::uint32_t id)
  :m_id(id)
{
  // Perform initialization
  init();
}


/**********************************
 * Destructor
 *********************************/
CpiTransferTemplate::
~CpiTransferTemplate()
{
  if ( m_zCopy )
    delete m_zCopy;

  for ( CPI::OS::uint32_t n=0; n<n_transfers; n++ ) {
    delete m_xferReq[n];
  }
}


/**********************************
 * Make sure we dont already have this pair
 *********************************/
bool 
CpiTransferTemplate::
isDuplicate(  OutputBuffer* output, InputBuffer* input )
{
  if ( m_zCopy ) {
    ZCopy* z = m_zCopy;
    while( z ) {
      if ( (z->output == output ) && ( z->input == input) ) {
        return true;
      }
      z = z->next;
    }
  }
  return false;
}


/**********************************
 * Add a xero copy transfer request
 *********************************/
void 
CpiTransferTemplate::
addZeroCopyTransfer( 
                    OutputBuffer* output, 
                    InputBuffer* input )
{

#ifndef NDEBUG
  printf("addZeroCopyTransfer, adding Zero copy transfer, output = %p, input = %p\n", output, input);
#endif

  if ( isDuplicate( output, input) ) {
    return;
  }

  ZCopy * zc = new ZCopy(output,input);
  if ( ! m_zCopy ) {
    m_zCopy = zc;
  }
  else {
    m_zCopy->add( zc );
  }

}


/**********************************
 * Is this transfer complete
 *********************************/
bool 
CpiTransferTemplate::
isComplete()
{
  // If we have any gated transfers pending, we are not done
  if ( m_sequence < m_maxSequence ) {
    return false;
  }

  // Make sure all of our transfers are complete
  for ( CPI::OS::uint32_t n=0; n<n_transfers; n++ ) {
    if ( m_xferReq[n]->getStatus() != 0 ) {
      return false;
    }
  }

  // now make sure all of the gated transfers are complete
  CPI::OS::int32_t n_pending = get_nentries(&m_gatedTransfersPending);

#ifndef NDEBUG
  printf("There are %d gated transfers\n", n_pending );
#endif

  for (CPI::OS::int32_t i=0; i < n_pending; i++) {
    CpiTransferTemplate* temp = static_cast<CpiTransferTemplate*>(get_entry(&m_gatedTransfersPending, i));
    if ( temp->isComplete() ) {
      remove_from_list( &m_gatedTransfersPending, temp );
      n_pending = get_nentries(&m_gatedTransfersPending);
      i = 0;
    }
    else {
      return false;
    }
  }

  return true;
}

CPI::OS::uint32_t 
CpiTransferTemplate::
produceGated( CPI::OS::uint32_t port_id, CPI::OS::uint32_t tid  )
{

  // See if we have any other transfers of this type left
#ifndef NDEBUG
  printf("m_sequence = %d\n", m_sequence );
#endif

  CpiTransferTemplate* temp = getNextGatedTransfer(port_id,tid);

  if ( temp ) {
    temp->produce();
  }
  else {

#ifndef NDEBUG
    printf("ERROR !!! CpiTransferTemplate::produceGated got a NULL template, p = %d, t = %d\n",
           port_id, tid);
#endif

    cpiAssert(0);
  }

  // Add the template to our list
  insert_to_list(&m_gatedTransfersPending, temp, 64, 8);
        
  return m_maxSequence - m_sequence;

}



/**********************************
 * Presets values into the output meta-data prior to kicking off
 * a transfer
 *********************************/
void 
CpiTransferTemplate::
presetMetaData( 
               volatile BufferMetaData* data, 
               CPI::OS::uint32_t length,
               bool end_of_whole,
               CPI::OS::uint32_t parts_per_whole,                
               CPI::OS::uint32_t sequence)                                
{

  CPI::OS::uint32_t seq_inc=0;

  PresetMetaData *pmd = new PresetMetaData();
  pmd->ptr = data;
  pmd->length = length;
  pmd->endOfWhole = end_of_whole ? 1 : 0;
  pmd->nPartsPerWhole = parts_per_whole;
  pmd->sequence = sequence + seq_inc;
  seq_inc++;

  // Add the structure to our list
  insert_to_list(&m_PresetMetaData, pmd, 64, 8);

}


/**********************************
 * Presets values into the output meta-data prior to kicking off
 * a transfer
 *********************************/
void 
CpiTransferTemplate::
presetMetaData()
{
  CPI::OS::int32_t n_pending = get_nentries(&m_PresetMetaData);
  for (CPI::OS::int32_t i=0; i < n_pending; i++) {
    PresetMetaData *pmd = static_cast<PresetMetaData*>(get_entry(&m_PresetMetaData, i));
    pmd->ptr->cpiMetaDataWord.length = pmd->length;
    pmd->ptr->endOfWhole         = pmd->endOfWhole;
    pmd->ptr->nPartsPerWhole     = pmd->nPartsPerWhole;
    pmd->ptr->partsSequence      = pmd->sequence;
  }
           
}





/**********************************
 * start the transfer
 *********************************/
void 
CpiTransferTemplate::
produce()
{
  // start the transfers now
#ifndef NDEBUG
  printf("Producing %d transfers\n", n_transfers);
#endif

  // At this point we need to pre-set the meta-data if needed
  presetMetaData();

  // If we have a list of attached buffers, it means that we need to 
  // inform the local buffers that there is zero copy data available
  if ( m_zCopy ) {
    ZCopy* z = m_zCopy;
    while( z ) {
#ifndef NDEBUG
      printf("Attaching %p to %p\n", z->output, z->input);
#endif
      z->input->attachZeroCopy( z->output );
      z = z->next;
    }
  }

  // Remote transfers
  for ( CPI::OS::uint32_t n=0; n<n_transfers; n++ ) {
    m_xferReq[n]->start();
  }

  // Now increment our gated transfer control
  m_sequence = 0;

}



/**********************************
 * start the transfer
 *********************************/
Buffer* 
CpiTransferTemplate::
consume()
{
  Buffer* rb=NULL;

#ifndef NDEBUG
  printf("Consuming %d transfers\n", n_transfers);
#endif

  // If we have a list of attached buffers, it means that we are now
  // done with the local zero copy buffers and need to mark them 
  // as available
  if ( m_zCopy ) {
    ZCopy* z = m_zCopy;
    while( z ) {
#ifndef NDEBUG
      printf("Detaching %p to %p\n", z->output, z->input);
#endif
      rb = z->input->detachZeroCopy();
      z = z->next;
    }
  }

  // Remote transfers
  for ( CPI::OS::uint32_t n=0; n<n_transfers; n++ ) {
    m_xferReq[n]->start();
  }

  return rb;

}

/**********************************
 * Start the input reply transfer
 *********************************/
void CpiTransferTemplate::modify( CPI::OS::uint32_t new_off[], CPI::OS::uint32_t old_off[] )
{
  for ( CPI::OS::uint32_t n=0; n<n_transfers; n++ ) {
    m_xferReq[n]->modify( new_off, old_off );
  }
}

/**********************************
 * Is this transfer pending
 *********************************/
bool CpiTransferTemplate::isPending()
{
  return false;
}


void CpiTransferTemplate::ZCopy::add( ZCopy * zc )
{
  ZCopy* t = this;
  while ( t->next ) {
    t = t->next;
  }
  t->next = zc;
}

