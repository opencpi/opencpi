/*******************************************************************************

Copyright 2008, Virginia Polytechnic Institute and State University

This file is part of the OSSIE Parser.

OSSIE Parser is free software; you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

OSSIE Parser is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Lesser GNU General Public License for more details.

You should have received a copy of the Lesser GNU General Public License
along with OSSIE Parser; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include <iostream>

#ifdef HAVE_OMNIORB4_CORBA_H
#include <omniORB4/CORBA.h>
#include <omniORB4/omniURI.h>
#endif

#include "ossie/ossieSupport.h"

using namespace std;
using namespace ossieSupport;

// Initialize static class members

CORBA::ORB_var ORB::orb = CORBA::ORB::_nil();
unsigned int ORB::ref_cnt = 0;
PortableServer::POA_var ORB::poa;
PortableServer::POAManager_var ORB::pman;
CosNaming::NamingContext_var ORB::inc;

ORB::ORB()

{
    if (CORBA::is_nil(ORB::orb)) {
        int argc = 0;
        orb = CORBA::ORB_init(argc, NULL);
        init();
    }

    ref_cnt++;

}

ORB::ORB(int argc, char *argv[])

{
    if (CORBA::is_nil(ORB::orb)) {
        orb = CORBA::ORB_init(argc, argv);
        init();
    }

    ref_cnt++;

}

ORB::~ORB()

{
///\todo work on destructor
    ref_cnt--;
    cout << "ORB destructor called, re_cnt now " << ref_cnt << endl;
}


void ORB::init()

{

    try {
        CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
        poa = PortableServer::POA::_narrow(obj);
    } catch (const CORBA::Exception &e) {
        cerr << "Failed to resolve RootPOA: " ;
        CORBA::Any tmp;
        tmp <<= e;
        CORBA::TypeCode_var tc = tmp.type();
        const char *p = tc->name();
        if (*p != '\0')
            cerr << p << endl;
        else
            cerr << tc->id();
        throw;
    }

    try {
        pman = poa->the_POAManager();
        pman->activate();

    } catch (const CORBA::Exception &e) {
        cerr << "Failed to activate POA Manager: " ;
        CORBA::Any tmp;
        tmp <<= e;
        CORBA::TypeCode_var tc = tmp.type();
        const char *p = tc->name();
        if (*p != '\0')
            cerr << p << endl;
        else
            cerr << tc->id();
        throw;
    }

    try {
        CORBA::Object_var obj = orb->resolve_initial_references("NameService");
        inc = CosNaming::NamingContext::_narrow(obj);
    } catch (const CORBA::Exception &e) {
        cerr << "Failed to resolve NameService: " ;
        CORBA::Any tmp;
        tmp <<= e;
        CORBA::TypeCode_var tc = tmp.type();
        const char *p = tc->name();
        if (*p != '\0')
            cerr << p << endl;
        else
            cerr << tc->id();
        throw;
    }
}

CORBA::Object_ptr ORB::get_object_from_name(const char *name)

{
    CORBA::Object_var obj;
    CosNaming::Name_var cosName = string_to_CosName(name);

    obj = inc->resolve(cosName);

    return obj._retn();
}

CosNaming::Name_var ORB::string_to_CosName(const char *name)

{


// Most generic solution, but makes call on remote object
// Use ORB optimized solution if possible

// CosNaming::Name_var cosName = inc->to_name(name);

#ifdef HAVE_OMNIORB4_CORBA_H
    CosNaming::Name_var cosName = omni::omniURI::stringToName(name);
#endif

    return cosName._retn();
}

void ORB::bind_object_to_name(CORBA::Object_ptr obj, const char *name)

{
    CosNaming::Name_var cosName = string_to_CosName(name);

    try {
        inc->rebind(cosName, obj);   // SCA says not to use rebind, sue me
    } catch (...) {
        cerr << "Failed to bind object to the name : " << name << endl;
        throw;
    }

}

void ORB::bind_object_to_name(CORBA::Object_ptr obj, const CosNaming::NamingContext_ptr nc, const char *name)

{
    CosNaming::Name_var cosName = string_to_CosName(name);

    try {
        nc->rebind(cosName, obj);   // SCA says not to use rebind, sue me
    } catch (...) {
        cerr << "Failed to bind object to the name : " << name << endl;
        throw;
    }

}

void ORB::unbind_name(const char *name)
{
    CosNaming::Name_var cosName = string_to_CosName(name);
    try {
        inc->unbind(cosName);
    } catch (...) {
        cerr << "Failed to unbind name : " << name << endl;
    }
}

void ORB::unbind_name(const CosNaming::NamingContext_ptr nc, const char *name)
{
    CosNaming::Name_var cosName = string_to_CosName(name);
    try {
        nc->unbind(cosName);
    } catch (...) {
        cerr << "Failed to unbind name : " << name << endl;
    }
}

void ORB::unbind_all_from_context(CosNaming::NamingContext_ptr nc)

{
///\todo Add support for deleting more than 100 names
    CosNaming::BindingIterator_var it;
    CosNaming::BindingList_var bl;
    const CORBA::ULong CHUNK = 100;

    nc->list(CHUNK, bl, it);

    for (unsigned int i = 0; i < bl->length(); i++) {
        nc->unbind(bl[i].binding_name);
    }
}
