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

#ifndef DEVICE_IMPL_H
#define DEVICE_IMPL_H

#include <string>
#include <iostream>

#include "Resource_impl.h"
#include "cf.h"
#include "ossie/debug.h"

class OSSIECF_API Device_impl:public virtual POA_CF::Device, public Resource_impl
{
protected:
    char *keyProperties[6];
    enum AnyComparisonType {
        FIRST_BIGGER,
        SECOND_BIGGER,
        BOTH_EQUAL,
        POSITIVE,
        NEGATIVE,
        ZERO,
        UNKNOWN
    };
    CF::Device::AdminType _adminState;
    CF::Device::UsageType _usageState;
    CF::Device::OperationalType _operationalState;
    std::string _softwareProfile;
    std::string _label;

//AggregateDevice _compositeDevice;
    bool initialConfiguration;
    CF::Properties originalCap;
    void setUsageState (CF::Device::UsageType newUsageState);
    Device_impl::AnyComparisonType compareAnyToZero (CORBA::Any & first);
    Device_impl::AnyComparisonType compareAnys (CORBA::Any & first, CORBA::Any & second);
    void deallocate (CORBA::Any & deviceCapacity, const CORBA::Any & resourceRequest);
    bool allocate (CORBA::Any & deviceCapacity, const CORBA::Any & resourceRequest);

public:
    Device_impl (char *, char *, char *);
    Device_impl (char *, char *, char *, CF::Properties & capacities);
    ~Device_impl ();

    char *label () throw (CORBA::SystemException);
    char *softwareProfile () throw (CORBA::SystemException);
    CF::Device::UsageType usageState ()throw (CORBA::SystemException);
    CF::Device::AdminType adminState ()throw (CORBA::SystemException);
    CF::Device::OperationalType operationalState ()throw (CORBA::SystemException);
    CF::AggregateDevice_ptr compositeDevice ()throw (CORBA::SystemException);
    void adminState (CF::Device::AdminType _adminType) throw (CORBA::SystemException);
    void deallocateCapacity (const CF::Properties & capacities) throw (CF::Device::InvalidState, CF::Device::InvalidCapacity, CORBA::SystemException);
    CORBA::Boolean allocateCapacity (const CF::Properties & capacities) throw (CF::Device::InvalidState, CF::Device::InvalidCapacity, CORBA::SystemException);
    bool isUnlocked ();
    bool isLocked ();
    bool isDisabled ();
    bool isBusy ();
    bool isIdle ();
    void configure (const CF::Properties & configProperties) throw (CF::PropertySet::PartialConfiguration, CF::PropertySet::InvalidConfiguration, CORBA::SystemException);

private:
    Device_impl(); // Code that tries to use this constructor will not work
    Device_impl(Device_impl&); // No copying
};


#endif

