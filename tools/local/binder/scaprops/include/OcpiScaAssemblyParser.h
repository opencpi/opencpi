
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */


// -*- c++ -*-

#ifndef OCPI_SCA_ASSEMBLY_PARSER_H__
#define OCPI_SCA_ASSEMBLY_PARSER_H__

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
#include <OcpiUtilVfs.h>
#include <ezxml.h>

namespace OCPI {
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

      AssemblyParser (OCPI::Util::Vfs::Vfs & fs,
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

      void parse (OCPI::Util::Vfs::Vfs & fs,
                  const std::string & fileName)
        throw (std::string);

      void processSAD (OCPI::Util::Vfs::Vfs & fs,
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
const OCPI::SCA::Assembly &
OCPI::SCA::AssemblyParser::
get () const
  throw ()
{
  return m_assembly;
}

#endif
