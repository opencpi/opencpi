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

#include <string>
#include <cstring>
#include <CpiOsAssert.h>
#include <CpiOsMisc.h>
#include <CpiOsMutex.h>
#include <CpiUtilAutoMutex.h>
#include <CosNaming.h>
#include <CosNaming_s.h>
#include "CpiStringifyCorbaException.h"
#include "CpiCORBAUtilNameServiceWait.h"

/*
 * ----------------------------------------------------------------------
 * A partial implementation of the CosNaming::NamingContext interface
 * that allows a client to bind an object.  Only the bind() and rebind()
 * operations are implemented.
 * ----------------------------------------------------------------------
 */

namespace {

  class MyNamingContext_impl :
    public CPI::CORBAUtil::WaitForNameServiceBinding::NamingContextBase,
    public POA_CosNaming::NamingContext
  {
  public:
    MyNamingContext_impl (const CosNaming::Name & expectedBinding)
      throw ();
    ~MyNamingContext_impl ()
      throw ();

    /*
     * Naming Context API.  Only bind() and rebind() are implemented, but
     * we have to provide stub implementations for all.
     */

    void bind (const CosNaming::Name & n, CORBA::Object_ptr obj)
      throw (CosNaming::NamingContext::NotFound,
             CosNaming::NamingContext::CannotProceed,
             CosNaming::NamingContext::InvalidName,
             CosNaming::NamingContext::AlreadyBound,
             CORBA::SystemException);

    void rebind (const CosNaming::Name & n, CORBA::Object_ptr obj)
      throw (CosNaming::NamingContext::NotFound,
             CosNaming::NamingContext::CannotProceed,
             CosNaming::NamingContext::InvalidName,
             CORBA::SystemException);

    void bind_context (const CosNaming::Name & n, CosNaming::NamingContext_ptr nc)
      throw (CosNaming::NamingContext::NotFound,
             CosNaming::NamingContext::CannotProceed,
             CosNaming::NamingContext::InvalidName,
             CosNaming::NamingContext::AlreadyBound,
             CORBA::SystemException);

    void rebind_context (const CosNaming::Name & n, CosNaming::NamingContext_ptr nc)
      throw (CosNaming::NamingContext::NotFound,
             CosNaming::NamingContext::CannotProceed,
             CosNaming::NamingContext::InvalidName,
             CORBA::SystemException);

    CORBA::Object_ptr resolve (const CosNaming::Name & n)
      throw (CosNaming::NamingContext::NotFound,
             CosNaming::NamingContext::CannotProceed,
             CosNaming::NamingContext::InvalidName,
             CORBA::SystemException);

    void unbind (const CosNaming::Name & n)
      throw (CosNaming::NamingContext::NotFound,
             CosNaming::NamingContext::CannotProceed,
             CosNaming::NamingContext::InvalidName,
             CORBA::SystemException);

    CosNaming::NamingContext_ptr new_context ()
      throw (CORBA::SystemException);

    CosNaming::NamingContext_ptr bind_new_context (const CosNaming::Name & n)
      throw (CosNaming::NamingContext::NotFound,
             CosNaming::NamingContext::AlreadyBound,
             CosNaming::NamingContext::CannotProceed,
             CosNaming::NamingContext::InvalidName,
             CORBA::SystemException);

    void destroy ()
      throw (CosNaming::NamingContext::NotEmpty,
             CORBA::SystemException);

    void list (CORBA::ULong how_many,
               CosNaming::BindingList_out bl,
               CosNaming::BindingIterator_out bi)
      throw (CORBA::SystemException);
  };

  MyNamingContext_impl::
  MyNamingContext_impl (const CosNaming::Name & expectedBinding)
    throw ()
    : NamingContextBase (expectedBinding)
  {
  }

  MyNamingContext_impl::
  ~MyNamingContext_impl ()
    throw ()
  {
  }

  void
  MyNamingContext_impl::
  bind (const CosNaming::Name & n, CORBA::Object_ptr obj)
    throw (CosNaming::NamingContext::NotFound,
           CosNaming::NamingContext::CannotProceed,
           CosNaming::NamingContext::InvalidName,
           CosNaming::NamingContext::AlreadyBound,
           CORBA::SystemException)
  {
    CPI::Util::AutoMutex lock (m_mutex);

    if (namesAreEqual (m_expectedBinding, n)) {
      if (m_haveBinding) {
        throw CosNaming::NamingContext::AlreadyBound ();
      }

      m_haveBinding = true;
      m_boundObject = CORBA::Object::_duplicate (obj);
    }
    else {
      throw CosNaming::NamingContext::InvalidName ();
    }
  }

  void
  MyNamingContext_impl::
  rebind (const CosNaming::Name & n, CORBA::Object_ptr obj)
    throw (CosNaming::NamingContext::NotFound,
           CosNaming::NamingContext::CannotProceed,
           CosNaming::NamingContext::InvalidName,
           CORBA::SystemException)
  {
    CPI::Util::AutoMutex lock (m_mutex);

    if (namesAreEqual (m_expectedBinding, n)) {
      m_haveBinding = true;
      m_boundObject = CORBA::Object::_duplicate (obj);
    }
    else {
      throw CosNaming::NamingContext::InvalidName ();
    }
  }

  void
  MyNamingContext_impl::
  bind_context (const CosNaming::Name &, CosNaming::NamingContext_ptr)
    throw (CosNaming::NamingContext::NotFound,
           CosNaming::NamingContext::CannotProceed,
           CosNaming::NamingContext::InvalidName,
           CosNaming::NamingContext::AlreadyBound,
           CORBA::SystemException)
  {
    throw CORBA::NO_IMPLEMENT ();
  }

  void
  MyNamingContext_impl::
  rebind_context (const CosNaming::Name &, CosNaming::NamingContext_ptr)
    throw (CosNaming::NamingContext::NotFound,
           CosNaming::NamingContext::CannotProceed,
           CosNaming::NamingContext::InvalidName,
           CORBA::SystemException)
  {
    throw CORBA::NO_IMPLEMENT ();
  }

  CORBA::Object_ptr
  MyNamingContext_impl::
  resolve (const CosNaming::Name & n)
    throw (CosNaming::NamingContext::NotFound,
           CosNaming::NamingContext::CannotProceed,
           CosNaming::NamingContext::InvalidName,
           CORBA::SystemException)
  {
    CPI::Util::AutoMutex lock (m_mutex);

    if (namesAreEqual (m_expectedBinding, n)) {
      if (m_haveBinding) {
        return CORBA::Object::_duplicate (m_boundObject);
      }
      else {
        throw CosNaming::NamingContext::NotFound (CosNaming::NamingContext::missing_node, n);
      }
    }
    else {
      throw CosNaming::NamingContext::InvalidName ();
    }
  }

  void
  MyNamingContext_impl::
  unbind (const CosNaming::Name & n)
    throw (CosNaming::NamingContext::NotFound,
           CosNaming::NamingContext::CannotProceed,
           CosNaming::NamingContext::InvalidName,
           CORBA::SystemException)
  {
    CPI::Util::AutoMutex lock (m_mutex);

    if (namesAreEqual (m_expectedBinding, n)) {
      if (m_haveBinding) {
        m_haveBinding = false;
      }
      else {
        throw CosNaming::NamingContext::NotFound (CosNaming::NamingContext::missing_node, n);
      }
    }
    else {
      throw CosNaming::NamingContext::InvalidName ();
    }
  }

  CosNaming::NamingContext_ptr
  MyNamingContext_impl::
  new_context ()
    throw (CORBA::SystemException)
  {
    throw CORBA::NO_IMPLEMENT ();
  }

  CosNaming::NamingContext_ptr
  MyNamingContext_impl::
  bind_new_context (const CosNaming::Name &)
    throw (CosNaming::NamingContext::NotFound,
           CosNaming::NamingContext::AlreadyBound,
           CosNaming::NamingContext::CannotProceed,
           CosNaming::NamingContext::InvalidName,
           CORBA::SystemException)
  {
    throw CORBA::NO_IMPLEMENT ();
  }

  void
  MyNamingContext_impl::
  destroy ()
    throw (CosNaming::NamingContext::NotEmpty,
           CORBA::SystemException)
  {
  }

  void
  MyNamingContext_impl::
  list (CORBA::ULong,
        CosNaming::BindingList_out,
        CosNaming::BindingIterator_out)
    throw (CORBA::SystemException)
  {
    throw CORBA::NO_IMPLEMENT ();
  }

}

/*
 * ----------------------------------------------------------------------
 * Implementation of the NamingContextBase base class.
 * ----------------------------------------------------------------------
 */

CPI::CORBAUtil::WaitForNameServiceBinding::NamingContextBase::
NamingContextBase (const CosNaming::Name & expectedBinding)
  throw ()
  : m_haveBinding (false),
    m_expectedBinding (expectedBinding)
{
  cpiAssert (m_expectedBinding.length());
}

CPI::CORBAUtil::WaitForNameServiceBinding::NamingContextBase::
~NamingContextBase ()
  throw ()
{
}

bool
CPI::CORBAUtil::WaitForNameServiceBinding::NamingContextBase::
haveBinding ()
  throw ()
{
  CPI::Util::AutoMutex lock (m_mutex);
  return m_haveBinding;
}

CORBA::Object_ptr
CPI::CORBAUtil::WaitForNameServiceBinding::NamingContextBase::
getBinding ()
  throw ()
{
  CPI::Util::AutoMutex lock (m_mutex);
  return CORBA::Object::_duplicate (m_boundObject);
}

bool
CPI::CORBAUtil::WaitForNameServiceBinding::NamingContextBase::
namesAreEqual (const CosNaming::Name & n1,
               const CosNaming::Name & n2)
  throw ()
{
  CORBA::ULong nl = n1.length ();

  if (nl != n2.length()) {
    return false;
  }

  for (CORBA::ULong li=0; li<nl; li++) {
    const CosNaming::NameComponent & n1c = n1[li];
    const CosNaming::NameComponent & n2c = n2[li];

    if (std::strcmp (n1c.id.in(), n2c.id.in()) != 0 ||
        std::strcmp (n1c.kind.in(), n2c.kind.in()) != 0) {
      return false;
    }
  }

  return true;
}

/*
 * ----------------------------------------------------------------------
 * Implementation of the WaitForNameServiceBinding class.
 * ----------------------------------------------------------------------
 */

CPI::CORBAUtil::WaitForNameServiceBinding::
WaitForNameServiceBinding (CORBA::ORB_ptr orb,
                           PortableServer::POA_ptr poa,
                           const CosNaming::Name & expectedBinding)
  throw (std::string)
  : m_orb (CORBA::ORB::_duplicate (orb)),
    m_poa (PortableServer::POA::_duplicate (poa))
{
  MyNamingContext_impl * mnc = new MyNamingContext_impl (expectedBinding);
  m_myNamingContext = mnc;

  try {
    /*
     * Note: this assumes that the POA has acceptable policies and that
     * its POA Manager plays along.
     */

    try {
      m_oid = m_poa->activate_object (mnc);

      try {
        CORBA::Object_var obj = m_poa->id_to_reference (m_oid);
        m_context = CosNaming::NamingContext::_narrow (obj);
        cpiAssert (!CORBA::is_nil (m_context));
      }
      catch (...) {
        m_poa->deactivate_object (m_oid);
      }
    }
    catch (...) {
      throw std::string ("Error activating Naming Context");
    }
  }
  catch (...) {
    mnc->_remove_ref ();
    throw;
  }
}

CPI::CORBAUtil::WaitForNameServiceBinding::
~WaitForNameServiceBinding ()
  throw ()
{
  try {
    m_poa->deactivate_object (m_oid);
  }
  catch (...) {
  }

  MyNamingContext_impl * mnc = dynamic_cast<MyNamingContext_impl *> (m_myNamingContext);
  cpiAssert (mnc);
  mnc->_remove_ref ();
}

CosNaming::NamingContext_ptr
CPI::CORBAUtil::WaitForNameServiceBinding::
getContext ()
  throw ()
{
  return CosNaming::NamingContext::_duplicate (m_context);
}

bool
CPI::CORBAUtil::WaitForNameServiceBinding::
waitForBinding (unsigned long timeoutInSeconds)
  throw ()
{
  /*
   * Poor-man's timeout.
   */

  bool haveBinding = false;

  while (timeoutInSeconds) {
    if ((haveBinding = m_myNamingContext->haveBinding())) {
      break;
    }

    try {
      if (m_orb->work_pending()) {
        m_orb->perform_work ();
      }
    }
    catch (...) {
      break;
    }

    CPI::OS::sleep (1000);
    timeoutInSeconds--;
  }

  if (!haveBinding) {
    haveBinding = m_myNamingContext->haveBinding ();
  }

  return haveBinding;
}

CORBA::Object_var
CPI::CORBAUtil::WaitForNameServiceBinding::
getBinding ()
  throw ()
{
  return m_myNamingContext->getBinding ();
}

