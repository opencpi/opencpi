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

#ifndef CPI_SCA_PROPERTY_PARSER_H__
#define CPI_SCA_PROPERTY_PARSER_H__

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
#include <CpiUtilVfs.h>
#include <sca_props.h>
#include <ezxml.h>

namespace CPI {
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

      PropertyParser (CPI::Util::Vfs::Vfs & fs,
                      const std::string & fileName)
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

      void parse (CPI::Util::Vfs::Vfs & fs,
                  const std::string & fileName)
        throw (std::string);

      void processSPD (CPI::Util::Vfs::Vfs & fs,
                       const std::string & fileName,
                       const std::string & implementation,
                       ezxml_t top)
        throw (std::string);

      void processSCD (CPI::Util::Vfs::Vfs & fs,
                       const std::string & fileName,
                       ezxml_t top)
        throw (std::string);

      void processPRF (ezxml_t top)
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
       * Size of the property space.
       *
       * \return The size of the property space, in bytes.
       */

      unsigned int sizeOfPropertySpace () const
        throw ();

      /**
       * Properties accessor.
       *
       * \param[out] numProps The number of properties.
       * \return An array of \a numProps properties.
       */

      const CPI::SCA::Property * getProperties (unsigned int & numProps) const
        throw ();

      /**
       * Ports accessor.
       *
       * \param[out] numPorts The number of ports.
       * \return An array of \a numPorts ports.
       */

      const CPI::SCA::Port * getPorts (unsigned int & numPorts) const
        throw ();

      /**
       * Tests accessor.
       *
       * \param[out] numTests The number of tests.
       * \return An array of \a numTests tests.
       */

      const CPI::SCA::Test * getTests (unsigned int & numTests) const
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

      void parsePropertyfile (CPI::Util::Vfs::Vfs & fs,
                              const std::string & fileName,
                              ezxml_t propertyFileNode)
        throw (std::string);

      void processSimpleProperty (ezxml_t simplePropertyNode,
                                  CPI::SCA::Property * propData,
                                  unsigned int & offset,
                                  bool isSequence,
                                  bool isTest)
        throw (std::string);

      void adjustSimpleProperty (ezxml_t simplePropertyNode,
                                 CPI::SCA::Property * propData,
                                 bool isSequence,
                                 bool isTest)
        throw (std::string);

      const char * getNameOrId (ezxml_t node)
        throw ();

      bool haveProperty (const char * name)
        throw ();

      static bool isConfigurableProperty (ezxml_t propertyNode)
        throw ();

      static CPI::SCA::DataType mapPropertyType (const char * type)
        throw (std::string);

      static unsigned int propertySize (CPI::SCA::DataType type)
        throw ();

      static unsigned int propertyAlign (CPI::SCA::DataType type)
        throw ();

      static unsigned int roundUp (unsigned int, unsigned int)
        throw ();

      static char * strdup (const char *)
        throw ();

    protected:
      char * m_magicString;
      unsigned int m_sizeOfPropertySpace;

      unsigned int m_numProperties;
      CPI::SCA::Property * m_properties;

      unsigned int m_numPorts;
      CPI::SCA::Port * m_ports;

      unsigned int m_numTests;
      CPI::SCA::Test * m_tests;

      typedef std::map<std::string, unsigned int> NameToIdxMap;
      NameToIdxMap m_nameToIdxMap;

      std::string m_componentType;
    };

  }
}

inline
unsigned int
CPI::SCA::PropertyParser::
sizeOfPropertySpace () const
  throw ()
{
  return m_sizeOfPropertySpace;
}

inline
const CPI::SCA::Property *
CPI::SCA::PropertyParser::
getProperties (unsigned int & numProps) const
  throw ()
{
  numProps = m_numProperties;
  return m_properties;
}

inline
const CPI::SCA::Port *
CPI::SCA::PropertyParser::
getPorts (unsigned int & numPorts) const
  throw ()
{
  numPorts = m_numPorts;
  return m_ports;
}

inline
const CPI::SCA::Test *
CPI::SCA::PropertyParser::
getTests (unsigned int & numTests) const
  throw ()
{
  numTests = m_numTests;
  return m_tests;
}

inline
const std::string &
CPI::SCA::PropertyParser::
getType () const
  throw ()
{
  return m_componentType;
}

#endif
