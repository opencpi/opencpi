
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


#include <iostream>
#include <OcpiTimeEmit.h>
#define _c_plus_plus
#include <OcpiTimeEmitC.h>
#include <OcpiUtilCommandLineConfiguration.h>

using namespace OCPI::API;
using namespace OCPI::Time;


class OcpiRccBinderConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  OcpiRccBinderConfigurator ();

public:
  bool help;
  bool verbose;
  long format;



private:
  static CommandLineConfiguration::Option g_options[];
};

// Configuration
static  OcpiRccBinderConfigurator config;

OcpiRccBinderConfigurator::
OcpiRccBinderConfigurator ()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
    verbose (false),
    format(0)

{
}

OCPI::Util::CommandLineConfiguration::Option
OcpiRccBinderConfigurator::g_options[] = {

  /*
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "endpoint", "Set this containers endpoint",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::endpoint), 0 },
   { OCPI::Util::CommandLineConfiguration::OptionType::LONG,
    "msgSize", "Message size",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::msgSize), 0 },
  */

   { OCPI::Util::CommandLineConfiguration::OptionType::LONG,
    "format", "Output format",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::format), 0 },


  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::verbose), 0 },


  { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::help), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::END, 0, 0, 0, 0 }
};

static
void
printUsage (OcpiRccBinderConfigurator & config,
            const char * argv0)
{
  std::cout << "usage: " << argv0 << " [options]" << std::endl
            << "  options: " << std::endl;
  config.printOptions (std::cout);
}




class B : public Emit {
public:
  B( Emit* p ):Emit( p, "Class B"){}
  void doStuff() {
    OCPI_EMIT_( "B event 1");
    OCPI_EMIT_( "B event 2");
  }
};


class A : public Emit {
public:
  A(const char* instance_name=0):Emit( "Class A", instance_name){}
  void doStuff() {
    OCPI_EMIT_( "A event 1");
  }

};


class C : public Emit {
public:
  C( Emit* p ):Emit( p, "Class C"){}
  void setStates(int v) {
    switch ( v ) {
    case 1:
      OCPI_EMIT_STATE_( "State Event1", 1 );
      OCPI_EMIT_STATE_( "State Event2", 1 );
      break;
    case 2:
      OCPI_EMIT_STATE_( "State Event1", 0 );
      OCPI_EMIT_UINT64_( "uint64", 56789 );
      break;
    case 3:
      OCPI_EMIT_STATE_( "State Event2", 0 );
      break;
    }
  }
};


class EmitResource : public Emit {
public:
  EmitResource(const char* res_inst_name=0):Emit("EmitResource", res_inst_name),
    m_memory(1024*10)
  {
    OCPI_EMIT_UINT32_( "Container Memory", m_memory );
  }

  void allocate( int amount ) {
    m_memory -= amount;
    OCPI_EMIT_UINT32_( "Container Memory", m_memory );
  }

  void free( int amount ) {
    m_memory += amount;
    OCPI_EMIT_UINT32_( "Container Memory", m_memory );
  }
private:
  OCPI::OS::uint64_t m_memory;
};


bool vcdtest1()
{

  // Emit class, instance 1,
  A a;

  // Emit class, instance 2, w/ instance def.
  A a1("A1 instance");

  // Runtime parentable analyzer class
  B b( &a);

  {
    // Traceble Resource object
    EmitResource tr;

    // Make some events happen
    a.doStuff();
    b.doStuff();

    // Now do some resource stuff
    tr.allocate( 1024 );
    a.doStuff();
    tr.allocate( 5000 );
    b.doStuff();
    tr.free( 1024);
    tr.free( 5000);

    // allow the resource object to be removed to capture that on the trace
  }

  {
    C c( &b );

    // inherited object using state trace
    c.setStates(1);
    a.doStuff();
    c.setStates(2);
    a.doStuff();
    c.setStates(3);

    // allow c to be removed to capture that on the trace
  }

  OCPI_EMIT_HERE;


  // Create a trace formatter object and dump to cout
  OCPI::Time::EmitFormatter tfa(OCPI::Time::EmitFormatter::VCDFormat);
  std::cout << tfa;

  return true;
}

bool simpleEventTests()
{

  OCPI_EMIT_HERE;

  OCPI_EMIT_STATE( "state event", 0 );
  OCPI_EMIT_STATE( "state event", 1 );
  OCPI_EMIT_STATE( "state event", 0 );

  OCPI_EMIT_UINT8(  "uint8", 8 );
  OCPI_EMIT_UINT16( "uint16", 16 );
  OCPI_EMIT_UINT32( "uint32", 32 );
  OCPI_EMIT_UINT64( "uint16", 64 );

  OCPI_EMIT( "simple event" );

  // Create a trace formatter object and dump to cout
  OCPI::Time::EmitFormatter tfb(OCPI::Time::EmitFormatter::OCPIReadable);
  std::cout << tfb;

  return true;
}



class SimpleEventTestClass : public Emit {
public:
  SimpleEventTestClass(const char* instance_name=0):Emit( "SimpleEventTestClass", instance_name){}
  void events() {

    OCPI_EMIT_HERE_;

    OCPI_EMIT_STATE_( "state event", 0 );
    OCPI_EMIT_STATE_( "state event", 1 );
    OCPI_EMIT_STATE_( "state event", 0 );

    OCPI_EMIT_UINT8_( "uint8", 8 );
    OCPI_EMIT_UINT16_( "uint16", 16 );
    OCPI_EMIT_UINT32_( "uint32", 32 );
    OCPI_EMIT_UINT64_( "uint16", 64 );

    OCPI_EMIT_( "A event 1");
  }
};

bool simpleEventTests_()
{

  SimpleEventTestClass c;
  c.events();

  // Create a trace formatter object and dump to cout
  OCPI::Time::EmitFormatter tfb(OCPI::Time::EmitFormatter::OCPIReadable);
  std::cout << tfb;

  return true;
}


bool propertyEventTests()
{

  PVString pstring("pvstring type", "emit this string" );
  OCPI_EMIT_PVALUE( pstring );

  PVShort pshort("pshort type", -10 );
  OCPI_EMIT_PVALUE( pshort);

  PVUShort pushort("pushort type", 10 );
  OCPI_EMIT_PVALUE( pushort);

  PVBool pbool("pbool type", 0 );
  OCPI_EMIT_PVALUE( pbool );

  PVChar pchar("pchar type", 'k' );
  OCPI_EMIT_PVALUE( pchar );

  PVLong plong("plong type", -666 );
  OCPI_EMIT_PVALUE( plong );

  PVULong pulong("pulong type", 666  );
  OCPI_EMIT_PVALUE( pulong );

  PVLongLong plonglong("plonglong type", -3 );
  OCPI_EMIT_PVALUE( plonglong );

  PVULongLong pulonglong("pulonglong type", 1234567  );
  OCPI_EMIT_PVALUE( pulonglong );

  PVDouble pdouble("pdouble type", 123.4567 );
  OCPI_EMIT_PVALUE( pdouble );

  // Create a trace formatter object and dump to cout
  OCPI::Time::EmitFormatter tfb(OCPI::Time::EmitFormatter::OCPIReadable);
  std::cout << tfb;

  return true;
}


class PropertyEventTestClass : public Emit {
public:
  PropertyEventTestClass(const char* instance_name=0):Emit( "PropertyEventTestClass", instance_name){}
  void events() {

  PVString pstring("pvstring type", "emit this string" );
  OCPI_EMIT_PVALUE_( pstring );

  PVShort pshort("pshort type", -10 );
  OCPI_EMIT_PVALUE_( pshort);

  PVUShort pushort("pushort type", 10 );
  OCPI_EMIT_PVALUE_( pushort);

  PVBool pbool("pbool type", 0 );
  OCPI_EMIT_PVALUE_( pbool );

  PVChar pchar("pchar type", 'k' );
  OCPI_EMIT_PVALUE_( pchar );

  PVLong plong("plong type", -666 );
  OCPI_EMIT_PVALUE_( plong );

  PVULong pulong("pulong type", 666  );
  OCPI_EMIT_PVALUE_( pulong );

  PVLongLong plonglong("plonglong type", -3 );
  OCPI_EMIT_PVALUE_( plonglong );

  PVULongLong pulonglong("pulonglong type", 1234567  );
  OCPI_EMIT_PVALUE_( pulonglong );

  PVDouble pdouble("pdouble type", 123.4567 );
  OCPI_EMIT_PVALUE_( pdouble );

  }
};

bool propertyEventTests_()
{

  PropertyEventTestClass c;
  c.events();

  // Create a trace formatter object and dump to cout
  OCPI::Time::EmitFormatter tfb(OCPI::Time::EmitFormatter::OCPIReadable);
  std::cout << tfb;

  return true;
}




OCPI::Time::Emit::QConfig qc1 = {300,false};
class QLengthTestClass : public Emit {
public:
  QLengthTestClass(const char* instance_name=0)
    : Emit( "QLengthTestClass", instance_name, &qc1 ){}
  void events( int n ) {
    for ( int i=0; i<n; i++ ) {
      OCPI_EMIT_UINT32_( "uint64", i );
    }
  }
};

void QLengthTest1()
{
  QLengthTestClass qlt;
  qlt.events( 5 );

  // Create a trace formatter object and dump to cout
  OCPI::Time::EmitFormatter tfb(OCPI::Time::EmitFormatter::OCPIReadable);
  std::cout << tfb;

  qlt.events( 5 );
  std::cout << tfb;

  qlt.events( 5 );
  std::cout << tfb;

}


OCPI::Time::Emit::QConfig qc2 = {150,true};
class QStopTestClass : public Emit {
public:
  QStopTestClass(const char* instance_name=0)
    : Emit( "QLengthTestClass", instance_name, &qc2 ){}
  void events( int n ) {
    for ( int i=0; i<n; i++ ) {
      OCPI_EMIT_UINT32_( "uint64", i );
    }
  }
};

void QStopTest1()
{
  QStopTestClass q;
  q.events( 5 );

  // Create a trace formatter object and dump to cout
  OCPI::Time::EmitFormatter tfb(OCPI::Time::EmitFormatter::OCPIReadable);
  std::cout << tfb;

  q.events( 10 );
  std::cout << tfb;

  q.events( 500 );
  std::cout << tfb;
}


static OCPI::Time::Emit::RegisterEvent ev0( "my event" );
static OCPI::Time::Emit::RegisterEvent ev1( "uint event", 8 );

int main( int argc, char** argv )
{

  try {
    config.configure (argc, argv);
  }
  catch (const std::string & oops) {
    std::cerr << "Error: " << oops << std::endl;
    return false;
  }
  if (config.help) {
    printUsage (config, argv[0]);
    return false;
  }

  PVLongLong plonglong("plonglong type", -3 );
  OCPI::Time::Emit::RegisterEvent ev2( plonglong );
  OCPI::Time::Emit::getSEmit().emit(ev0);
  OCPI::Time::Emit::getSEmit().emit(ev1,89);
  OCPI_TIME_EMIT_C("C event");


  /*
  OCPI::Time::Emit::getSEmit().emit(ev0);
  OCPI::Time::Emit::getSEmit().emit(ev1,89);
  OCPI::Time::Emit::getSEmit().emit(ev2,plonglong);







  if (config.format == 0 ) {
    OCPI::Time::EmitFormatter tfb(OCPI::Time::EmitFormatter::OCPIReadable);
    std::cout << tfb;
  }
  else if ( config.format == 1 ) {
    OCPI::Time::EmitFormatter tfb(OCPI::Time::EmitFormatter::VCDFormat);
    std::cout << tfb;
  }
  else {
    OCPI::Time::EmitFormatter tfb(OCPI::Time::EmitFormatter::OCPIRaw);
    std::cout << tfb;
  }



  simpleEventTests();
  simpleEventTests_();


  QLengthTest1();
  QStopTest1();

  propertyEventTests();
  propertyEventTests_();


  vcdtest1();

  */

  OCPI::Time::EmitFormatter tfb(OCPI::Time::EmitFormatter::VCDFormat);
  std::cout << tfb;

}


