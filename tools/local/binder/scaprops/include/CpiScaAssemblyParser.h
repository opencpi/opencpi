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

// -*- c++ -*-

#ifndef CPI_SCA_ASSEMBLY_PARSER_H__
#define CPI_SCA_ASSEMBLY_PARSER_H__

/**
 * \file
 * \brief Parse SCA SAD files.
 *
 * Revision History:
 *
 *     03/02/2009 - Frank Pilhofer
 *                  Initial version.
 */

#include <map>
#include <string>
#include <vector>
#include <CpiUtilVfs.h>
#include <ezxml.h>

namespace CPI {
  namespace SCA {

    /**
     * \brief Parse SCA Software Assembly Descriptor files.
     */

    typedef std::map<std::string, std::string> StringStringMap;

    struct ComponentPlacement {
      std::string fileRef;
      std::string spdFileName;
      std::string usageName;
    };

    // Indexed by component instantiation id
    typedef std::map<std::string, ComponentPlacement> ComponentPlacements;

    struct HostCollocation {
      std::string id;
      std::string name;
      ComponentPlacements componentPlacement;
    };

    typedef std::vector<HostCollocation> HostCollocations;

    struct FindBy {
      enum FindByType {
        NAMINGSERVICE,
        DOMAINFINDER
      };

      enum DomainFinderType {
        DF_FILEMANAGER,
        DF_LOG,
        DF_EVENTCHANNEL,
        DF_NAMINGSERVICE
      };

      FindByType type;
      std::string name;
      DomainFinderType domainFinderType;
    };

    struct PortRef {
      enum PortRefType {
        COMPONENTINSTANTIATIONREF,
        DEVICETHATLOADEDTHISCOMPONENTREF,
        DEVICEUSEDBYTHISCOMPONENTREF,
        FINDBY
      };

      PortRefType type;
      std::string portName;
      std::string refId;
      std::string usesRefId;
      FindBy findBy;
    };

    struct SupportedInterfaceRef {
      enum SupportedInterfaceRefType {
        COMPONENTINSTANTIATIONREF,
        FINDBY
      };

      SupportedInterfaceRefType type;
      std::string supportedIdentifier;
      std::string refId;
      FindBy findBy;
    };

    struct Connection {
      enum ProvidesPortType {
        PROVIDESPORT,
        COMPONENTSUPPORTEDINTERFACE,
        FINDBY
      };

      ProvidesPortType type;
      std::string id;
      PortRef usesPort;
      PortRef providesPort;
      SupportedInterfaceRef supportedInterface;
      FindBy findBy;
    };

    typedef std::vector<Connection> Connections;

    struct ExternalPort {
      enum ExternalPortType {
        USESIDENTIFIER,
        PROVIDESIDENTIFIER,
        SUPPORTEDIDENTIFIER
      };

      ExternalPortType type;
      std::string portName;
      std::string refId;
      std::string description;
    };

    typedef std::vector<ExternalPort> ExternalPorts;
    typedef std::map<std::string, unsigned int> StringUIntMap;

    struct Assembly {
      std::string id;
      std::string name;
      std::string version;
      std::string description;
      StringStringMap componentFiles; // maps id to localfile
      ComponentPlacements componentPlacement;
      HostCollocations hostCollocation;
      StringUIntMap allComponents; // maps instance id to host collocation index or -1
      std::string assemblyController;
      Connections connection;
      ExternalPorts externalPort;
    };

    class AssemblyParser {
    public:
      /**
       * Default constructor.
       * Must call parse().
       */

      AssemblyParser ()
        throw ();

      /**
       * Constructor.
       *
       * Calls #parse (\a fs, \a fileName).
       *
       * \note See parse() for more details.
       */

      AssemblyParser (CPI::Util::Vfs::Vfs & fs,
                      const std::string & fileName)
        throw (std::string);

      /**
       * Destructor.
       */

      ~AssemblyParser ()
        throw ();

      /**
       * \brief Parse a Software Assembly Descriptor file.
       */

      void parse (CPI::Util::Vfs::Vfs & fs,
                  const std::string & fileName)
        throw (std::string);

      void processSAD (CPI::Util::Vfs::Vfs & fs,
                       const std::string & fileName,
                       ezxml_t top)
        throw (std::string);

      /**
       * \brief Assembly Accessor.
       */

      const Assembly & get () const
        throw ();

    protected:
      void parseComponentFileNode (ezxml_t cfNode, const char * id)
        throw (std::string);

      void parseHostCollocation (ezxml_t hcNode, unsigned int hcIdx)
        throw (std::string);

      void parseComponentPlacement (ezxml_t cpNode,
                                    ComponentPlacements & cp,
                                    unsigned int hcIdx = static_cast<unsigned int> (-1))
        throw (std::string);

      void parseConnection (ezxml_t ciNode, unsigned int ciIdx)
        throw (std::string);

      void parsePortRef (ezxml_t portNode, PortRef & portRef, bool isProvides)
        throw (std::string);

      void parseSupportedInterfaceRef (ezxml_t csiNode, SupportedInterfaceRef & supportedInterface)
        throw (std::string);

      void parseFindBy (ezxml_t findByNode, FindBy & findBy)
        throw (std::string);

      void parseExternalPort (ezxml_t epNode, unsigned int epIdx)
        throw (std::string);

    protected:
      Assembly m_assembly;
    };
  }
}

inline
const CPI::SCA::Assembly &
CPI::SCA::AssemblyParser::
get () const
  throw ()
{
  return m_assembly;
}

#endif
