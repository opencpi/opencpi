
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




/**
  Given an OpenOCPI implementation metadata file for an X-Midas primitive
  this tool generates the OpenOCPI RCC wrapper that will permit the
  X-Midas primitive to be used as an OpenOCPI RCC component outside of
  the X-Midas runtime environment.
  When an X-Midas primitive is wrapped to become an RCC component the
  primitive no longer depends on the X-Midas runtime environment or
  build system. Instead, the X-Midas primitive becomes dependent on
  the X-Midas "intercept" library which is collection of M$ and m_ functions
  that replaces the X-Midas library implementation with an OpenOCPI
  implementation.

  Usage:

  $ ocpixm <path to X-Midas metadata file>

  Below is an example of the X-Midas XML metadata for a primitive. Please
  see the X-Midas authoring model reference and users guide for more
  information.

  filename: ubiquitous_xm.xml

  <?xml version="1.0" encoding="us-ascii"?>

  <XmImplementation Name="ubiquitous" Language="C">

    <xi:include href="../ubiquitous_spec.xml"/>

    <ControlInterface Name="ControlPlane" ControlOperations="initialize, start, stop, finish"/>

    <XmCommandLineArguments>
      <XmCommandLineArgument Name="signal1" Argument="1" Value="signal1" Type="String" StringLength="80"/>
      <XmCommandLineArgument Name="power1" Argument="2" Value="power1" Type="String" StringLength="80"/>
      <XmCommandLineArgument Name="transform size" Argument="3" Property="transform size"/>
      <XmCommandLineArgument Name="window" Argument="4" Value="NONE" Type="String" StringLength="80"/>
      <XmCommandLineArgument Name="overlap" Argument="5" Property="overlap"/>
      <XmCommandLineArgument Name="coupling" Argument="6" Property="coupling"/>
      <XmCommandLineArgument Name="averaging" Argument="7" Property="averaging"/>
      <XmCommandLineArgument Name="signal2" Argument="8" Value="signal2" Type="String" StringLength="80"/>
      <XmCommandLineArgument Name="power2" Argument="9" Value="power2" Type="String" StringLength="80"/>
      <XmCommandLineArgument Name="cross_spectrum" Argument="10" Value="cross_spectrum" Type="String" StringLength="80"/>
    </XmCommandLineArguments>

    <XmWidgets>
      <XmWidget Name="window" Property="window"/>
      <XmWidget Name="averaging" Property="averaging"/>
      <XmWidget Name="x output" Property="modex"/>
      <XmWidget Name="nfft" Property="transform size"/>
      <XmWidget Name="transfer" Property="transform size"/>
    </XmWidgets>

    <!-- Use numeric values for the switch (not the string names) -->

    <XmSwitches>
      <XmSwitch Name="modex" Property="modex"/>
      <XmSwitch Name="tl" Property="transform size"/>
    </XmSwitches>

    <XmHeader Name="signal1" filespec="BLUE" type="1000" buf_type="F" format="CF" xstart="0.0" xdelta="1.0"/>
    <XmHeader Name="signal2" filespec="BLUE" type="1000" buf_type="F" format="CF" xstart="0.0" xdelta="1.0"/>
    <XmHeader Name="power1" filespec="BLUE" type="1000" buf_type="F" format="CF" xstart="0.0" xdelta="1.0"/>
    <XmHeader Name="power2" filespec="BLUE" type="1000" buf_type="F" format="CF" xstart="0.0" xdelta="1.0"/>
    <XmHeader Name="cross_spectrum" filespec="BLUE" type="1000" buf_type="F" format="CF" xstart="0.0" xdelta="1.0"/>

  </XmImplementation>

************************************************************************** */

#include <OcpiOsAssert.h>
#include <OcpiUtilVfs.h>
#include <OcpiUtilFileFs.h>
#include <OcpiUtilEzxml.h>
#include <OcpiUtilCommandLineConfiguration.h>

#include <string>
#include <vector>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#include <cctype>
#include <cstring>

#include <errno.h>
#include <stdint.h>

namespace
{
  enum PropertyType
  {
    OCPI_none, // 0 isn't a valid type
    OCPI_Bool,
    OCPI_Char,
    OCPI_UChar,
    OCPI_Short,
    OCPI_UShort,
    OCPI_Long,
    OCPI_ULong,
    OCPI_LongLong,
    OCPI_ULongLong,
    OCPI_Float,
    OCPI_Double,
    OCPI_String,
    OCPI_data_type_limit
  };

  const char* propertyTypes [ ] =
  {
    "None",
    "Bool",
    "Char",
    "UChar",
    "Short",
    "UShort",
    "Long",
    "ULong",
    "LongLong",
    "ULongLong",
    "Float",
    "Double",
    "String",
    0
  };

  const char* propertyTypeStr [ ] =
  {
    "RCCNone",
    "uint8_t", /* Bool */
    "int8_t",
    "uint8_t",
    "int16_t",
    "uint16_t",
    "int32_t",
    "uint32_t",
    "int64_t",
    "uint64_t",
    "float",
    "double",
    "char",
    0
  };

  enum PortType
  {
    NoPort,
    WCIPort,
    XmHCBPort
  };

  struct WCI
  {
    WCI ( )
      : name ( 0 ),
        sizeOfConfigSpace ( 0 ),
        writableConfigProperties ( false ),
        readableConfigProperties ( false ),
        sub32BitConfigProperties ( false ),
        control_ops ( 0 )
    {
      // Empty
    }
    const char* name;
    std::size_t sizeOfConfigSpace;
    bool writableConfigProperties;
    bool readableConfigProperties;
    bool sub32BitConfigProperties;
    uint32_t control_ops; // bit mask
  };

  struct XmHCB
  {
    const char* file_name;
    const char* filespec;
    uint32_t type;
    const char* buf_type;
    const char* format;
    uint32_t xunits;
    double size;
    double xstart;
    double xdelta;
    uint32_t yunits;
    double ystart;
    double ydelta;
  };

  struct Port
  {
    Port ( )
      : port_type ( NoPort ),
        count ( 0 ),
        producer ( false ),
        optional ( false )
    {
      // Empty
    }

    std::string name;
    PortType port_type;
    uint32_t count;
    bool producer;
    bool optional;
    union
    {
      XmHCB hcb;
    };
  };

  struct XmCommandLine
  {
    XmCommandLine ()
    : name ( 0 ),
      argument_idx ( 0 ),
      property ( 0 ),
      value ( 0 ),
      data_type ( OCPI_none )

    {
      // Empty
    }
    const char* name;
    uint32_t argument_idx;
    const char* property;
    const char* value;
    PropertyType data_type;
  };

  struct XmWidget
  {
    XmWidget ()
    : name ( 0 ),
      property ( 0 ),
      value ( 0 ),
      data_type ( OCPI_none )

    {
      // Empty
    }
    const char* name;
    const char* property;
    const char* value;
    PropertyType data_type;
  };

  struct XmResult
  {
    XmResult ()
    : name ( 0 ),
      property ( 0 ),
      value ( 0 ),
      data_type ( OCPI_none )

    {
      // Empty
    }
    const char* name;
    const char* property;
    const char* value;
    PropertyType data_type;
  };

  struct XmSwitch
  {
    XmSwitch ()
    : name ( 0 ),
      property ( 0 ),
      value ( 0 ),
      data_type ( OCPI_none )

    {
      // Empty
    }
    const char* name;
    const char* property;
    const char* value;
    PropertyType data_type;
  };

  struct Simple
  {
    Simple ( )
      : type ( OCPI_none ),
        is_sequence ( false ),
        n_bytes ( 0 ),
        align ( 0 ),
        offset ( 0 ),
        string_length ( 0 ),
        array_n_bytes ( 0 )
    {
      // Empty
    }
    std::string name;
    PropertyType type;
    bool is_sequence;
    std::size_t n_bytes;
    std::size_t align;
    std::size_t offset;
    uint32_t string_length;
    uint32_t array_n_bytes;
  };

  struct Property
  {
    Property ( )
      : isReadable ( 0 ),
        isWritable ( 0 ),
        isStruct ( 0 ),
        n_types ( 0 ), // for struct REMOVE
        n_bytes ( 0 ),
        offset ( 0 )
    {
      // Empty
    }

    std::string name;
    bool isReadable;
    bool isWritable;
    bool isStruct;
    std::size_t n_types; // for struct REMOVE
    std::size_t n_bytes;
    std::size_t offset;
    std::vector<Simple> types;
  };

  struct XmComponentImpl
  {
    XmComponentImpl ( )
      : offset ( 0 )
    {
     // Empty
    }

    std::string impl_name;
    std::string spec_name;
    std::string language;
    std::size_t offset;
    std::vector<Property> properties;
    std::vector<Port> ports;
    std::vector<XmSwitch> switches;
    std::vector<XmWidget> widgets;
    std::vector<XmResult> results;
    std::vector<XmCommandLine> commandline;
    WCI wci;
  };

  /*
    Borrowing "after configure for the X-Midas "finish" control operation
  */
  enum ControlOp
  {
    OCPI_CONTROL_OP_INITIALIZE   = ( 1 << 0 ),
    OCPI_CONTROL_OP_START        = ( 1 << 1 ),
    OCPI_CONTROL_OP_STOP         = ( 1 << 2 ),
    OCPI_CONTROL_OP_RELEASE      = ( 1 << 3 ),
    OCPI_CONTROL_OP_BEFORE_QUERY = ( 1 << 4 ),
    OCPI_CONTROL_OP_AFTER_CONFIG = ( 1 << 5 ),
    OCPI_CONTROL_OP_TEST         = ( 1 << 6 ),
    OCPI_CONTROL_OP_NONE         = ( 1 << 7 ),
    OCPI_CONTROL_OP_FINISH = OCPI_CONTROL_OP_AFTER_CONFIG
  };

  const char* control_op_str [ ] =
  {
    "initialize",
    "start",
    "stop",
    "release",
    "beforeQuery",
    "finish", /* "afterConfigure", */
    "finish"
    "test",
    0,
  };

  std::size_t tsize [ OCPI_data_type_limit ] =
  {
     0,
     8/CHAR_BIT,
     8/CHAR_BIT,
     8/CHAR_BIT,
    16/CHAR_BIT,
    16/CHAR_BIT,
    32/CHAR_BIT,
    32/CHAR_BIT,
    64/CHAR_BIT,
    64/CHAR_BIT,
    32/CHAR_BIT,
    64/CHAR_BIT,
     8/CHAR_BIT
  };

  class OcpiXmConfigurator : public OCPI::Util::CommandLineConfiguration
  {
    public:

      OcpiXmConfigurator ();

      bool help;

      bool verbose;

    private:

      static CommandLineConfiguration::Option options [ ];
  };

  OcpiXmConfigurator::OcpiXmConfigurator ( )
    : OCPI::Util::CommandLineConfiguration ( options ),
      help ( false ),
      verbose ( false )
  {
    // Empty
  }

  OCPI::Util::CommandLineConfiguration::Option OcpiXmConfigurator::options [ ] =
  {
    { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
      "verbose",
      "Be verbose",
      OCPI_CLC_OPT ( &OcpiXmConfigurator::verbose ),
      0
    },
    { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
      "help",
      "This message",
      OCPI_CLC_OPT ( &OcpiXmConfigurator::help ),
      0
    },
    { OCPI::Util::CommandLineConfiguration::OptionType::END,
      "end",
      "End",
      0,
      0
    }
  };

  void print_usage ( OcpiXmConfigurator* config,
                     const char* argv0 )
  {
    std::cout << "\n\nUsage: "
              << argv0
              << " [options] <component implementation XML>"
              << std::endl
              << "  options:"
              << std::endl;
    config->printOptions ( std::cout );
  }

  int parse_commandline ( int argc,
                          char* argv [ ],
                          OcpiXmConfigurator* config )
  {
    try
    {
      config->configure ( argc, argv );
    }
    catch ( const std::string& oops )
    {
      std::cerr << "Oops: " << oops << std::endl;
      std::string err_msg = "Unable to parse command line arguments : ";
      err_msg += oops;
      throw err_msg;
    }

    if ( config->help || ( argc != 2 ) )
    {
      print_usage ( config, argv [ 0 ] );
      return -1;
    }

    return 0;
  }

  void check_attrs ( ezxml_t x, ...)
  {
    if ( !x->attr )
    {
      return;
    }

    va_list ap;

    for ( char** a = x->attr; *a; a += 2 )
    {
      char* p;
      va_start ( ap, x );
      while ( ( p = va_arg ( ap, char* ) ) )
      {
        if ( !strcmp ( p, *a ) )
        {
          break;
        }
      }
      va_end ( ap );
      if ( !p )
      {
        std::cout << "Invalid property name: \""
                  << *a
                  << "\" in a "
                  << x->name
                  << " element."
                  << std::endl;

        throw std::string ( "Invalid property name" );
      }

    } /* End: for ( char** a = x->attr; *a; a += 2 ) */

    return;
  }

  ezxml_t get_xml_file_root ( const char* filename,
                              OCPI::Util::EzXml::Doc& doc )
  {
    OCPI::Util::FileFs::FileFs file_fs ( "/" );

    const std::string fs_filename ( file_fs.fromNativeName ( filename ) );

    ezxml_t xml = doc.parse ( file_fs, fs_filename );

    if ( !xml || !xml->name )
    {
      throw std::string ( "Can't parse XML file" );
    }

    return xml;
  }

  void parse_tag_component_implementation_attr ( ezxml_t& xml,
                                                 XmComponentImpl* impl )
  {
    if ( std::string ( xml->name ) != std::string ( "XmImplementation" ) )
    {
      throw std::string ( "File is not a XmImplementation" );
    }

    const char* name = ezxml_attr ( xml, "Name" );

    if ( !name )
    {
      throw std::string ( "Name attribute is missing from XmImplementation" );
    }

    impl->impl_name = name;

    const char* language = ezxml_attr ( xml, "Language" );

    if ( !language )
    {
      throw std::string ( "Language attribute is missing from XmImplementation" );
    }

    impl->language = language;

    if ( std::string ( "C" ) != impl->language )
    {
      throw std::string ( "Language attribute is not \"C\" in XmImplementation" );
    }

  }

  ezxml_t get_component_spec_root ( ezxml_t& xml,
                                    OCPI::Util::EzXml::Doc& doc )
  {
    ezxml_t spec = ezxml_child ( xml, "ComponentSpec" );

    if ( !spec )
    {
      ezxml_t xi_include = ezxml_child ( xml, "xi:include" );

      if ( !xi_include )
      {
        throw std::string ( "No ComponentSpec element found in XmImplementation" );
      }

      const char* filename = ezxml_attr ( xi_include, "href" );

      if ( !filename )
      {
        throw std::string ( "No href attribute in xi:include element" );
      }

      spec = get_xml_file_root ( filename, doc );

      if ( std::string ( spec->name ) != std::string ( "ComponentSpec" ) )
      {
        throw std::string ( "File referenced by xi:include href attributed is not a ComponentSpec" );
      }
    }

    return spec;
  }

  void parse_tag_component_spec_attr ( ezxml_t& xml,
                                       XmComponentImpl* impl )
  {
    if ( std::string ( xml->name ) != std::string ( "ComponentSpec" ) )
    {
      throw std::string ( "File referenced by xi:include href attributed is not a ComponentSpec" );
    }

    const char* name = ezxml_attr ( xml, "Name" );

    if ( !name )
    {
      throw std::string ( "Name attribute is missing from ComponentSpec" );
    }

    impl->spec_name = name;
  }

 const char* ezxml_children ( ezxml_t xml,
                              const char* ( *func ) ( ezxml_t child,
                                                      void* arg ),
                              void* arg)
  {
    const char *err;
    for ( xml = xml ? xml->child : NULL; xml; xml = xml->ordered )
    {
      err = ( *func ) ( xml, arg );
      if ( err )
      {
        return err;
      }
    }

    return 0;
  }

  void get_number_f64 ( ezxml_t x,
                        const char* attr,
                        double* np,
                        bool* found,
                        double defaultValue )
  {
    const char* a = ezxml_attr ( x, attr );

    if ( !a )
    {
      if ( found )
      {
        *found = false;
      }
      *np = defaultValue;
      return;
    }

    std::string temp ( a );

    std::stringstream ss ( temp );

    ss >> ( *np );

    if ( found )
    {
      *found = true;
    }
  }

  unsigned long get_number_helper ( const char* s )
  {
    char* endptr;
    errno = 0;
    unsigned long val =  strtoul ( s, &endptr, 0 );

    if ( errno == 0 )
     {
      if (*endptr == 'K' || *endptr == 'k')
      {
        val *= 1024;
      }
      return val;
    }
    return ULONG_MAX;
  }

  const char* get_number ( ezxml_t x,
                           const char* attr,
                           uint32_t* np,
                           bool* found,
                           unsigned long defaultValue )
  {
    const char *a = ezxml_attr ( x, attr );

    if ( !a )
    {
      if ( found )
      {
        *found = false;
      }
      *np = ( uint32_t ) defaultValue;
      return 0;
    }

    unsigned long val = get_number_helper ( a );

    if ( val == ULONG_MAX )
    {
      std::cout << "Bad numeric value: \""
                << a
                << "\" for attribute "
                << attr
                << " in element "
                << x->name
                << std::endl;
      throw std::string ( "Bad numeric value" );
    }

    *np = ( uint32_t ) val;

    if ( found )
    {
      *found = true;
    }

    return 0;
  }

  // get a boolean value whose default is false anyway
  const char* get_boolean ( ezxml_t x,
                            const char* name,
                            bool* b )
  {
    const char* a = ezxml_attr ( x, name );

    if ( a )
    {
      if ( !strcasecmp ( a, "true" ) || !strcmp ( a, "1" ) )
      {
        *b = true;
      }
      else if ( !strcasecmp ( a, "false" )  || !strcmp ( a, "0" ) )
      {
        *b =  false;
      }
      else
      {
        std::cout << "Bad boolean value: \""
                  << a
                  << "\" for attribute "
                  << name
                  << "in element "
                  << x->name
                  << std::endl;

        throw std::string ( "Bad bolean value." );
      }
    }
    else
    {
      *b = false;
    }
    return 0;
  }

  std::size_t roundup ( std::size_t n, std::size_t grain )
  {
    return ( n + grain - 1 ) & ~( grain - 1 );
  }

  const char* get_member ( ezxml_t xp,
                           Simple* t,
                           size_t& max_align,
                           size_t& offset )
  {
    bool found;

    t->name = ezxml_attr ( xp, "Name" );

    if ( t->name.empty ( ) )
    {
      throw std::string ( "Missing Name attribute in Property element" );
    }

    const char* type = ezxml_attr ( xp, "Type" );

    if ( type )
    {
      const char** tp;

      for ( tp = propertyTypes; *tp; tp++ )
      {
        if ( !strcmp ( type, *tp ) )
        {
          t->type = ( PropertyType ) ( tp - propertyTypes );
          t->align = tsize [ t->type ];
          if ( t->align > max_align )
          {
            max_align = t->align;
          }
          break;
        } /* End: if ( !strcmp ( type, *tp ) ) */
      } /* End: for ( tp = propertyTypes; *tp; tp++ ) */

      if ( !*tp )
      {
        throw std::string ( "Unknown property type" );
      }
    }
    else
    {
      t->type = OCPI_ULong;
    }

    if ( t->type == OCPI_String )
    {
      get_number ( xp, "StringLength", &t->string_length, &found, 0 );

      if ( !found )
      {
        throw std::string ( "Missing StringLength attribute for string type" );
      }

      if ( t->string_length == 0 )
      {
        throw std::string ( "StringLength cannot be zero" );
      }
    }
    else if ( ezxml_attr(xp, "StringLength" ) )
    {
      throw std::string ( "StringLength attribute only valid for string types" );
    }

    get_boolean ( xp, "IsSequence", &t->is_sequence );

    get_number ( xp, "ArraySize", &t->array_n_bytes, &found, 0 );

    if ( found && ( t->array_n_bytes == 0 ) )
    {
      throw std::string ( "ArraySize cannot be zero" );
    }

    if ( t->is_sequence && !found )
    {
      throw std::string ( "Missing ArraySize attribute for sequence attribute" );
    }

    t->n_bytes = roundup ( t->type == OCPI_String ? t->string_length + 1
                                                : tsize [ t->type ], 4 );

    if ( t->array_n_bytes )
    {
      t->n_bytes *= t->array_n_bytes;
    }

    if ( t->is_sequence )
    {
      t->n_bytes += t->align > 4 ? t->align : 4;
    }

    offset = roundup ( offset, t->align );

    t->offset = offset;

    offset += t->n_bytes;

    return 0;
  }

  // Process one element of a properties element
  const char* parse_one_property ( ezxml_t prop, void* arg )
  {
    XmComponentImpl* impl = ( XmComponentImpl* ) arg;

    // If it is an "include", basically recurse
    const char *name = ezxml_name ( prop );

    if ( name && !strcmp ( name, "xi:include" ) )
    {
      check_attrs ( prop, "href", 0 );

      const char* ifile = ezxml_attr ( prop, "href" );

      if ( ifile )
      {
        prop = ezxml_parse_file(ifile);
      }

      if ( !prop )
      {
        throw std::string ( "File referenced by xi:include href attribute doesn't parse as XML" );
      }

      name = ezxml_name ( prop );

      if ( !name || strcmp ( name, "Properties" ) )
      {
        throw std::string ( "Properties file does not container \"Properties\" element" );
      }

      return ezxml_children ( prop, parse_one_property, arg );

    } /* End: if ( name && !strcmp ( name, "xi:include" ) )  */

    if ( !name || strcmp ( name, "Property" ) )
    {
      throw std::string ( "Element under Properties is neither Property or xi:include" );
    }

    // Now actually process a property element
    check_attrs ( prop,
                  "Name",
                  "Type",
                  "Readable",
                  "Writable",
                  "IsSequence",
                  "StringLength",
                  "ArraySize",
                  0 );

    Property p;

    p.name = ezxml_attr ( prop, "Name" );

    if ( p.name.empty ( ) )
    {
      throw std::string ( "Missing \"Name\" attribute for property" );
    }

    const char* type = ezxml_attr ( prop, "Type" );

    if ( type && !strcmp ( type, "Struct" ) )
    {
      p.isStruct = true;
      for ( ezxml_t m = ezxml_child ( prop, "Property" );
            m;
            m = ezxml_next ( m ) )
      {
        p.n_types++;
      }
      if ( p.n_types == 0 )
      {
        throw std::string ( "Missing Property elements in Property with type == \"struct\"" );
      }
    }
    else
    {
      p.isStruct = false;
      p.n_types = 1;
    }

    p.types.resize ( p.n_types );

    std::size_t max_align = 4;
    std::size_t my_offset = 0;

    if ( p.isStruct )
    {
      Simple* mem = &( p.types [ 0 ] );

      for ( ezxml_t m = ezxml_child ( prop, "Property" );
            m;
            m = ezxml_next ( m ), mem++ )
      {
        check_attrs ( m, "Name", "Type", "IsSequence", "StringLength", "ArraySize", 0 );

        get_member ( m, mem, max_align, my_offset );
      }
    }
    else
    {
      get_member ( prop, &( p.types [ 0 ] ), max_align, my_offset );
    }

    p.n_bytes = my_offset;
    impl->offset = roundup ( impl->offset, max_align );
    p.offset = impl->offset;
    impl->offset += my_offset;

    get_boolean ( prop, "Readable", &p.isReadable );
    get_boolean ( prop, "Writable", &p.isWritable );

    if ( p.isReadable )
    {
      impl->wci.readableConfigProperties = true;
    }

    if ( p.isWritable )
    {
      impl->wci.writableConfigProperties = true;
    }

    impl->properties.push_back ( p );

    return 0;
  }

  // Called with a Properties element or it may contain both Property
  // subelements or xi:include subelements

  const char* parse_tag_properties ( ezxml_t props,
                                     XmComponentImpl* impl )
  {
    std::string name ( ezxml_name ( props ) );

    if ( name.empty ( ) || ( std::string ( "Properties" ) != name ) )
    {
      throw std::string ( "Properties file does not contain a Properties element" );
    }

    check_attrs ( props, "href", 0 );

    const char* filename = ezxml_attr ( props, "href" );

    if ( filename )
    {
      if ( ezxml_child ( props, "Property" ) )
      {
        throw std::string ( "A Properties element with \"href\" attribute cannot contain Property elements" );
      }

      props = ezxml_parse_file ( filename );

      if ( !props )
      {
        throw std::string ( "File referenced by xi:include href attribute doesn't parse as XML" );
      }

      return parse_tag_properties ( props, impl );
    } // End: if ( filename )

    // So now we have a list of props or includes
    return ezxml_children ( props, parse_one_property, impl );
  }

  void parse_tag_control_interface ( ezxml_t& xml,
                                     XmComponentImpl* impl )
  {
    ezxml_t xctl = ezxml_child ( xml, "ControlInterface" );

    if ( xctl )
    {
      check_attrs ( xctl, "Name", "ControlOperations", 0 );

      impl->wci.name = ezxml_attr ( xctl, "Name" );

      const char* ops = ezxml_attr ( xctl, "ControlOperations" );

      if ( ops )
      {
        char* o;
        char* last = 0;

        while ( ( o = strtok_r ( ( char* ) ops, ", \t", &last ) ) )
        {
          ops = 0;
          unsigned int op = 0;
          const char** p;
          for ( p = control_op_str; *p; p++, op++)
          {
            if ( !strcasecmp ( *p, o ) )
            {
              impl->wci.control_ops |= 1 << op;
              break;
            }
          }
          if ( !*p )
          {
            throw std::string  ( "Invalid control operation name in ControlOperations attribute" );
          }
        } /* End: while ( ( o = strtok_r ( ( char* ) ops, ", \t", &last...  */

        if ( !( impl->wci.control_ops & ( 1 << OCPI_CONTROL_OP_START ) ) )
        {
          throw std::string ( "Missing \"start\" operation in ControlOperations attribute" );
        }

      } /* End: if ( ops ) */

    } /* End: if ( xclt ) */
  }

  void  allocate_ports ( ezxml_t& xml_spec,
                         XmComponentImpl* impl )
  {
    std::size_t n_ports = 0; /* Do not count the control plane port */

    for ( ezxml_t x = ezxml_child ( xml_spec, "DataInterfaceSpec" );
          x;
          x = ezxml_next ( x ) )
    {

      n_ports++;
    }
    impl->ports.resize ( n_ports ); /* Pre-allocate the port elements */
  }

  void parse_tag_data_interface_space ( ezxml_t& xml_spec,
                                        XmComponentImpl* impl )
  {
    Port* d = &( impl->ports [ 0 ] );

    for ( ezxml_t x = ezxml_child ( xml_spec, "DataInterfaceSpec" );
          x;
          x = ezxml_next ( x ), d++ )
    {
      check_attrs ( x, "Name", "Producer", "Optional", "Count", 0 );

      get_number ( x, "Count", &d->count, 0, 0 );

      d->port_type = XmHCBPort;

      d->name = ezxml_attr ( x, "Name" );

      for ( Port* pp = &( impl->ports [ 0 ] ); pp < d; pp++ )
      {
        if ( pp->name == std::string ( d->name ) )
        {
          throw std::string ( "DataInterfaceSpec Name attribute duplicates another interface name" );
        }
      }
      get_boolean ( x, "Producer", &d->producer );
      get_boolean ( x, "Optional", &d->optional );
    }
  }

  void parse_tag_xm_header ( ezxml_t& xml_impl,
                             XmComponentImpl* impl )
  {
    Port* d = &( impl->ports [ 0 ] );

    for ( ezxml_t x = ezxml_child ( xml_impl, "XmHeader" );
          x;
          x = ezxml_next ( x ), d++ )
    {
      // Check that the attributes are valid
      check_attrs ( x, "Name",
                       "filespec",
                       "type",
                       "buf_type",
                       "format",
                       "size",
                       "xunits",
                       "xstart",
                       "xdelta",
                       "yunits",
                       "ystart",
                       "ydelta",
                       0 );

      std::string name = ezxml_attr ( x, "Name" );

      // Check that the header names match the port name
      bool match ( false );

      for ( std::vector<Port>::iterator pp = impl->ports.begin ( );
            pp != impl->ports.end ( );
            pp++ )
      {
        if ( pp->name == name )
        {
          match = true;
          break;
        }
      }

      if ( !match )
      {
        std::cout << "A DataInterfaceSpec is missing for this XmHeader named "
                  << name
                  << std::endl;
        throw std::string ( "A DataInterfaceSpec is missing for this XmHeader" );
      }

      // Pull out the attributes
      for ( std::vector<Port>::iterator pp = impl->ports.begin ( );
            pp != impl->ports.end ( );
            pp++ )
      {
        if ( pp->name == std::string ( d->name ) )
        {
          bzero ( &( d->hcb ), sizeof ( d->hcb ) );
          d->hcb.file_name = ezxml_attr ( x, "Name" );
          d->hcb.format = ezxml_attr ( x, "format" );
          d->hcb.buf_type = ezxml_attr ( x, "buf_type" );
          d->hcb.filespec = ezxml_attr ( x, "filespec" );
          get_number ( x, "type", &( d->hcb.type ), 0, 0 );
          get_number_f64 ( x, "size", &( d->hcb.size ), 0, 0 );
          get_number ( x, "xunits", &( d->hcb.xunits ), 0, 0 );
          get_number_f64 ( x, "xstart", &( d->hcb.xstart ), 0, 0 );
          get_number_f64 ( x, "xdelta", &( d->hcb.xdelta ), 0, 0 );
          get_number ( x, "yunits", &( d->hcb.yunits ), 0, 0 );
          get_number_f64 ( x, "ystart", &( d->hcb.ystart ), 0, 0 );
          get_number_f64 ( x, "ydelta", &( d->hcb.ydelta ), 0, 0 );
          break;
        }
      } /* End: for ( Port* pp = &( impl->ports [ 0 ] ); pp < d; pp++ ) */
    } /* End: for ( ezxml_t x = ezxml_child ( xml_spec, "XmHeader" ); ... */
  }

  PropertyType string2PropertyType ( const char* type )
  {
    if ( type )
    {
      const char** tp;

      for ( tp = propertyTypes; *tp; tp++ )
      {
        if ( !strcmp ( type, *tp ) )
        {
          return ( PropertyType ) ( tp - propertyTypes );
        }
      }

      if ( !*tp )
      {
        throw std::string ( "Unknown property type" );
      }
    }

    return OCPI_ULong;
  }

  void parse_tag_xm_widgets ( ezxml_t& xml_impl,
                              XmComponentImpl* impl )
  {
    ezxml_t args = ezxml_child ( xml_impl, "XmWidgets" );

    for ( ezxml_t x = ezxml_child ( args, "XmWidget" );
          x;
          x = ezxml_next ( x ) )
    {
      {
        XmWidget d;

        // Check that the attributes are valid
        check_attrs ( x, "Name",
                         "Property",
                         "Value",
                         "Type",
                         "StringLength",
                         0 );

        d.name = ezxml_attr ( x, "Name" );

        d.value = ezxml_attr ( x, "Value" );

        d.property = ezxml_attr ( x, "Property" );

        // Argument shadows a property
        if ( d.property )
        {
          bool match ( false );

          for ( std::vector<Property>::iterator p = impl->properties.begin ( );
                p != impl->properties.end ( );
                p++ )
          {
            if ( p->name == std::string ( d.property ) )
            {
              match = true;
              break;
            }
          }

          if ( !match )
          {
            std::cout << "A Property is missing for this XmWidget named "
                      << d.name
                      << std::endl;
            throw std::string ( "A Property is missing for this XmWidget" );
          }

        } // End: if ( d->property )
        else
        {
          if ( !d.value )
          {
            std::cout << "XmWidget named "
                      << d.name
                      << " is not a property and is missing its value"
                      << std::endl;
            throw std::string ( "A value is missing for this XmWidget" );
          }

          const char* type = ezxml_attr ( x, "Type" );

          d.data_type = string2PropertyType ( type );

          if ( !type )
          {
            std::cout << "XmWidget named "
                      << d.name
                      << " is not a property and is missing its data type"
                      << std::endl;
            throw std::string ( "A data type is missing for this XmWidget" );
          }

        }

        // Add the element to the list
        impl->widgets.push_back ( d );
      }
    } /* End: for ( ezxml_t x = ezxml_child ( xml_spec, "XmHeader" ); ... */
  }

  void parse_tag_xm_results ( ezxml_t& xml_impl,
                              XmComponentImpl* impl )
  {
    ezxml_t args = ezxml_child ( xml_impl, "XmResults" );

    for ( ezxml_t x = ezxml_child ( args, "XmResult" );
          x;
          x = ezxml_next ( x ) )
    {
      {
        XmResult d;

        // Check that the attributes are valid
        check_attrs ( x, "Name",
                         "Property",
                         "Value",
                         "Type",
                         "StringLength",
                         0 );

        d.name = ezxml_attr ( x, "Name" );

        d.value = ezxml_attr ( x, "Value" );

        d.property = ezxml_attr ( x, "Property" );

        // Argument shadows a property
        if ( d.property )
        {
          bool match ( false );

          for ( std::vector<Property>::iterator p = impl->properties.begin ( );
                p != impl->properties.end ( );
                p++ )
          {
            if ( p->name == std::string ( d.property ) )
            {
              match = true;
              break;
            }
          }

          if ( !match )
          {
            std::cout << "A Property is missing for this XmResult named "
                      << d.name
                      << std::endl;
            throw std::string ( "A Property is missing for this XmResult" );
          }

        } // End: if ( d->property )
        else
        {
          if ( !d.value )
          {
            std::cout << "XmResult named "
                      << d.name
                      << " is not a property and is missing its value"
                      << std::endl;
            throw std::string ( "A value is missing for this XmResult" );
          }

          const char* type = ezxml_attr ( x, "Type" );

          d.data_type = string2PropertyType ( type );

          if ( !type )
          {
            std::cout << "XmResult named "
                      << d.name
                      << " is not a property and is missing its data type"
                      << std::endl;
            throw std::string ( "A data type is missing for this XmResult" );
          }

        }

        // Add the element to the list
        impl->results.push_back ( d );
      }
    } /* End: for ( ezxml_t x = ezxml_child ( xml_spec, "XmHeader" ); ... */
  }


  void parse_tag_xm_switches ( ezxml_t& xml_impl,
                               XmComponentImpl* impl )
  {
    ezxml_t args = ezxml_child ( xml_impl, "XmSwitches" );

    for ( ezxml_t x = ezxml_child ( args, "XmSwitch" );
          x;
          x = ezxml_next ( x ) )
    {
      {
        XmSwitch d;

        // Check that the attributes are valid
        check_attrs ( x, "Name",
                         "Property",
                         "Value",
                         "Type",
                         "StringLength",
                         0 );

        d.name = ezxml_attr ( x, "Name" );

        d.value = ezxml_attr ( x, "Value" );

        d.property = ezxml_attr ( x, "Property" );

        // Argument shadows a property
        if ( d.property )
        {
          bool match ( false );

          for ( std::vector<Property>::iterator p = impl->properties.begin ( );
                p != impl->properties.end ( );
                p++ )
          {
            if ( p->name == std::string ( d.property ) )
            {
              match = true;
              break;
            }
          }

          if ( !match )
          {
            std::cout << "A Property is missing for this XmSwitch named "
                      << d.name
                      << std::endl;
            throw std::string ( "A Property is missing for this XmSwitch" );
          }

        } // End: if ( d->property )
        else
        {
          if ( !d.value )
          {
            std::cout << "XmSwitch named "
                      << d.name
                      << " is not a property and is missing its value"
                      << std::endl;
            throw std::string ( "A value is missing for this XmSwitch" );
          }

          const char* type = ezxml_attr ( x, "Type" );

          d.data_type = string2PropertyType ( type );

          if ( !type )
          {
            std::cout << "XmSwitch named "
                      << d.name
                      << " is not a property and is missing its data type"
                      << std::endl;
            throw std::string ( "A data type is missing for this XmSwitch" );
          }

        }

        // Add the element to the list
        impl->switches.push_back ( d );
      }
    }
  }


  void parse_tag_xm_commandline ( ezxml_t& xml_impl,
                                  XmComponentImpl* impl )
   {
    ezxml_t args = ezxml_child ( xml_impl, "XmCommandLineArguments" );

    for ( ezxml_t x = ezxml_child ( args, "XmCommandLineArgument" );
          x;
          x = ezxml_next ( x ) )
    {
      {
        XmCommandLine d;

        // Check that the attributes are valid
        check_attrs ( x, "Name",
                         "Property",
                         "Value",
                         "Type",
                         "Argument",
                         "StringLength",
                         0 );

        d.name = ezxml_attr ( x, "Name" );

        d.value = ezxml_attr ( x, "Value" );

        d.property = ezxml_attr ( x, "Property" );

        get_number ( x, "Argument", &d.argument_idx, 0, 0 );

        // Argument shadows a property
        if ( d.property )
        {
          bool match ( false );

          for ( std::vector<Property>::iterator p = impl->properties.begin ( );
                p != impl->properties.end ( );
                p++ )
          {
            if ( p->name == std::string ( d.property ) )
            {
              match = true;
              break;
            }
          }

          if ( !match )
          {
            std::cout << "A Property is missing for this XmCommandLineArgument named "
                      << d.name
                      << std::endl;
            throw std::string ( "A Property is missing for this XmCommandLineArgument" );
          }

        } // End: if ( d->property )
        else
        {

          if ( !d.value )
          {
            std::cout << "XmCommandLineArgument named "
                      << d.name
                      << " is not a property and is missing its value"
                      << std::endl;
            throw std::string ( "A value is missing for this XmCommandLineArgument" );
          }

          const char* type = ezxml_attr ( x, "Type" );

          d.data_type = string2PropertyType ( type );

          if ( !type )
          {
            std::cout << "XmCommandLineArgument named "
                      << d.name
                      << " is not a property and is missing its data type"
                      << std::endl;
            throw std::string ( "A data type is missing for this XmCommandLineArgument" );
          }

        }

        // Add the element to the list
        impl->commandline.push_back ( d );
      }
    }
  }

  void parse_component_xml ( ezxml_t& xml_impl,
                             ezxml_t& xml_spec,
                             XmComponentImpl* impl )
  {
    /* ---- Parse XmImplementation tag attributes ----------------------- */

    parse_tag_component_implementation_attr ( xml_impl, impl );

    parse_tag_control_interface ( xml_impl, impl );

    /* ---- Parse componentSpec tag attributes -------------------------- */

    parse_tag_component_spec_attr ( xml_spec, impl );

    ezxml_t props = ezxml_child ( xml_spec, "Properties" );

    allocate_ports ( xml_spec, impl );

    parse_tag_properties ( props, impl );

    parse_tag_data_interface_space ( xml_spec, impl );

    parse_tag_xm_header ( xml_impl, impl );

    parse_tag_xm_widgets ( xml_impl, impl );

    parse_tag_xm_results ( xml_impl, impl );

    parse_tag_xm_switches ( xml_impl, impl );

    parse_tag_xm_commandline ( xml_impl, impl );
#if 0
    std::cout << "\nWorker"
              << "\nimpl name "
              << impl->impl_name
              << "\nspec name "
              << impl->spec_name
              << "\nlanguage  "
              << impl->language
              << "\noffset    "
              << impl->offset
              << std::endl;

     for ( std::vector<Property>::const_iterator itr = impl->properties.begin ();
           itr != impl->properties.end ();
           ++itr )
    {
      std::cout << "\nProperty"
                << "\n  name      "
                << itr->name
                << "\n isReadable "
                << itr->isReadable
                << "\n isWritable "
                << itr->isWritable
                << "\n isStruct   "
                << itr->isStruct
                << std::endl;

      for ( std::vector<Simple>::const_iterator t = itr->types.begin ();
             t != itr->types.end ();
             ++t )
      {
        std::cout << "\nSimple"
                  << "\n name          "
                  << t->name
                  << "\ntype           "
                  << propertyTypes [ t->type ]
                  << "\n is_sequence   "
                  << t->is_sequence
                  << "\n n_bytes       "
                  << t->n_bytes
                  << "\n align         "
                  << t->align
                  << "\n offset        "
                  << t->offset
                  << "\n string_length "
                  << t->string_length
                  << "\n array_n_bytes "
                  << t->array_n_bytes
                  << std::endl;
      }
    }

    std::cout << "2 impl->commandline.size() = " << impl->commandline.size ( ) << std::endl;

     for ( std::vector<XmCommandLine>::const_iterator itr = impl->commandline.begin ();
           itr != impl->commandline.end ();
           ++itr )
    {
      std::cout << "\nXmCommandLine"
                << "\n name          "
                << itr->name
                << "\n Argument      "
                << itr->argument_idx
                << "\n Property    "
                <<  ( ( itr->property ) ? itr->property : "(null)" )
                << "\n value         "
                <<  ( ( itr->value ) ? itr->value : "(null)" )
                << std::endl;
    }

    std::cout << "2 impl->widgets.size() = " << impl->widgets.size ( ) << std::endl;

     for ( std::vector<XmWidget>::const_iterator itr = impl->widgets.begin ();
           itr != impl->widgets.end ();
           ++itr )
    {
      std::cout << "\nXmWidget"
                << "\n name          "
                << itr->name
                << "\n Property    "
                <<  ( ( itr->property ) ? itr->property : "(null)" )
                << "\n value         "
                <<  ( ( itr->value ) ? itr->value : "(null)" )
                << std::endl;
    }

    std::cout << "2 impl->results.size() = " << impl->results.size ( ) << std::endl;

     for ( std::vector<XmResult>::const_iterator itr = impl->results.begin ();
           itr != impl->results.end ();
           ++itr )
    {
      std::cout << "\nXmResult"
                << "\n name          "
                << itr->name
                << "\n Property    "
                <<  ( ( itr->property ) ? itr->property : "(null)" )
                << "\n value         "
                <<  ( ( itr->value ) ? itr->value : "(null)" )
                << std::endl;
    }

    std::cout << "2 impl->switches.size() = " << impl->switches.size ( ) << std::endl;

     for ( std::vector<XmSwitch>::const_iterator itr = impl->switches.begin ();
           itr != impl->switches.end ();
           ++itr )
    {
      std::cout << "\nXmSwitches"
                << "\n name          "
                << itr->name
                << "\n Property    "
                <<  ( ( itr->property ) ? itr->property : "(null)" )
                << "\n value         "
                <<  ( ( itr->value ) ? itr->value : "(null)" )
                << std::endl;
    }

    std::cout << "\nWCI Port"
              << "\n name                     "
              << impl->wci.name
              << "\n sizeOfConfigSpace        "
              << impl->wci.sizeOfConfigSpace
              << "\n writableConfigProperties "
              << impl->wci.writableConfigProperties
              << "\n readableConfigProperties "
              << impl->wci.readableConfigProperties
              << "\n sub32BitConfigProperties "
              << impl->wci.sub32BitConfigProperties
              << "\n control_ops              "
              << std::hex
              << impl->wci.control_ops
              << std::endl;

    for ( std::vector<Port>::const_iterator p = impl->ports.begin ();
           p != impl->ports.end ();
           ++p )
    {
      std::cout << "\nPorts"
                << "\n name         "
                << p->name
                << "\n port_type    "
                << p->port_type
                << "\n count        "
                << p->count
                << "\n producer     "
                << p->producer
                << "\n optional     "
                << p->optional
                << "\n HCB filename "
                << p->hcb.file_name
                << "\n HCB filespec "
                << p->hcb.filespec
                << "\n HCB type     "
                << p->hcb.type
                << "\n HCB buf_type "
                << p->hcb.buf_type
                << "\n HCB format   "
                << p->hcb.format
                << "\n HCB size     "
                << p->hcb.size
                << "\n HCB xunits   "
                << p->hcb.xunits
                << "\n HCB xstart   "
                << p->hcb.xstart
                << "\n HCB xdelta   "
                << p->hcb.xdelta
                << "\n HCB yunits   "
                << p->hcb.yunits
                << "\n HCB ystart   "
                << p->hcb.ystart
                << "\n HCB ydelta   "
                << p->hcb.ydelta
                << std::endl;
    }
#endif
    return;
  }

  void create_output_stream ( const char* filename,
                              std::ofstream& out )
  {
    OCPI::Util::FileFs::FileFs file_fs ( "/" );

    const std::string fs_filename ( file_fs.fromNativeName ( filename ) );

    out.open ( fs_filename.c_str ( ), std::ios::trunc );
  }

  std::string genterate_xm_proxy_filename ( const std::string& basename,
                                            const std::string& suffix )
  {
    std::string base ( basename );

    std::transform ( base.begin ( ),
                     base.end ( ),
                     base.begin ( ),
                     ::tolower );

    std::string filename ( base + suffix );

    return filename;
  }

  void generate_xm_proxy_include_header ( const XmComponentImpl* impl,
                                          std::ofstream& out )
  {
    std::string base ( impl->impl_name );

    std::transform ( base.begin ( ),
                     base.end ( ),
                     base.begin ( ),
                     ::toupper );

    out << "\n"
        << "#ifndef INCLUDED_OCPI_XM_PROXY_" << base << "_H\n"
        << "#define INCLUDED_OCPI_XM_PROXY_" << base << "_H\n"
        << "\n"
        << "#include <RCC_Worker.h>\n"
        << "\n"
        << "#ifdef __cplusplus\n"
        << "  extern \"C\" {\n"
        << "#endif\n"
        << std::endl;
  }

  const std::string make_port_name ( const XmComponentImpl* impl,
                                     std::vector<Port>::const_iterator port )
  {
    std::string port_name_uc ( port->name );

    std::transform ( port_name_uc.begin ( ),
                     port_name_uc.end ( ),
                     port_name_uc.begin ( ),
                     ::toupper );

    std::string base ( impl->impl_name );

    std::transform ( base.begin ( ),
                     base.end ( ),
                     base.begin ( ),
                     ::toupper );

    std::string port_name ( base + std::string ( "_PORT_" ) + port_name_uc );

    return port_name;
  }

  std::size_t get_number_of_target_ports ( const XmComponentImpl* impl )
  {
    std::size_t n_target_ports ( 0 );

    for ( std::vector<Port>::const_iterator p = impl->ports.begin ();
           p != impl->ports.end ();
           ++p )
    {
      if ( !p->producer )
      {
        n_target_ports++;
      }
    }

    return n_target_ports;
  }

  std::size_t get_number_of_source_ports ( const XmComponentImpl* impl )
  {
    std::size_t n_source_ports ( 0 );

    for ( std::vector<Port>::const_iterator p = impl->ports.begin ();
           p != impl->ports.end ();
           ++p )
    {
      if ( p->producer )
      {
        n_source_ports++;
      }
    }

    return n_source_ports;
  }

  void generate_xm_proxy_include_port_ordinals ( const XmComponentImpl* impl,
                                                 std::ofstream& out )
  {
    std::string base ( impl->impl_name );

    std::transform ( base.begin ( ),
                     base.end ( ),
                     base.begin ( ),
                     ::toupper );

    out << "/* ---- Worker port ordinals --------------------------------------------- */\n"
        << "\n"
        << "enum " << impl->impl_name << "PortOrdinal\n"
        << "{\n";

    std::size_t n_source_ports = get_number_of_source_ports ( impl );
    std::size_t n_target_ports = get_number_of_target_ports ( impl );

    size_t idx ( 0 );

    for ( std::vector<Port>::const_iterator p = impl->ports.begin ();
           p != impl->ports.end ();
           ++p )
    {
      out << "  " << make_port_name ( impl, p )
          << " = ( " << idx++ << " )";

      if ( ( p + 1 ) != impl->ports.end () )
      {
        out << ",";
      }

      out << "\n";

    } // End: for ( std::vector<Port>::const_iterator p = impl->ports ... */

    out << "};\n"
        << std::endl;


    if ( n_target_ports )
    {
      out << "#define " << base << "_N_TARGET_PORTS ( "
          << n_target_ports
          << " ) /* Input ports */\n";
    }

    if ( n_source_ports )
    {
      out << "#define " << base << "_N_SOURCE_PORTS ( "
          << n_source_ports
          << " ) /* Output ports */\n";
    }

    out << std::endl;
  }

  const std::string get_xm_prop_type_prefix ( const XmComponentImpl* impl,
                                              const std::string& name )
  {
    for ( std::vector<XmCommandLine>::const_iterator itr = impl->commandline.begin ();
          itr != impl->commandline.end ();
          ++itr )
    {
      if ( std::string ( itr->name ) == name )
      {
        return std::string ( "cmd_" );
      }
    }

    for ( std::vector<XmSwitch>::const_iterator itr = impl->switches.begin ();
          itr != impl->switches.end ();
          ++itr )
    {
      if ( std::string ( itr->name ) == name )
      {
        return std::string ( "swt_" );
      }
    }

    for ( std::vector<XmWidget>::const_iterator itr = impl->widgets.begin ();
          itr != impl->widgets.end ();
          ++itr )
    {
      if ( std::string ( itr->name ) == name )
      {
        return std::string ( "wid_" );
      }
    }

    for ( std::vector<XmResult>::const_iterator itr = impl->results.begin ();
          itr != impl->results.end ();
          ++itr )
    {
      if ( std::string ( itr->name ) == name )
      {
        return std::string ( "rlt_" );
      }
    }


    std::cout << "Unable to identify the argument type of " << name << std::endl;

    // Should not get here
    throw std::string ( "Missing argument designator for this property" );

    return std::string ( "xxx_" );
  }

  int space_to_underscore ( char c )
  {
    return ( c == ' ' ) ? '_' : c;
  }


  const std::string make_property_name ( const XmComponentImpl* impl,
                                         const std::string& name )
  {
    ( void ) impl;

    std::string no_spaces_name ( name );

    std::transform ( no_spaces_name.begin ( ),
                     no_spaces_name.end ( ),
                     no_spaces_name.begin ( ),
                     space_to_underscore );

// REMOVE    return get_xm_prop_type_prefix ( impl, name ) + no_spaces_name;
    return no_spaces_name;
  }

  void generate_xm_proxy_include_properties ( const XmComponentImpl* impl,
                                              std::ofstream& out )
  {

    out << "/* ---- Worker property space -------------------------------------------- */\n"
        << std::endl
        << "typedef struct\n"
        << "{\n";

     for ( std::vector<Property>::const_iterator itr = impl->properties.begin ();
           itr != impl->properties.end ();
           ++itr )
    {
      if ( itr->isStruct )
      {
        out << "  struct \n"
            << "  {\n";
        for ( std::vector<Simple>::const_iterator t = itr->types.begin ();
               t != itr->types.end ();
               ++t )
        {
          out << "    "
              << propertyTypeStr [ t->type ]
              << " "
              << t->name;

          size_t length ( 0 );
          if ( t->string_length )
          {
            length = t->string_length;
          }
          else if ( t->array_n_bytes )
          {
            length = t->array_n_bytes;
           }

          if ( length )
          {
            out << " [ " << length << " ]";
          }

          out << ";\n";

        } // End: for ( std::vector<Simple>::const_iterator t = itr->... */

        out << "  } " << itr->name << "\n";

      }
      else
      {
        for ( std::vector<Simple>::const_iterator t = itr->types.begin ();
               t != itr->types.end ();
               ++t )
        {
          out << "  "
              << propertyTypeStr [ t->type ]
              << " "
              << make_property_name ( impl, itr->name );

          size_t length ( 0 );

          if ( t->string_length )
          {
            length = t->string_length;
          }
          else if ( t->array_n_bytes )
          {
            length = t->array_n_bytes;
          }

          if ( length )
          {
            out << " [ " << length << " ]";
          }

          out << ";\n";
        }
      }
    }
    out << "} " << impl->impl_name << "WorkerProperties;\n"
        << std::endl;

  }

  void generate_xm_proxy_include_entry_point ( const XmComponentImpl* impl,
                                               std::ofstream& out )
  {
    out << "/* ---- Worker entry point ----------------------------------------------- */\n"
        << "\n"
        << "extern RCCDispatch " << impl->impl_name << "WorkerDispatchTable;\n"
        << std::endl;
  }

  void generate_xm_proxy_include_footer ( const XmComponentImpl* impl,
                                          std::ofstream& out )
  {
    std::string base ( impl->impl_name );

    std::transform ( base.begin ( ),
                     base.end ( ),
                     base.begin ( ),
                     ::toupper );

    out << "#ifdef __cplusplus\n"
        << "  } /* End: extern \"C\" */\n"
        << "#endif\n"
        << "\n"
        << "#endif /* End: #ifndef INCLUDED_OCPI_XM_PROXY_" << base << "_H */\n"
        << std::endl;
  }

  void generate_xm_proxy_include ( XmComponentImpl* impl )
  {
    std::ofstream out;

    std::string filename = genterate_xm_proxy_filename ( impl->impl_name,
                                                         "_Worker.h" );

    create_output_stream ( filename.c_str ( ), out );

    generate_xm_proxy_include_header ( impl, out );

    generate_xm_proxy_include_port_ordinals ( impl, out );

    generate_xm_proxy_include_properties ( impl, out );

    generate_xm_proxy_include_entry_point ( impl, out );

    generate_xm_proxy_include_footer ( impl, out );
  }

  void generate_xm_proxy_source_header ( const XmComponentImpl* impl,
                                         std::ofstream& out )
  {
    std::string spec_name ( impl->spec_name );

    std::transform ( spec_name.begin ( ),
                     spec_name.end ( ),
                     spec_name.begin ( ),
                     ::tolower );

    std::string impl_name ( impl->impl_name );

    std::transform ( impl_name.begin ( ),
                     impl_name.end ( ),
                     impl_name.begin ( ),
                     ::tolower );

    out << "\n"
        << "#include \"" << impl_name << "_Worker.h\"\n"
        << "\n"
        << "#include \"ocpi_xm_port_table.h\"\n"
        << "#include \"ocpi_xm_property_table.h\"\n"
        << "#include \"ocpi_xm_execution_context.h\"\n"
        << "\n"
        << "#include <stdio.h>\n"
        << "#include <assert.h>\n"
        << "#include <stdlib.h>\n"
        << "#include <string.h>\n"
        << "\n"
        << "extern void ocpi_xm_mainroutine_"
        << spec_name << " ( RCCWorker* );\n"
        << std::endl;
  }

  const std::string get_xm_data_type_def ( const std::vector<Simple>& types )
  {
    if ( types.size ( ) != 1 )
    {
      throw std::string ( "At this time we only support simple types.\n" );
    }

    std::string type ( propertyTypes [ types [ 0 ].type ] );

    if ( type == std::string ( "String" ) )
    {
      return std::string ( "XM_PROP_TYPE_STRING" );
    }
    else if ( type == std::string ( "Float" ) )
    {
      return std::string ( "XM_PROP_TYPE_F32" );
    }
    else if ( type == std::string ( "Double" ) )
    {
      return std::string ( "XM_PROP_TYPE_F64" );
    }
    else if ( type == std::string ( "Char" ) )
    {
      return std::string ( "XM_PROP_TYPE_S8" );
    }
    else if ( type == std::string ( "UChar" ) )
    {
      return std::string ( "XM_PROP_TYPE_U8" );
    }
    else if ( type == std::string ( "Short" ) )
    {
      return std::string ( "XM_PROP_TYPE_S16" );
    }
    else if ( type == std::string ( "Ushort" ) )
    {
      return std::string ( "XM_PROP_TYPE_U16" );
    }
    else if ( type == std::string ( "Long" ) )
    {
      return std::string ( "XM_PROP_TYPE_S32" );
    }
    else if ( type == std::string ( "ULong" ) )
    {
      return std::string ( "XM_PROP_TYPE_U32" );
    }
    else if ( type == std::string ( "LongLong" ) )
    {
      return std::string ( "XM_PROP_TYPE_S64" );
    }
    else if ( type == std::string ( "ULongLong" ) )
    {
      return std::string ( "XM_PROP_TYPE_U64" );
    }
    else if ( type == std::string ( "Bool" ) )
    {
      return std::string ( "XM_PROP_TYPE_BOOL" );
    }

    return std::string ( "Unknown" );
  }

  const std::string get_xm_prop_type_def ( const XmComponentImpl* impl,
                                           const std::string name )
  {
    for ( std::vector<XmCommandLine>::const_iterator itr = impl->commandline.begin ();
          itr != impl->commandline.end ();
          ++itr )
    {
      if ( std::string ( itr->name ) == name )
      {
        return std::string ( "XM_PROP_COMMANDLINE" );
      }
    }

    for ( std::vector<XmSwitch>::const_iterator itr = impl->switches.begin ();
          itr != impl->switches.end ();
          ++itr )
    {
      if ( std::string ( itr->name ) == name )
      {
        return std::string ( "XM_PROP_SWITCH" );
      }
    }

    for ( std::vector<XmWidget>::const_iterator itr = impl->widgets.begin ();
          itr != impl->widgets.end ();
          ++itr )
    {
      if ( std::string ( itr->name ) == name )
      {
        return std::string ( "XM_PROP_WIDGET" );
      }
    }

    for ( std::vector<XmResult>::const_iterator itr = impl->results.begin ();
          itr != impl->results.end ();
          ++itr )
    {
      if ( std::string ( itr->name ) == name )
      {
        return std::string ( "XM_PROP_RESULT" );
      }
    }

    return std::string ( "Unknown" );
  }

  void generate_xm_proxy_source_propery_table ( const XmComponentImpl* impl,
                                                std::ofstream& out )
  {
    out << "/* ---- Worker property table initialization ----------------------------- */\n"
        << std::endl;

    out << "#define N_PROPERTIES ( "
        << impl->properties.size ( ) << " )\n"
        << std::endl;

    out << "static void init_property_table ( RCCWorker* wctx )\n"
        << "{\n"
        << "  XmPropertyTable* p_tab = ( XmPropertyTable* )\n"
        << "                             wctx->memories [ XM_PROPERTY_TABLE_INDEX ];\n"
        << "\n"
        << "  p_tab->n_properties = N_PROPERTIES;\n";

    size_t idx ( 0 );

    for ( std::vector<Property>::const_iterator itr = impl->properties.begin ();
           itr != impl->properties.end ();
           ++itr )
    {
      out << "\n  XmProperty " << make_property_name ( impl, itr->name ) << " =\n"
          << "  {\n"
          << "    .name = \"" << itr->name << "\",\n"
          << "    .offset = offsetof ( "
          << impl->impl_name << "WorkerProperties, " << make_property_name ( impl, itr->name ) << " ),\n"
          << "    .data_type = " << get_xm_data_type_def ( itr->types ) << ",\n"
//          << "    .xm_prop_type = " << get_xm_prop_type_def ( impl, itr->name ) << "\n"
          << "  };\n"
          << "\n"
          << "  p_tab->properties [ " << idx++ << " ] = "
          << make_property_name ( impl, itr->name )
          << ";\n";
    }

    out << "}\n" << std::endl;
  }

  void generate_xm_proxy_source_commandline_table ( const XmComponentImpl* impl,
                                                    std::ofstream& out )
  {
    out << "/* ---- Worker command line table initialization ------------------------- */\n"
        << std::endl;

    out << "#define N_COMMANDLINE ( "
        << impl->commandline.size ( ) << " )\n"
        << std::endl;

    if ( impl->commandline.size ( ) == 0 )
    {
      return;
    }

    out << "static void init_commandline_table ( RCCWorker* wctx )\n"
        << "{\n"
        << "  XmCommandLineTable* p_tab = ( XmCommandLineTable* )\n"
        << "                              wctx->memories [ XM_COMMANDLINE_TABLE_INDEX ];\n"
        << "\n"
        << "  p_tab->n_arguments = N_COMMANDLINE;\n";

    size_t idx ( 0 );

    for ( std::vector<XmCommandLine>::const_iterator itr = impl->commandline.begin ();
           itr != impl->commandline.end ();
           ++itr )
    {
      {
        Simple s;

        s.type = itr->data_type;

        std::vector<Simple> types;

        types.push_back ( s );

        out << "\n  XmCommandLine " << make_property_name ( impl, itr->name ) << " =\n"
            << "  {\n"
            << "    .name = \"" << itr->name << "\",\n"
            << "    .index = " << itr->argument_idx << ",\n";

        if ( itr->property )
        {
            out << "    .property = \"" << itr->property << "\",\n";
        }
        else
        {
            out << "    .property [ 0 ] = \'\\0\',\n";
        }

        if ( itr->value )
        {
            out << "    .value = \"" << itr->value << "\",\n";
        }
        else
        {
            out << "    .value [ 0 ] = \'\\0\',\n";
        }

        if ( itr->property )
        {
          out << "    .data_type = XM_PROP_TYPE_NONE\n";
        }
        else
        {
          out << "    .data_type = " << get_xm_data_type_def ( types ) << "\n";
        }
      }

      out << "  };\n"
          << "\n"
          << "  p_tab->arguments [ " << idx++ << " ] = "
          << make_property_name ( impl, itr->name )
          << ";\n";
    }

    out << "}\n" << std::endl;
  }


  void generate_xm_proxy_source_widget_table ( const XmComponentImpl* impl,
                                               std::ofstream& out )
  {
    out << "/* ---- Worker widget table initialization ------------------------------- */\n"
        << std::endl;

    out << "#define N_WIDGETS ( "
        << impl->widgets.size ( ) << " )\n"
        << std::endl;

    if ( impl->widgets.size ( ) == 0 )
    {
      return;
    }

    out << "static void init_widgets_table ( RCCWorker* wctx )\n"
        << "{\n"
        << "  XmWidgetTable* p_tab = ( XmWidgetTable* )\n"
        << "                           wctx->memories [ XM_WIDGET_TABLE_INDEX ];\n"
        << "\n"
        << "  p_tab->n_widgets = N_WIDGETS;\n";

    size_t idx ( 0 );

    for ( std::vector<XmWidget>::const_iterator itr = impl->widgets.begin ();
           itr != impl->widgets.end ();
           ++itr )
    {
      {
        Simple s;

        s.type = itr->data_type;

        std::vector<Simple> types;

        types.push_back ( s );

        out << "\n  XmWidget " << make_property_name ( impl, itr->name ) << " =\n"
            << "  {\n"
            << "    .name = \"" << itr->name << "\",\n";

        if ( itr->property )
        {
            out << "    .property = \"" << itr->property << "\",\n";
        }
        else
        {
            out << "    .property [ 0 ] = \'\\0\',\n";
        }

        if ( itr->value )
        {
            out << "    .value = \"" << itr->value << "\",\n";
        }
        else
        {
            out << "    .value [ 0 ] = \'\\0\',\n";
        }

        if ( itr->property )
        {
          out << "    .data_type = XM_PROP_TYPE_NONE\n";
        }
        else
        {
          out << "    .data_type = " << get_xm_data_type_def ( types ) << "\n";
        }
      }

      out << "  };\n"
          << "\n"
          << "  p_tab->widgets [ " << idx++ << " ] = "
          << make_property_name ( impl, itr->name )
          << ";\n";
    }

    out << "}\n" << std::endl;
  }

  void generate_xm_proxy_source_result_table ( const XmComponentImpl* impl,
                                               std::ofstream& out )
  {
    out << "/* ---- Worker result table initialization ------------------------------- */\n"
        << std::endl;

    out << "#define N_RESULTS ( "
        << impl->results.size ( ) << " )\n"
        << std::endl;

    if ( impl->results.size ( ) == 0 )
    {
      return;
    }

    out << "static void init_results_table ( RCCWorker* wctx )\n"
        << "{\n"
        << "  XmResultTable* p_tab = ( XmResultTable* )\n"
        << "                           wctx->memories [ XM_RESULT_TABLE_INDEX ];\n"
        << "\n"
        << "  p_tab->n_results = N_RESULTS;\n";

    size_t idx ( 0 );

    for ( std::vector<XmResult>::const_iterator itr = impl->results.begin ();
           itr != impl->results.end ();
           ++itr )
    {
      {
        Simple s;

        s.type = itr->data_type;

        std::vector<Simple> types;

        types.push_back ( s );

        out << "\n  XmResult " << make_property_name ( impl, itr->name ) << " =\n"
            << "  {\n"
            << "    .name = \"" << itr->name << "\",\n";

        if ( itr->property )
        {
            out << "    .property = \"" << itr->property << "\",\n";
        }
        else
        {
            out << "    .property [ 0 ] = \'\\0\',\n";
        }

        if ( itr->value )
        {
            out << "    .value = \"" << itr->value << "\",\n";
        }
        else
        {
            out << "    .value [ 0 ] = \'\\0\',\n";
        }

        if ( itr->property )
        {
          out << "    .data_type = XM_PROP_TYPE_NONE\n";
        }
        else
        {
          out << "    .data_type = " << get_xm_data_type_def ( types ) << "\n";
        }
      }

      out << "  };\n"
          << "\n"
          << "  p_tab->results [ " << idx++ << " ] = "
          << make_property_name ( impl, itr->name )
          << ";\n";
    }

    out << "}\n" << std::endl;
  }

  void generate_xm_proxy_source_switch_table ( const XmComponentImpl* impl,
                                               std::ofstream& out )
  {
    out << "/* ---- Worker switch table initialization ------------------------------- */\n"
        << std::endl;

    out << "#define N_SWITCHES ( "
        << impl->switches.size ( ) << " )\n"
        << std::endl;

    if ( impl->switches.size ( ) == 0 )
    {
      return;
    }

    out << "static void init_switches_table ( RCCWorker* wctx )\n"
        << "{\n"
        << "  XmSwitchTable* p_tab = ( XmSwitchTable* )\n"
        << "                           wctx->memories [ XM_SWITCH_TABLE_INDEX ];\n"
        << "\n"
        << "  p_tab->n_switches = N_SWITCHES;\n";

    size_t idx ( 0 );

    for ( std::vector<XmSwitch>::const_iterator itr = impl->switches.begin ();
           itr != impl->switches.end ();
           ++itr )
    {
      {
        Simple s;

        s.type = itr->data_type;

        std::vector<Simple> types;

        types.push_back ( s );

        out << "\n  XmSwitch " << make_property_name ( impl, itr->name ) << " =\n"
            << "  {\n"
            << "    .name = \"" << itr->name << "\",\n";

        if ( itr->property )
        {
            out << "    .property = \"" << itr->property << "\",\n";
        }
        else
        {
            out << "    .property [ 0 ] = \'\\0\',\n";
        }

        if ( itr->value )
        {
            out << "    .value = \"" << itr->value << "\",\n";
        }
        else
        {
            out << "    .value [ 0 ] = \'\\0\',\n";
        }

        if ( itr->property )
        {
          out << "    .data_type = XM_PROP_TYPE_NONE\n";
        }
        else
        {
          out << "    .data_type = " << get_xm_data_type_def ( types ) << "\n";
        }
      }

      out << "  };\n"
          << "\n"
          << "  p_tab->switches [ " << idx++ << " ] = "
          << make_property_name ( impl, itr->name )
          << ";\n";
    }

    out << "}\n" << std::endl;
  }

  const std::string get_xm_port_type ( bool producer )
  {
    if ( producer )
    {
      return std::string ( "XM_PORT_SOURCE" );
    }

    return std::string ( "XM_PORT_TARGET" );
  }

  const std::string get_xm_filetype_str ( int filetype )
  {
    std::stringstream ss;

    ss << filetype;

    return ss.str ( );
  }

  void generate_xm_proxy_source_port_table ( const XmComponentImpl* impl,
                                             std::ofstream& out )
  {
    out << "/* ---- Worker port table initialization --------------------------------- */\n"
        << std::endl;

    out << "#define N_PORTS ( "
        << impl->ports.size ( ) << " )\n"
        << std::endl;

    out << "static void init_port_table ( RCCWorker* wctx )\n"
        << "{\n"
        << "  XmPortTable* p_tab = ( XmPortTable* )\n"
        << "                         wctx->memories [ XM_PORT_TABLE_INDEX ];\n"
        << "\n"
        << "  p_tab->n_ports = N_PORTS;\n";

    size_t idx ( 0 );

    for ( std::vector<Port>::const_iterator p = impl->ports.begin ( );
          p != impl->ports.end ( );
          p++ )
    {
      out << "\n  XmPort " << p->name << " =\n"
          << "  {\n"
          << "    .port_idx = " << idx << ",\n"
          << "    .file_name = " << "\"" << p->hcb.file_name << "\"" << ",\n"
          << "    .direction = " << get_xm_port_type ( p->producer ) << ",\n"
          << "    .filespec = " << "\"" << p->hcb.filespec << "\"" << ",\n"
          << "    .type = " << p->hcb.type << ",\n"
          << "    .type_allow = " << "\"" << get_xm_filetype_str ( p->hcb.type ) << "\""  << ",\n"
          << "    .format_allow = " << "\"" << p->hcb.format << "\"" << ",\n"
          << "    .buf_type = " << "\'" << p->hcb.format [ 1 ] << "\'" << ",\n"
          << "    .format = " << "\"" << p->hcb.format << "\""<< ",\n"
          << "    .size  = "<< p->hcb.size << ",\n"
          << "    .xstart = "<< p->hcb.xstart << ",\n"
          << "    .xdelta = "<< p->hcb.xdelta << ",\n"
          << "    .xunits = "<< p->hcb.xunits << ",\n"
          << "    .ystart = "<< p->hcb.ystart << ",\n"
          << "    .ydelta = "<< p->hcb.ydelta << ",\n"
          << "    .yunits = "<< p->hcb.yunits << "\n"
          << "  };\n"
          << "\n"
          << "  p_tab->ports [ " << idx << " ] = "
          << p->name
          << ";\n";

          idx++;
    }

    out << "}\n" << std::endl;
  }

  void generate_xm_proxy_control_ops ( const XmComponentImpl* impl,
                                       std::ofstream& out )
  {
    std::string spec_name ( impl->spec_name );

    std::transform ( spec_name.begin ( ),
                     spec_name.end ( ),
                     spec_name.begin ( ),
                     ::tolower );


    out << "/* ---- Worker implementation -------------------------------------------- */\n"
        << "\n"
        << "static RCCResult initialize ( RCCWorker* wctx )\n"
        << "{\n"
        << "  init_port_table ( wctx );\n"
        << "\n"
        << "  init_property_table ( wctx );\n"
        << "\n";

    if ( impl->widgets.size ( ) > 0 )
    {
      out << "  init_widgets_table ( wctx );\n"
          << "\n";
    }

    if ( impl->results.size ( ) > 0 )
    {
      out << "  init_results_table ( wctx );\n"
          << "\n";
    }

    if ( impl->switches.size ( ) > 0 )
    {
      out << "  init_switches_table ( wctx );\n"
          << "\n";
    }

    if ( impl->commandline.size ( ) > 0 )
    {
      out << "  init_commandline_table ( wctx );\n"
          << "\n";
    }

    out << "  ExecutionContext* ctx = ( ExecutionContext* ) \n"
        << "                            wctx->memories [ XM_CONTEXT_INDEX ];\n"
        << "\n"
        << "  getcontext ( &ctx->primitive );\n"
        << "\n"
        << "  ctx->primitive.uc_link = &ctx->caller;\n"
        << "  ctx->primitive.uc_stack.ss_sp = ctx->stack_primitive;\n"
        << "  ctx->primitive.uc_stack.ss_size = SIGSTKSZ;\n"
        << "  ctx->start_is_done = 0;\n"
        << "  ctx->run_is_done = 0;\n"
        << "  ctx->stop_is_done = 0;\n"
        << "  ctx->finish_is_done = 0;\n"
        << "\n"
        << "  makecontext ( &ctx->primitive,\n"
        << "                ( void ( * ) ( ) ) ocpi_xm_mainroutine_" << spec_name << ",\n"
        << "                2,\n"
        << "                wctx );\n"
        << "\n"
        << "  return RCC_OK;\n"
        << "}\n"
        << "\n"
        << "static RCCResult start ( RCCWorker* wctx )\n"
        << "{\n"
        << "  ExecutionContext* ctx = ( ExecutionContext* )\n"
        << "                            wctx->memories [ XM_CONTEXT_INDEX ];\n"
        << "\n"
        << "  getcontext ( &ctx->caller );\n"
        << "\n"
        << "  if ( ctx->start_is_done == 0 )\n"
        << "  {\n"
        << "    swapcontext ( &ctx->caller, &ctx->primitive );\n"
        << "  }\n"
        << "\n"
        << "  return RCC_OK;\n"
        << "}\n"
        << "\n"
        << "static RCCResult stop ( RCCWorker* wctx )\n"
        << "{\n"
        << "  ExecutionContext* ctx = ( ExecutionContext* )\n"
        << "                            wctx->memories [ XM_CONTEXT_INDEX ];\n"
        << "\n"
        << "  getcontext ( &ctx->caller );\n"
        << "\n"
        << "  if ( ( ctx->stop_is_done == 0 ) && ( ctx->finish_is_done == 0 ) )\n"
        << "  {\n"
        << "    ctx->stop_is_done = 1;\n"
        << "    swapcontext ( &ctx->caller, &ctx->primitive );\n"
        << "  }\n"
        << "\n"
        << "  ctx->stop_is_done = 0;\n"
        << "\n"
        << "  return RCC_OK;\n"
        << "}\n"
        << "\n"
        << "static RCCResult release ( RCCWorker* wctx )\n"
        << "{\n"
        << "  ( void ) wctx;\n"
        << "\n"
        << "  return RCC_OK;\n"
        << "}\n"
        << "\n"
        << "static RCCResult test ( RCCWorker* wctx )\n"
        << "{\n"
        << "  ( void ) wctx;\n"
        << "\n"
        << "  return RCC_OK;\n"
        << "}\n"
        << "\n"
        << "/* We are going to use \"after configure\" as \"finish\" */\n"
        << "\n"
        << "static RCCResult finish ( RCCWorker* wctx )\n"
        << "{\n"
        << "  ExecutionContext* ctx = ( ExecutionContext* )\n"
        << "                            wctx->memories [ XM_CONTEXT_INDEX ];\n"
        << "\n"
        << "  getcontext ( &ctx->caller );\n"
        << "\n"
        << "  if ( ctx->finish_is_done == 0 )\n"
        << "  {\n"
        << "    ctx->finish_is_done = 1;\n"
        << "    swapcontext ( &ctx->caller, &ctx->primitive );\n"
        << "  }\n"
        << "\n"
        << "  return RCC_OK;\n"
        << "}\n"
        << std::endl;
  }

  void generate_xm_proxy_run ( const XmComponentImpl* impl,
                               std::ofstream& out )
  {
    out << "\n"
        << "static RCCResult run ( RCCWorker* wctx,\n"
        << "                       RCCBoolean timedout,\n"
        << "                       RCCBoolean* new_run_condition )\n"
        << "{\n"
        << "  ( void ) timedout;\n"
        << "  ( void ) new_run_condition;\n"
        << "\n"
        << "  ExecutionContext* ctx = ( ExecutionContext* )\n"
        << "                            wctx->memories [ XM_CONTEXT_INDEX ];\n"
        << "\n"
        << "  getcontext ( &ctx->caller );\n"
        << "\n"
        << "  if ( ctx->run_is_done == 0 )\n"
        << "  {\n"
        << "    swapcontext ( &ctx->caller, &ctx->primitive );\n"
        << "  }\n"
        << "\n"
        << "  ctx->run_is_done = 0;\n"
        << "\n";

    for ( std::vector<Port>::const_iterator p = impl->ports.begin ( );
          p != impl->ports.end ( );
          p++ )
    {
      if ( p->producer )
      {
        std::string port_name_lc ( p->name );

        std::transform ( port_name_lc.begin ( ),
                         port_name_lc.end ( ),
                         port_name_lc.begin ( ),
                         ::tolower );

        std::string port_name_uc ( p->name );

        std::transform ( port_name_uc.begin ( ),
                         port_name_uc.end ( ),
                         port_name_uc.begin ( ),
                         ::toupper );

        std::string base ( impl->impl_name );

        std::transform ( base.begin ( ),
                         base.end ( ),
                         base.begin ( ),
                         ::toupper );

        if ( get_number_of_target_ports ( impl ) == 0 )
        {
          out << "  if ( wctx->memories [ XM_TODO_INDEX ] )\n"
              << "  {\n"
              << "    XmTodoState* p = ( XmTodoState* ) wctx->memories [ XM_TODO_INDEX ];\n"
              << "\n"
              << "    if ( p->done )\n"
              << "    {\n"
              << "      return RCC_DONE;\n"
              << "    }\n"
              << "  }\n";
        }

#if 0 // m$filad takes care of the output buffer update
        if ( p->optional )
        {
          out << "  if ( wctx->connectedPorts & ( 1 << "
              << make_port_name ( impl, p ) << " ) )\n"
              << "  {\n"
              << "    RCCPort* p_port_" << port_name_lc
              << " = &wctx->ports [ " << make_port_name ( impl, p ) << " ];\n"
              << "\n"
              << "    p_port_" << port_name_lc << "->output.length = "
              << "  p_port_" << port_name_lc << "->current.maxLength;\n"
              << "  }\n"
              << std::endl;
        }
        else
        {
          out << "  RCCPort* p_port_" << port_name_lc
              << " = &wctx->ports [ " << make_port_name ( impl, p ) << " ];\n"
              << "\n"
              << "  p_port_" << port_name_lc << "->output.length = "
              << "p_port_" << port_name_lc << "->current.maxLength;"
              << std::endl;
        }
#endif
      } // End: if ( p->producer )

    } // End: for ( std::vector<Port>::const_iterator p = impl->ports.beg...

    out << "\n"
        << "  return RCC_ADVANCE;\n"
        << "}\n"
        << std::endl;
  }

  void generate_xm_memories_table ( const XmComponentImpl* impl,
                                    std::ofstream& out )
  {
    out << "/* ---- Worker Memories Table -------------------------------------------- */\n"
        << "\n"
        << "static uint32_t memories [ ] =\n"
        << "{\n"
        << "  sizeof ( ExecutionContext ),\n"
        << "  sizeof ( XmPortTable ) + sizeof ( XmPort ) * N_PORTS,\n"
        << "  sizeof ( XmWidgetTable ) + sizeof ( XmWidget ) * N_WIDGETS,\n"
        << "  sizeof ( XmResultTable ) + sizeof ( XmResult ) * N_RESULTS,\n"
        << "  sizeof ( XmSwitchTable ) + sizeof ( XmSwitch ) * N_SWITCHES,\n"
        << "  sizeof ( XmPropertyTable ) + sizeof ( XmProperty ) * N_PROPERTIES,\n"
        << "  sizeof ( XmCommandLineTable ) + sizeof ( XmCommandLine ) * N_COMMANDLINE,\n";

    if ( get_number_of_target_ports ( impl ) == 0 )
    {
        out << "  sizeof ( XmTodoState ), // M$TODO element count\n";
    }

    out << "  0\n"
        << "};\n"
        << std::endl;
  }

  const std::string make_optional_ports ( const XmComponentImpl* impl )
  {
    std::string optional_ports;

    for ( std::vector<Port>::const_iterator p = impl->ports.begin ();
           p != impl->ports.end ();
           ++p )
    {
      if ( p->optional )
      {
        optional_ports += ( std::string ( "( 1 << " ) +
                            make_port_name ( impl, p ) +
                            std::string ( " )" ) );

        if ( ( p + 1 ) != impl->ports.end ( ) )
        {
          optional_ports += std::string ( " |\n                              " );
        }
      }
    }

    if ( optional_ports.empty ( ) )
    {
      optional_ports = std::string ( "0" ); // Indicates no ports are optional
    }

    return optional_ports;
  }

  void generate_xm_dispatch_table ( const XmComponentImpl* impl,
                                    std::ofstream& out )
  {
    std::string base ( impl->impl_name );

    std::transform ( base.begin ( ),
                     base.end ( ),
                     base.begin ( ),
                     ::toupper );

    bool have_target_ports = false;
    bool have_source_ports = false;

    for ( std::vector<Port>::const_iterator p = impl->ports.begin ();
           p != impl->ports.end ();
           ++p )
    {
      if ( p->producer )
      {
        have_source_ports = true;
      }
      else
      {
        have_target_ports = true;
      }
    }

    std::string n_source_ports;
    std::string n_target_ports;

    if ( have_target_ports )
    {
      n_target_ports = base + std::string ( "_N_TARGET_PORTS" );
    }
    else
    {
      n_target_ports = std::string ( "0" );
    }

    if ( have_source_ports )
    {
      n_source_ports = base + std::string ( "_N_SOURCE_PORTS" );
    }
    else
    {
      n_source_ports = std::string ( "0" );
    }

    std::string worker_props ( std::string ( impl->impl_name ) +
                               std::string ( "WorkerProperties" ) );

    std::string worker_dispatch ( std::string ( impl->impl_name ) );

    out << "/* ---- Worker Dispatch Table -------------------------------------------- */\n"
        << "\n"
        << "RCCDispatch " << worker_dispatch << " =\n"
        << "{\n"
        << "  /* ---- Information for consistency checking --------------------------- */\n"
        << "\n"
        << "  .version = RCC_VERSION,\n"
        << "  .numInputs = " << n_target_ports << ",\n"
        << "  .numOutputs = " << n_source_ports << ",\n"
        << "  .propertySize = sizeof ( " << worker_props << " ),\n"
        << "  .memSizes = memories,\n"
        << "  .threadProfile = 0,\n"
        << "\n"
        << "  /* ---- Methods -------------------------------------------------------- */\n"
        << "\n"
        << "  .initialize = initialize,\n"
        << "  .start = start,\n"
        << "  .stop = stop,\n"
        << "  .release = release,\n"
        << "  .test = test,\n"
        << "  .afterConfigure = finish,\n"
        << "  .beforeQuery = 0,\n"
        << "  .run = run,\n"
        << "\n"
        << "  /* ---- Implementation information for container behavior -------------- */\n"
        << "\n"
        << "  .portInfo = 0,\n"
        << "  .runCondition = 0,\n"
        << "  .optionallyConnectedPorts = " << make_optional_ports ( impl ) << "\n"
        << "};\n"
        << std::endl;
  }

  void generate_xm_proxy_source_footer ( const XmComponentImpl*,
                                         std::ofstream& )
  {
    // Nothing to do
  }

  void generate_xm_proxy_source ( XmComponentImpl* impl )
  {
    std::ofstream out;

    std::string filename = genterate_xm_proxy_filename ( impl->impl_name,
                                                         ".c" );

    create_output_stream ( filename.c_str ( ), out );


    generate_xm_proxy_source_header ( impl, out );

    generate_xm_proxy_source_propery_table ( impl, out );

    generate_xm_proxy_source_commandline_table ( impl, out );

    generate_xm_proxy_source_widget_table ( impl, out );

    generate_xm_proxy_source_result_table ( impl, out );

    generate_xm_proxy_source_switch_table ( impl, out );

    generate_xm_proxy_source_port_table ( impl, out );

    generate_xm_proxy_control_ops ( impl, out );

    generate_xm_proxy_run ( impl, out );

    generate_xm_memories_table ( impl, out );

    generate_xm_dispatch_table ( impl, out );

    generate_xm_proxy_source_footer ( impl, out );
  }

  void generate_xm_proxy ( XmComponentImpl* impl )
  {
    generate_xm_proxy_include ( impl );

    generate_xm_proxy_source ( impl );
  }

  void xm_to_openocpi ( int argc, char* argv [ ] )
  {
    /* ---- Parse command line arguments -------------------------------- */

    OcpiXmConfigurator config;

    int rc = parse_commandline ( argc, argv, &config );

    if ( rc )
    {
      return;
    }

    std::cout << "\nocpixm is running with "
              << argv [ 1 ]
              << std::endl;

    /* ---- Find XmImplementation root ---------------------------------- */

    OCPI::Util::EzXml::Doc impl_doc;

    ezxml_t xml_impl = get_xml_file_root ( argv [ 1 ], impl_doc );

    /* ---- Find componentSpec root ------------------------------------- */

    OCPI::Util::EzXml::Doc spec_doc;

    ezxml_t xml_spec = get_component_spec_root ( xml_impl, spec_doc );

    /* ---- Parse the component implementation XML ---------------------- */

    XmComponentImpl impl;

    parse_component_xml ( xml_impl, xml_spec, &impl );

    /* ---- Emit the XM proxy ------------------------------------------- */

    generate_xm_proxy ( &impl );

    return;
  }

} // End: namespace <unamed>

int main ( int argc, char* argv [ ] )
{
  try
  {
    xm_to_openocpi ( argc, argv );
  }
  catch ( const std::string& s )
  {
    std::cerr << "\nException: " << s << std::endl;
  }
  catch ( ... )
  {
    std::cerr << "\nException: unknown" << std::endl;
  }

  std::cout << "\nocpixm is done\n" << std::endl;

  return 0;
}

