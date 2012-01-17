/****************************************************************************

Copyright 2008, Virginia Polytechnic Institute and State University

This file is part of the OSSIE Core Framework.

OSSIE Core Framework is free software; you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

OSSIE Core Framework is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Lesser GNU General Public License for more details.

You should have received a copy of the Lesser GNU General Public License
along with OSSIE Core Framework; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

****************************************************************************/

#ifndef APPLICATION_H
#define APPLICATION_H

#include <vector>
#include <string>

#include "cf.h"

#include "ossiecf.h"
#include "ossieSupport.h"
#include "Resource_impl.h"
#include "SADParser.h"
#include "SPDParser.h"
#include "PRFParser.h"
#include "SPDImplementation.h"

typedef
CF::Application::ComponentElementSequence
ELEM_SEQ;

typedef
CF::Application::ComponentProcessIdSequence
PROC_ID_SEQ;

class OSSIECF_API
        Application_impl:
        public
        virtual
        POA_CF::Application,
        public
        Resource_impl
{
protected:
    typedef CF::DeviceAssignmentSequence DEV_SEQ;

    DEV_SEQ *
    appComponentDevices;
    ELEM_SEQ *
    appComponentImplementations;
    ELEM_SEQ *
    appComponentNamingContexts;
    PROC_ID_SEQ *
    appComponentProcessIds;

//      ResourceFactory_impl*   rscFactory;
    SADParser *
    sadParser;
    CF::Resource_ptr
    assemblyController;


public:
    Application_impl (const char *_id, const char *_name,
                      const char *_profile, DEV_SEQ * _devSequence,
                      ELEM_SEQ * _implSequence,
                      ELEM_SEQ * _namingCtxSequence,
                      PROC_ID_SEQ * _procIdSequence,
                      std::vector <ossieSupport::ConnectionInfo *> _connectionData,
                      CF::Resource_ptr _assemblyController, CF::DomainManager::ApplicationSequence *_appseq);

    ~Application_impl ();

    void
    start ()
    throw (CF::Resource::StartError, CORBA::SystemException);
    void
    stop ()
    throw (CF::Resource::StopError, CORBA::SystemException);
    void
    initialize ()
    throw (CF::LifeCycle::InitializeError, CORBA::SystemException);
    void
    releaseObject ()
    throw (CF::LifeCycle::ReleaseError, CORBA::SystemException);
    CORBA::Object_ptr
    getPort (const char *)
    throw (CORBA::SystemException, CF::PortSupplier::UnknownPort);
    void
    runTest (CORBA::ULong, CF::Properties&)
    throw (CORBA::SystemException, CF::UnknownProperties, CF::TestableObject::UnknownTest);
    char *profile () throw (CORBA::SystemException);
    char *name () throw (CORBA::SystemException);
    DEV_SEQ *
    componentDevices ()
    throw (CORBA::SystemException);
    ELEM_SEQ *
    componentImplementations ()
    throw (CORBA::SystemException);
    ELEM_SEQ *
    componentNamingContexts ()
    throw (CORBA::SystemException);
    PROC_ID_SEQ *
    componentProcessIds ()
    throw (CORBA::SystemException);

private:
    Application_impl (); // No default constructor
    Application_impl(Application_impl &); // No copying


    std::string sadProfile;
    std::string appName;
    std::vector <ossieSupport::ConnectionInfo *> connectionData;
    ossieSupport::ORB *_orb;

    CF::DomainManager::ApplicationSequence *appseq;

};
#endif
