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

#include <iostream>

#include <stdlib.h>

#include "ossie/debug.h"
#include <ossie/ossieSupport.h>
#include <ossie/prop_helpers.h>

using namespace ossieSupport;

ComponentInfo::ComponentInfo() : isAssemblyController(false), isResource(false), isConfigurable(false)
{
}

void ComponentInfo::setName(const char *_name)
{
    name = _name;
}

void ComponentInfo::setIdentifier(const char *_identifier)
{
    identifier = _identifier;
}

void ComponentInfo::setAssignedDeviceId(const char *_assignedDeviceId)
{
    assignedDeviceId = _assignedDeviceId;
}

void ComponentInfo::setLocalFileName(const char *_localFileName)
{
    localFileName = _localFileName;
}

void ComponentInfo::setImplPRFFile(const char *_PRFFile)
{
    implPRF = _PRFFile;
}

void ComponentInfo::setImplId(const char *_implId)
{
    implId = _implId;
}

void ComponentInfo::setCodeType(CF::LoadableDevice::LoadType _codeType)
{
    codeType = _codeType;
}

void ComponentInfo::setEntryPoint(const char *_entryPoint)
{
    entryPoint = _entryPoint;
}

void ComponentInfo::setNamingService(const bool _isNamingService)
{
    isNamingService = _isNamingService;
}

void ComponentInfo::setNamingServiceName(const char *_namingServiceName)
{
    namingServiceName = _namingServiceName;
}

void ComponentInfo::setUsageName(const char *_usageName)
{
    usageName = _usageName;
}

void ComponentInfo::setIsResource(bool _isResource)
{
    isResource = _isResource;
}

void ComponentInfo::setIsConfigurable(bool _isConfigurable)
{
    isConfigurable = _isConfigurable;
}

void ComponentInfo::setIsAssemblyController(bool _isAssemblyController)
{
    isAssemblyController = _isAssemblyController;
}

void ComponentInfo::addProperty(CF::DataType *dt, CF::Properties &prop)
{
    for (unsigned int i=0; i<prop.length(); i++) {
        if (strcmp(dt->id, prop[i].id) == 0) {
            // Overwrite existing value
            prop[i].value = dt->value;
            return;
        }
    }

// New id, add property at end
    prop.length(prop.length() + 1);
    prop[prop.length() - 1].id = dt->id;
    prop[prop.length() - 1].value = dt->value;
}

void ComponentInfo::addFactoryParameter(CF::DataType *dt)
{
    addProperty(dt, factoryParameters);
}

void ComponentInfo::addExecParameter(CF::DataType *dt)
{
    addProperty(dt, execParameters);
}

void ComponentInfo::addAllocationCapacity(CF::DataType *dt)
{
    addProperty(dt, allocationCapacities);
}

void ComponentInfo::addConfigureProperty(CF::DataType *dt)
{
    addProperty(dt, configureProperties);
}

void ComponentInfo::overrideProperty(const char *id, std::vector <std::string> values)
{
    process_overrides(&allocationCapacities, id, values);
    process_overrides(&configureProperties, id, values);
    process_overrides(&options, id, values);
    process_overrides(&factoryParameters, id, values);
    process_overrides(&execParameters, id, values);
}

void ComponentInfo::process_overrides(CF::Properties *props, const char *id, std::vector <std::string> values)
{

    for (unsigned int i = 0; i < (*props).length(); ++i ) {
        if (strcmp(id, (*props)[i].id) == 0) {
            DEBUG(3, AppSup, "Found prop for " << id)
            CORBA::TypeCode_var tc = (*props)[i].value.type();

            switch (tc->kind()) {
                /*case CORBA::tk_boolean:
                (*props)[i].value <<= ossieSupport::strings_to_boolean(values);
                break;*/

                /*case CORBA::tk_char:
                (*props)[i].value <<= ossieSupport::strings_to_char(values);
                break;*/

            case CORBA::tk_double:
                (*props)[i].value <<= ossieSupport::strings_to_double(values);
                break;

                /*case CORBA::octet:
                (*props)[i].value <<= ossieSupport::strings_to_octet(values);
                break;*/

            case CORBA::tk_ushort:
                (*props)[i].value <<= ossieSupport::strings_to_unsigned_short(values);
                break;

            case CORBA::tk_short:
                (*props)[i].value <<= ossieSupport::strings_to_short(values);
                break;

            case CORBA::tk_float:
                (*props)[i].value <<= ossieSupport::strings_to_float(values);
                break;

            case CORBA::tk_ulong:
                (*props)[i].value <<= ossieSupport::strings_to_unsigned_long(values);
                break;

            case CORBA::tk_long:
                (*props)[i].value <<= ossieSupport::strings_to_long(values);
                break;

            case CORBA::tk_string:
                (*props)[i].value <<= ossieSupport::strings_to_string(values);
                break;

            case CORBA::tk_alias: {
                CORBA::TypeCode_var tca = tc->content_type();

                switch (tca->kind()) {
                case CORBA::tk_sequence: {
                    CORBA::TypeCode_var tc_seq = tca->content_type();
                    DEBUG(3, AppSup, "Property override, sequence of " << tc_seq->kind())
                    switch (tc_seq->kind()) {
                    case CORBA::tk_float:
                        (*props)[i].value <<= ossieSupport::strings_to_float_sequence(values);
                        break;

                    case CORBA::tk_double:
                        (*props)[i].value <<= ossieSupport::strings_to_double_sequence(values);
                        break;

                    case CORBA::tk_long:
                        (*props)[i].value <<= ossieSupport::strings_to_long_sequence(values);
                        break;

                    case CORBA::tk_ulong:
                        (*props)[i].value <<= ossieSupport::strings_to_unsigned_long_sequence(values);
                        break;

                        /*case CORBA::tk_boolean:
                        (*props)[i].value <<= ossieSupport::strings_to_boolean_sequence(values);
                        break;*/

                        /*case CORBA::tk_char:
                        (*props)[i].value <<= ossieSupport::strings_to_char_sequence(values);
                        break;*/

                        /*case CORBA::tk_octet:
                        (*props)[i].value <<= ossieSupport::strings_to_octet_sequence(values);
                        break;*/

                    case CORBA::tk_short:
                        (*props)[i].value <<= ossieSupport::strings_to_short_sequence(values);
                        break;

                    case CORBA::tk_ushort:
                        (*props)[i].value <<= ossieSupport::strings_to_unsigned_short_sequence(values);
                        break;

                        /*case CORBA::tk_string:
                        (*props)[i].value <<= ossieSupport::strings_to_string_sequence(values);
                        break;*/

                    default:
                        DEBUG(3, AppSup, "Property override from SAD, sequence of unknown type = " << tca->content_type())
                    }
                }
                break;

                default:
                    std::cout << "Property override from SAD, property (typedefed) has unknown type code = " << tc->kind() << std::endl;
                }
            }
            break;

            default:
                std::cout << "Property override from SAD, property has unknown type code = " << tc->kind() << std::endl;
            }
        }
    }
}


void ComponentInfo::setResourcePtr(CF::Resource_ptr _rsc)
{
    rsc = _rsc;
}

const char* ComponentInfo::getName()
{
    return name.c_str();
}

const char* ComponentInfo::getIdentifier()
{
    return identifier.c_str();
}

const char* ComponentInfo::getAssignedDeviceId()
{
    return assignedDeviceId.c_str();
}

const char* ComponentInfo::getImplId()
{
    return implId.c_str();
}

CF::LoadableDevice::LoadType ComponentInfo::getCodeType()
{
    return codeType;
}

const char* ComponentInfo::getLocalFileName()
{
    return localFileName.c_str();
}

const char* ComponentInfo::getImplPRFFile()
{
    return implPRF.c_str();
}

const char* ComponentInfo::getEntryPoint()
{
    return entryPoint.c_str();
}

const bool  ComponentInfo::getNamingService()
{
    return isNamingService;
}

const char* ComponentInfo::getUsageName()
{
    return usageName.c_str();
}

const char* ComponentInfo::getNamingServiceName()
{
    return namingServiceName.c_str();
}

const bool  ComponentInfo::getIsResource()
{
    return isResource;
}

const bool  ComponentInfo::getIsConfigurable()
{
    return isConfigurable;
}

const bool  ComponentInfo::getIsAssemblyController()
{
    return isAssemblyController;
}

CF::Properties ComponentInfo::getConfigureProperties()
{
    return configureProperties;
}

CF::Properties ComponentInfo::getAllocationCapacities()
{
    return allocationCapacities;
}

CF::Properties ComponentInfo::getOptions()
{
    return options;
}

CF::Properties ComponentInfo::getExecParameters()
{
    return execParameters;
}

CF::Resource_ptr ComponentInfo::getResourcePtr()
{
    return CF::Resource::_duplicate(rsc);
}

ConnectionInfo::ConnectionInfo(CF::Port_ptr obj, const char *id)
{
    port_obj = obj;
    identifier = id;
}

CF::Port_ptr ConnectionInfo::getPort()
{
    return port_obj;
}

const char * ConnectionInfo::getID()
{
    return identifier.c_str();
}
