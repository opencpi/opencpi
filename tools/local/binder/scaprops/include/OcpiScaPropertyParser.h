
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

#ifndef OCPI_SCA_PROPERTY_PARSER_H__
#define OCPI_SCA_PROPERTY_PARSER_H__

/**
 * \file
 * \brief Parse SCA SPD, SCD and PRF files.
 *
 * Revision History:
 *
 *     02/20/2009 - Frank Pilhofer
 *                  Parse test properties.
 *
 *     10/13/2008 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <map>
#include <string>
#include <OcpiUtilVfs.h>
#include <sca_props.h>
#include <ezxml.h>

namespace OCPI {
  namespace SCA {

    /**
     * \brief Parse SCA SPD, SCD and PRF files into the "magic property string" format.
     *
     * This class reads Software Package Descriptor (SPD), Software Component
     * Descriptor (SCD) and Property File (PRF) files, gathering information
     * about a component's ports and properties.  Creates a mapping of the
     * property space.  This information is then encoded into a string format
     * that can be used with the generic proxy.
     */

    class PropertyParser {
    public:
      /**
       * Default constructor.
       *
       * Must call parse().
       */

      PropertyParser ()
        throw ();

      /**
       * Constructor.
       *
       * Calls #parse (\a fs, \a fileName).
       *
       * \note See parse() for more details.
       */

      PropertyParser (OCPI::Util::Vfs::Vfs & fs,
                      const std::string & fileName,
		      const std::string & implementation = std::string())
        throw (std::string);

      /**
       * Destructor.
       */

      ~PropertyParser ()
        throw ();

      /**
       * \brief Parse an SPD, SCD or PRF file.
       *
       * If \a fileName is an SPD or SCD file, gather port and property
       * information (following the "propertyfile" element to the PRF file(s)).
       * If it is a PRF file, gather property information only (assuming that
       * the component has no ports).
       *
       * \param[in] fs The file system to read the SPD, SCD or PRF file from.
       * \param[in] fileName The file name of the SPD, SCD or PRF file.
       *
       * \throw std::string If the file does not exist or can not be parsed.
       * \throw std::string If the file is an SCD file and the referenced
       * PRF file (propertyfile element) can not be located.
       *
       * \note If the file is an SPD or SCD file, then the PRF file(s) are
       * searched for both using the relative URI in the propertyfile element
       * as well as in the same directory as the SCD file.
       *
       * \note This method can be called multiple times to aggregate port
       * and property information from multiple SPD/SCD/PRF files.
       */

      void parse (OCPI::Util::Vfs::Vfs & fs,
                  const std::string & fileName,
		  const std::string & implementation = std::string())
        throw (std::string);

      void processSPD (OCPI::Util::Vfs::Vfs & fs,
                       const std::string & fileName,
                       const std::string & implementation,
                       ezxml_t top)
        throw (std::string);

      void processSCD (OCPI::Util::Vfs::Vfs & fs,
                       const std::string & fileName,
                       ezxml_t top)
        throw (std::string);

      void processPRF (ezxml_t top, bool impl = false)
        throw (std::string);

      /**
       * Returns port and property information as a magic string.
       *
       * After gathering component information using #parse(), this
       * function can be used to retrieve a magic string, which encodes
       * the component's port and property space information.  This
       * string can later be passed to the generic proxy.
       *
       * \return The property "magic" string.  This class retains ownership
       * of the string; it must be used or copied before the destructor is
       * called.
       */

      const char * encode ()
        throw ();

      /**
       * Emit the ocpi xml spec and impl files corresponding to the parsed SPD etc.
       * All string arguments are optional (can be NULL).
       * If implId is NULL, the first implementation is used.
       *   Later use some attribute pattern match to id the impl?
       *   If there is no implementations it is an error.
       * If specFile is NULL, the file name of the spdFile are used (+ _spec).
       * If implFile is NULL, the file name of the spdFile is used.
       * If outDir is NULL, no output directory is used (prepended)
       * Return a string error.
       */

      const char * emitOcpiXml(std::string &name, std::string &specFile, std::string &specDir,
			       std::string &implFile, std::string&implDir, std::string &model,
			       char *idlFiles[], bool debug)
        throw ();

      /**
       * Size of the property space.
       *
       * \return The size of the property space, in bytes.
       */

      size_t sizeOfPropertySpace () const
        throw ();

      /**
       * Properties accessor.
       *
       * \param[out] numProps The number of properties.
       * \return An array of \a numProps properties.
       */

      const OCPI::SCA::Property * getProperties (unsigned int & numProps) const
        throw ();

      /**
       * Ports accessor.
       *
       * \param[out] numPorts The number of ports.
       * \return An array of \a numPorts ports.
       */

      const OCPI::SCA::Port * getPorts (unsigned int & numPorts) const
        throw ();

      /**
       * Tests accessor.
       *
       * \param[out] numTests The number of tests.
       * \return An array of \a numTests tests.
       */

      const OCPI::SCA::Test * getTests (unsigned int & numTests) const
        throw ();

      /**
       * Component "type" accessor.
       *
       * The file name of the SCD file is used as the component type,
       * unless we are processing an SPD file that has a propertyfile
       * element that adds to the component interface, in which case
       * the SPD's "name" attribute is used.  If we are processing a
       * specific implementation, and that implementation has a
       * propertyfile element that adds to the component interface,
       * then that implementation's "id" is used.
       *
       * \return The component "type".
       */

      const std::string & getType () const
        throw ();

    protected:
      void cleanup ()
        throw ();

      void parsePropertyfile (OCPI::Util::Vfs::Vfs & fs,
                              const std::string & fileName,
                              ezxml_t propertyFileNode,
			      bool impl = false)
        throw (std::string);

      void processSimpleProperty (ezxml_t simplePropertyNode,
                                  OCPI::SCA::Property * propData,
                                  size_t & offset,
                                  bool isSequence,
                                  bool isTest,
				  bool isImpl = false)
        throw (std::string);

      void adjustSimpleProperty (ezxml_t simplePropertyNode,
                                 OCPI::SCA::Property * propData,
                                 bool isSequence,
                                 bool isTest)
        throw (std::string);

      void processStructProperty (ezxml_t simplePropertyNode,
                                  OCPI::SCA::Property * propData,
                                  size_t & offset,
                                  bool isSequence,
                                  bool isTest,
				  bool isImpl = false)
        throw (std::string);

      static void doSimple(ezxml_t simplePropertyNode, OCPI::SCA::SimpleType *pt, size_t &max_align, size_t &size);

      const char * getNameOrId (ezxml_t node)
        throw ();

      bool haveProperty (const char * name)
        throw ();

      static bool isConfigurableProperty (ezxml_t propertyNode)
        throw ();

      static OCPI::SCA::DataType mapPropertyType (const char * type)
        throw (std::string);

      static size_t propertySize (OCPI::SCA::DataType type)
        throw ();

      static size_t propertyAlign (OCPI::SCA::DataType type)
        throw ();

      static char * strdup (const char *)
        throw ();

    protected:
      char * m_magicString;
      size_t m_sizeOfPropertySpace;

      unsigned int m_numProperties;
      OCPI::SCA::Property * m_properties;

      unsigned int m_numPorts;
      OCPI::SCA::Port * m_ports;

      unsigned int m_numTests;
      OCPI::SCA::Test * m_tests;

      typedef std::map<std::string, unsigned int> NameToIdxMap;
      NameToIdxMap m_nameToIdxMap;

      std::string m_componentType, m_scdName, m_spdName, m_implName, m_spdFileName, m_spdPathName;
    };

  }
}

inline
size_t
OCPI::SCA::PropertyParser::
sizeOfPropertySpace () const
  throw ()
{
  return m_sizeOfPropertySpace;
}

inline
const OCPI::SCA::Property *
OCPI::SCA::PropertyParser::
getProperties (unsigned int & numProps) const
  throw ()
{
  numProps = m_numProperties;
  return m_properties;
}

inline
const OCPI::SCA::Port *
OCPI::SCA::PropertyParser::
getPorts (unsigned int & numPorts) const
  throw ()
{
  numPorts = m_numPorts;
  return m_ports;
}

inline
const OCPI::SCA::Test *
OCPI::SCA::PropertyParser::
getTests (unsigned int & numTests) const
  throw ()
{
  numTests = m_numTests;
  return m_tests;
}

inline
const std::string &
OCPI::SCA::PropertyParser::
getType () const
  throw ()
{
  return m_componentType;
}

#endif
