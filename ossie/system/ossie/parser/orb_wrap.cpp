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

orb_wrap.cpp - this file contains interfaces to make access to the orb simpler

Even though all code is original, the architecture of the OSSIE Parser is based
on the architecture of the CRCs SCA Reference Implementation (SCARI)
see: http://www.crc.ca/en/html/rmsc/home/sdr/projects/scari

*********************************************************************************/

#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <assert.h>

#include "ossie/orb_wrap.h"

ORB_WRAP::ORB_WRAP ()
{
    if (CORBA::is_nil (ORB_WRAP::orb))
        ORB_WRAP::orb = CORBA::ORB_init (ORB_WRAP::vars, NULL);
    init ();
}


ORB_WRAP::ORB_WRAP (int argc, char **argv)
{
    if (CORBA::is_nil (ORB_WRAP::orb))
        ORB_WRAP::orb = CORBA::ORB_init (argc, argv);
    init ();
}


void
ORB_WRAP::init ()
{
    try {
        CORBAOBJ _poaObj = CORBA::Object::_nil ();

        while (CORBA::is_nil (_poaObj))
            _poaObj = orb->resolve_initial_references ("RootPOA");

        root_poa = PortableServer::POA::_narrow (_poaObj.in ());
        assert (!CORBA::is_nil (root_poa));
        mgr = root_poa->the_POAManager ();
        mgr->activate ();

        CORBAOBJ _nameObj = CORBA::Object::_nil ();

        while (CORBA::is_nil (_nameObj))
            _nameObj = orb->resolve_initial_references ("NameService");
        nc = CosNaming::NamingContext::_narrow (_nameObj.in ());
        assert (!CORBA::is_nil (nc));

        vars += 1;
    } catch (CORBA::Exception &) {
        cerr << "[ORB_WRAP::CTOR] ORB Initialization Failed" << endl;
    }
}


CORBAOBJ
ORB_WRAP::lookup (const char *_name)
{
    CosNaming::Name cosName;

    getCosName (cosName, _name);

    try {
        return nc->resolve (cosName);
    } catch (CORBA::Exception &) {
        cerr << "[ORB_WRAP::lookup] failed to resolve name - " << _name << endl;
    }

    return NULL;
}


void
ORB_WRAP::bindObj (CORBAOBJ & _obj, const char *_name)
{
    CosNaming::Name cosName;

    getCosName (cosName, _name);

    try {
        nc->rebind (cosName, _obj.in ());
    } catch (CORBA::Exception &) {
        cerr << "[ORB_WRAP::bindObj] Bind for " << _name << " failed." << endl;
    }
}


PortableServer::POA_var ORB_WRAP::getPOA () const
{
    return this->root_poa;
}


CosNaming::NamingContext_var ORB_WRAP::getNamingContext () const
{
    return this->nc;
}


void
ORB_WRAP::anyType (const char *_type, const char *_value, CORBA::Any & myRtn)
{
    CORBA::Any toRtn;
    char *ptr;

//              int argc = 0;
//              CORBA::ORB_var tmp = CORBA::ORB_init( argc, NULL );

    if (strcmp (_type, "double") == 0) {
        CORBA::Double val = strtod (_value, NULL);
        myRtn <<= val;
    } else if (strcmp (_type, "double") == 0) {
        CORBA::Float val = (float) strtod (_value, NULL);
        myRtn <<= val;
    } else if (strcmp (_type, "ulong") == 0) {
        CORBA::ULong val = strtoul (_value, &ptr, 0);
        myRtn <<= val;
    } else if (strcmp (_type, "long") == 0) {
        CORBA::Long val = strtol (_value, &ptr, 0);
        myRtn <<= val;
    } else if (strcmp (_type, "short") == 0) {
        CORBA::Short val = (short) atoi (_value);
        myRtn <<= val;
    } else if (strcmp (_type, "ushort") == 0) {
        CORBA::UShort val = (unsigned short) atoi (_value);
        myRtn <<= val;
    } else if (strcmp (_type, "string") == 0) {
        myRtn <<= CORBA::string_dup (_value);
    } else {
    }

//return toRtn;
    return;
}


bool
ORB_WRAP::isValidType (CORBA::Any _val1, CORBA::Any _val2)
{
    CORBA::TypeCode_var tc1 = _val1.type ();
    CORBA::TypeCode_var tc2 = _val2.type ();

    if (tc1->equal (tc2))
        return true;
    else
        return false;
}


void
ORB_WRAP::getCosName (CosNaming::Name & cosName, const char *_name)
{
    int _cnt = 0;

    string _context;

    char delim = '/';

    istringstream CNT (_name);

    getline (CNT, _context, delim);

    while (getline (CNT, _context, delim))
        _cnt++;

//              if( _cnt == 0 )
//                      throw;

// reset the streams get pointer to the beginning
    istringstream NAME (_name);

//      CosNaming::Name cosName;

    cosName.length (_cnt);

    getline (NAME, _context, delim);

    for (int i = 0; i < _cnt; i++) {
        getline (NAME, _context, delim);

        if (_context != "")
            cosName[i].id = CORBA::string_dup (_context.c_str ());
        else cosName[i].id="";
    }
}


bool
ORB_WRAP::orbIsWorking = false;

int
ORB_WRAP::vars = 0;

CORBA::ORB_var ORB_WRAP::orb = CORBA::ORB::_nil ();
