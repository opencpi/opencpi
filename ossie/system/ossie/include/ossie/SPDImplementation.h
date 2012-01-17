/*******************************************************************************

Copyright 2004, 2007 Virginia Polytechnic Institute and State University

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
on the architecture of the CRCs SCA Reference Implementation(SCARI)
see: http://www.crc.ca/en/html/rmsc/home/sdr/projects/scari

*********************************************************************************/

#ifndef SPDIMPLEMENTATION_H
#define SPDIMPLEMENTATION_H

#include <vector>
#include <string>

#include "ossie/SPDUsesDevice.h"
#include "ossie/PropertyFile.h"
#include "ossie/SPDDependency.h"

#include "tinyxml.h"

class OSSIEPARSER_API SPDParser;

class OSAttributes
{
public:
    OSAttributes() {};
//  OSAttributes(const OSAttributes &);

    void setOSName(const char * _name) {
        osName = _name;
    };
    void setOSVersion(const char *_version) {
        osVersion = _version;
    };
    const char* getOSName() {
        return osName.c_str();
    };
    const char* getOSVersion() {
        return osVersion.c_str();
    };

private:

    std::string osName;
    std::string osVersion;
};

class OSSIEPARSER_API SPDImplementation
{
private:
    SPDImplementation(); // No default constructor
    SPDImplementation(const SPDImplementation & _spdi); // No copying

    CF::Properties propDep;
    CF::LoadableDevice::LoadType codeType;

    std::string implementationID;
    PropertyFile *PRFFile;

    std::string codeFile;
    std::string entryPoint;
    std::string stackSize;
    std::string priority;

    std::string compilerName;
    std::string compilerVersion;
    std::string prgLanguageName;
    std::string prgLanguageVersion;
    std::string humanLanguageName;
    std::string runtimeVersion;
    std::string runtimeName;
    OSAttributes os;
    std::vector <SPDDependency*> dependencies;

    std::vector < std::string >processorsName;
    std::vector < SPDUsesDevice*  >usesDevice;

protected:
    void parseElement(TiXmlElement *elem);
    void parseID(TiXmlElement *elem);
    void parsePRFRef(TiXmlElement *elem);
    void parseCode(TiXmlElement *elem);
    void parseCompiler(TiXmlElement *elem);
    void parsePrgLanguage(TiXmlElement *elem);
    void parseHumanLanguage(TiXmlElement *elem);
    void parseRuntime(TiXmlElement *elem);
    void parseOperatingSystems(TiXmlElement *elem);
    void parseProcessors(TiXmlElement *elem);
    void parseDependencies(TiXmlElement *elem);
#if 0 ///\todo Figure out dependencies
    void parsePropertyDependencies(TiXmlElement *elem);
#endif
    void parseUsesDevices(TiXmlElement *elem);

public:
    SPDImplementation(TiXmlElement *elem);
    ~SPDImplementation();

    const char* getID();
    PropertyFile* getPRFFile();
    const char* getCodeFile();
    const char* getEntryPoint();
    const char* getCompilerName();
    const char* getCompilerVersion();
    const char* getPrgLanguageName();
    const char* getPrgLanguageVersion();
    const char* getHumanLanguageName();
    const char* getRuntimeName();
    const char* getRuntimeVersion();
    OSAttributes getOperatingSystem();

    void setCodeType(const char* _ct);

    std::vector <SPDUsesDevice*> *getUsesDevices();
    std::vector <std::string> getProcessors() const;
    std::vector <SPDDependency*> getDependencies();
    CF::Properties getPropertyDependencies() const;
    CF::LoadableDevice::LoadType getCodeType() const;

#if 0
    static int OS_NAME;
    static int OS_VERSION;
    static int SOFTWARE_SPD_NAME;
    static int SOFTWARE_IMPL_REF;
#endif
};
#endif
