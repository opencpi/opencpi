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

Even though all code is original, the architecture of the OSSIE Parser is based
on the architecture of the CRCs SCA Reference Implementation (SCARI)
see: http://www.crc.ca/en/html/rmsc/home/sdr/projects/scari

*********************************************************************************/

#ifndef ORB_DEV_H
#define ORB_DEV_H

#include "ossieparser.h"

#include "omniORB4/CORBA.h"

#include <iostream>

using namespace std;

#define CORBAOBJ CORBA::Object_var

#define CORBAPTR CORBA::Object_ptr

/*! \deprecated

This code will be replaced with a new imporved set of CORBA support utilities.

*/
class OSSIEPARSER_API ORB_WRAP
{


private:
    CosNaming::NamingContext_var nc;
    PortableServer::POA_var root_poa;
    PortableServer::POAManager_var mgr;

    static bool orbIsWorking;

    void init ();

public:
    static CORBA::ORB_var orb;
    static int vars;

    ORB_WRAP ();

    ORB_WRAP (int, char **);

//      ~ORB_WRAP();

    CORBAOBJ lookup (const char *_name);

    void bindObj (CORBAOBJ & _obj, const char *_name);

    PortableServer::POA_var getPOA () const;

    CosNaming::NamingContext_var getNamingContext () const;

//static CORBA::Any anyType( const char* _type, const char* _value );
    static void anyType (const char *_type, const char *_value, CORBA::Any &);

    static bool isValidType (CORBA::Any _val1, CORBA::Any _val2);

    static void getCosName (CosNaming::Name & cosName, const char *_name);

    static bool isWorking () {
        return ORB_WRAP::orbIsWorking;
    }

    static void startWork () {
        ORB_WRAP::orbIsWorking = true;
    }

    static void stopWork () {
        ORB_WRAP::orbIsWorking = false;
    }

//      static int getOrbArgs() { return orb_args; }

//      static char* getOrbParms() { return orb_parms; }
};
#endif
