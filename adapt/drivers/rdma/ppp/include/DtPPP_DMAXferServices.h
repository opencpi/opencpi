/*
 * Abstact:
 *   This file contains the PPP interface for XferServices.
 *
 *   
 *
 *   01/03/08 - John Miller   
 *   Initial Version.
 *
 *
 */

#ifndef CPI_PPP_DMAXFER_SERVICES_H_
#define CPI_PPP_DMAXFER_SERVICES_H_

#include <DtTransferInterface.h>
#include <DtHandshakeControl.h>

namespace DataTransfer {

  class PPPDMAXferRequest : public XferRequest
  {

    // Public methods available to clients
  public:
    void init (Creator cr, Flags flags, 
	       unsigned long srcoffs, Shape *psrcshape, 
	       unsigned long dstoffs, Shape *pdstshape, unsigned long length)
    {
      m_creator = cr;
      m_flags = flags;
      m_srcoffset = srcoffs;
      m_dstoffset = dstoffs;
      m_length = length;
      m_thandle = 0;
      memset (&m_srcshape, 0, sizeof (m_srcshape));
      if (psrcshape)
	{
	  memcpy (&m_srcshape, psrcshape, sizeof (m_srcshape));
	}
      memset (&m_dstshape, 0, sizeof (m_dstshape));
      if (pdstshape)
	{
	  memcpy (&m_dstshape, pdstshape, sizeof (m_dstshape));
	}
    }

    // Queue data transfer request
    long start ()
    {
      return xfer_start (m_thandle, 0);
    }

    // Get Information about a Data Transfer Request
    long getStatus ()
    {
      return xfer_get_status (m_thandle);
    }

    // Destructor - implementation at end of file
    virtual ~PPPXferRequest ();


    // GetFlags - The flags that were used to create the request
    Flags getFlags () { return m_flags; }

    // GetSourceOffset - The source memory offset
    unsigned long getSourceOffset () { return m_srcoffset; }

    // GetSourceShape - The source shape
    Shape getSourceShape () { return m_srcshape; }

    // GetDestinationOffset - The destination memory offset
    unsigned long getDestinationOffset () { return m_dstoffset; }


    // GetDestinationShape - The destination shape
    Shape getDestinationShape () { return m_dstshape; }

    // GetLength - The length of the request in bytes
    unsigned long getLength () { return m_length; }



    // Data members accessible from this/derived class
  protected:
    Flags			m_flags;	// Flags used during creation
    unsigned long		m_srcoffset;	// The source memory offset
    Shape			m_srcshape;	// The source shape
    unsigned long		m_dstoffset;	// The destination memory offset
    Shape			m_dstshape;	// The destination memory shape
    unsigned long		m_length;	// The length of the request in bytes

    // Public data members
  public:
    XF_transfer		m_thandle;	// Transfer handle returned by xfer_xxx etal
  };


  class PPPDMAXferServices : public XferServices
  {

    // So the destructor can invoke "remove"
    friend PPPDMAXferRequest::~PPPDMAXferRequest ();

  public:
    // Create tranfer services template
    long createTemplate (SmemServices* p1, SmemServices* p2)
    {

      m_txRequest = NULL;
      m_sourceSmb = p1;
      m_targetSmb = p2;
      p1->map(0,sizeof(ContainerComms::MailBox),(void**)&m_sourceComms);
      p2->map(0,sizeof(ContainerComms::MailBox),(void**)&m_targetComms);


      // Invoke original code, saving the returned template reference.
      return xfer_create (p1, p2, 0, &m_xftemplate);
    }

    // Create a transfer request
    long copy (unsigned long srcoffs, unsigned long dstoffs, unsigned long nbytes, XferRequest::Flags flags, XferRequest* &preq)
    {
      // Create a transfer request instance and save in list
      PPPXferRequest* pXferReq = new PPPXferRequest ();
      pXferReq->init (XferRequest::Copy, flags, srcoffs, 0, dstoffs, 0, nbytes);
      add (pXferReq);

      // Begin exception block
      long retVal = 0;
      try
	{
	  // map flags
	  long newflags = 0;
	  if (flags & XferRequest::FirstTransfer) newflags |= XFER_FIRST;
	  if (flags & XferRequest::LastTransfer) newflags |= XFER_LAST;

	  // Invoke original code.
	  retVal = xfer_copy (m_xftemplate, srcoffs, dstoffs, nbytes, newflags, &pXferReq->m_thandle);
	  if (retVal)
	    {
	      throw;
	    }
	}
      catch (...)
	{
	  remove (pXferReq);
	  delete pXferReq;
	  pXferReq = 0;
	}
      preq = pXferReq;
      return retVal;
    }

    // Create a 2-dimensional transfer request
    long copy2D (unsigned long srcoffs, Shape* psrc, unsigned long dstoffs, Shape* pdst, XferRequest* &preq)
    {
      // Create a transfer request instance and save in list
      PPPXferRequest* pXferReq = new PPPXferRequest ();
      pXferReq->init (XferRequest::Copy2D, (XferRequest::Flags)0, srcoffs, psrc, dstoffs, pdst, 0);
      add (pXferReq);

      // Begin exception block
      long retVal = 0;
      try
	{
	  // Invoke original code.
	  // We simple cast "XferServices::Shape" to "EP_shape" since they must have the
	  // exact same definitions. We don't specify any flags (they weren't used in the original).
	  //			retVal = xfer_copy_2d (m_xftemplate, srcoffs, (Shape*)psrc, dstoffs, (Shape*)pdst, 0, &pXferReq->m_thandle);
	  if (retVal)
	    {
	      throw;
	    }
	}
      catch (...)
	{
	  remove (pXferReq);
	  delete pXferReq;
	  pXferReq = 0;
	}
      preq = pXferReq;
      return retVal;
    }

    // Group data transfer requests
    long group (XferRequest* preqs[], XferRequest* &preq)
    {
      // Create a transfer request instance and save in list
      PPPXferRequest* pXferReq = new PPPXferRequest ();
      pXferReq->init (XferRequest::Group, (XferRequest::Flags)0, 0, 0, 0, 0, 0);
      add (pXferReq);

      // Begin exception handler
      long retVal = 0;
      XF_transfer* handles = 0;
      try
	{
	  // Make a list of existing XF_transfer from the XferRequest* [] argument.
	  int numHandles = 0;
	  while (preqs[numHandles]) { numHandles++;}
	  handles = new XF_transfer [numHandles + 1] ;
	  for (int i = 0; i < numHandles; i++)
	    {
	      handles[i] = ((NtXferRequest*)preqs[i])->m_thandle;
	    }
	  handles[numHandles] = 0;

	  // Invoke original code.
	  retVal = xfer_group (handles, 0, &pXferReq->m_thandle);
	  if (retVal)
	    {
	      throw;
	    }
	}
      catch (...)
	{
	  remove (pXferReq);
	  delete pXferReq;
	  pXferReq = 0;
	  delete handles;
	}
      preq = pXferReq;
      return retVal;
    }

    // Release a transfer request
    long release (XferRequest* preq)
    {
      // Delete of request insures list removal.
      delete preq;
      return 0;
    }

    // Destructor
    virtual ~PPPDMAXferServices ()
      {
	// Release all transfer requests
	releaseAll ();

	// Invoke destroy without flags.
	xfer_destroy (m_xftemplate, 0);

      }

    // Derived class helpers
  protected:

    // our mailbox slot
    int m_slot;

    // add a new transfer request instance to the list
    void add (PPPXferRequest* pXferReq);

    // remove a specified transfer request instance from the list
    static void remove (PPPXferRequest* pXferReq);

    // remove all transfer request instances from the list for "this"
    void releaseAll ();

  private:
#ifdef map_IN_CLASS_DEF // Should be in class definition but cause link errors.
    // A map of data transfer requests to the instance that created it
    // to support rundow. Note that list access is not thread-safe.
    static map<PPPXferRequest*, PPPXferServices*>	m_map;
#endif
    // The handle returned by xfer_create
    XF_template	m_xftemplate;



    // Our transfer request
    XferRequest* m_txRequest;
    SmemServices* m_sourceSmb;
    SmemServices* m_targetSmb;
    ContainerComms*  m_sourceComms;
    ContainerComms*  m_targetComms;

  };

#ifndef map_IN_CLASS_DEF // Should be in class definition but cause link errors.
  // A map of data transfer requests to the instance that created it
  // to support rundow. Note that list access is not thread-safe.
#define m_map SceMCOEXfermap
  static map<PPPXferRequest*, PPPXferServices*>	SceMCOEXfermap;
#endif



  // add a new transfer request instance to the list
  void PPPDMAXferServices::add (PPPDMAXferRequest* pXferReq)
  {
    m_map[pXferReq] = this;
  }



  // remove a specified transfer request instance from the list
  void PPPDMAXferServices::remove (PPPDMAXferRequest* pXferReq)
  {
    // Find it and erase.
    map<PPPDMAXferRequest*,PPPDMAXferServices*>::iterator pos = m_map.find (pXferReq);
    if (pos != m_map.end ())
      {
	m_map.erase (pos);
      }
  }

  // remove all transfer request instances from the list for "this"
  void PPPDMAXferServices::releaseAll ()
  {
    // Find and erase all transfer requests for "this".
    map<PPPDMAXferRequest*, PPPDMAXferServices*>::iterator it;
    for (it = m_map.begin (); it != m_map.end (); it++)
      {
	if (it->second == this)
	  {
	    m_map.erase (it);
	  }
      }
  }


  // NtXferRequest destructor implementation
  PPPDMAXferRequest::~PPPDMAXferRequest ()
    {
      // remove self from the map and release xfer handle.
      PPPDMAXferServices::remove (this);
      if (m_thandle)
	{
	  (void)xfer_release (m_thandle, 0);
	}
    }


};

#endif

