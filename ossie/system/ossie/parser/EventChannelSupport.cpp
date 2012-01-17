/****************************************************************************

Copyright 2005, Virginia Polytechnic Institute and State University

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


#include <iostream>

#ifdef HAVE_OMNIORB4
#include <omniORB4/CORBA.h>
#endif

#include "ossie/StandardEvent.h"
#include "ossie/ossieSupport.h"

void ossieSupport::sendObjAdded_event(const char *producerId, const char *sourceId, const char *sourceName, CORBA::Object_ptr sourceIOR, StandardEvent::SourceCategoryType sourceCategory)

{

    std::cout << "Object Added Event - ProducerId: " << producerId << "  SourceId: " << sourceId << "  SourceName: " << sourceName << std::endl;


// Future implementation
#ifdef HAVE_OMNIEVENTS
    StandardEvent::DomainManagementObjectAddedEventType event_data;
    CORBA::Any any;

    event_data.producerId = CORBA::string_dup(producerId);
    event_data.sourceId = CORBA::string_dup(sourceId);
    event_data.sourceName = CORBA::string_dup(sourceName);
//  event_data.sourceIOR = sourceIOR;
    event_data.sourceCategory = sourceCategory;

    any <<= event_data;

    ODM_Channel_consumer->push(any);
#endif

}
