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

#include "ossie/PropertySet_impl.h"

void
PropertySet_impl::configure (const CF::Properties & configProperties)
throw (CORBA::SystemException, CF::PropertySet::InvalidConfiguration,
       CF::PropertySet::PartialConfiguration)
{
    if (propertySet.length () == 0) {
        propertySet.length (configProperties.length ());

        for (unsigned int i = 0; i < configProperties.length (); i++) {
            propertySet[i].id = CORBA::string_dup (configProperties[i].id);

            propertySet[i].value = configProperties[i].value;

            CORBA::Short len = -1;

            propertySet[i].value >>= len;
        }

        return;
    }

// Unused PJB   int numProperties = configProperties.length();

    CF::Properties validProperties;
    validProperties.length (0);

    CF::Properties invalidProperties;
    invalidProperties.length (0);

    validate (configProperties, validProperties, invalidProperties);

    for (unsigned int j = 0; j < validProperties.length (); j++) {
        for (int k = 0; propertySet.length (); k++) {
            if (strcmp (propertySet[k].id, validProperties[j].id) == 0) {
                propertySet[k].value = validProperties[j].value;
                break;
            }
        }
    }

    if (validProperties.length () == 0 && invalidProperties.length () != 0) {
        throw CF::PropertySet::InvalidConfiguration ();
    } else if (validProperties.length () > 0 && invalidProperties.length () > 0) {
        throw CF::PropertySet::PartialConfiguration ();
    } else {
    }
}


void
PropertySet_impl::query (CF::Properties & configProperties)
throw (CORBA::SystemException, CF::UnknownProperties)
{
// for queries of zero length, return all id/value pairs in propertySet
    if (configProperties.length () == 0) {
        configProperties.length (propertySet.length ());

        for (unsigned int i = 0; i < propertySet.length (); i++) {
            configProperties[i].id = CORBA::string_dup (propertySet[i].id);
            configProperties[i].value = propertySet[i].value;
        }

        return;
    }

// for queries of length > 0, return all requested pairs in propertySet
    CF::Properties validProperties;
    validProperties.length (0);

    CF::Properties invalidProperties;
    invalidProperties.length (0);

    validate (configProperties, validProperties, invalidProperties);

//returns values for valid queries in the same order as requested
    for (unsigned int i = 0; i < validProperties.length (); i++) {
        for (unsigned int j = 0; j < configProperties.length (); j++) {
            if (strcmp(configProperties[j].id,validProperties[i].id)==0) {
                configProperties[j] =
                    getProperty (CORBA::string_dup (validProperties[i].id));
                break;
            }
        }
    }

    if (invalidProperties.length () != 0)
        throw CF::UnknownProperties();
}


void
PropertySet_impl::validate (CF::Properties property,
                            CF::Properties & validProps,
                            CF::Properties & invalidProps)
{
    int cntValid = 0;
    int cntInvalid = 0;

    for (unsigned int i = 0; i < property.length (); i++) {
        bool SUCCESS = false;
        for (unsigned int j = 0; j < propertySet.length (); j++) {
            if (strcmp (propertySet[j].id, property[i].id) == 0) {
                SUCCESS = true;
                break;
            }
        }

        if (SUCCESS) {
            validProps.length (cntValid + 1);
            validProps[cntValid].id = property[i].id;
            validProps[cntValid].value = property[i].value;
            cntValid++;
        } else {
            invalidProps.length (cntInvalid + 1);
            invalidProps[cntInvalid].id = property[i].id;
            invalidProps[cntInvalid].value = property[i].value;
            cntInvalid++;
        }
    }
}


CF::DataType PropertySet_impl::getProperty (CORBA::String_var _id)
{
    for (unsigned int i = 0; i < propertySet.length (); i++) {
        if (strcmp (propertySet[i].id, _id) == 0)
            return propertySet[i];
    }
    return CF::DataType ();
}
