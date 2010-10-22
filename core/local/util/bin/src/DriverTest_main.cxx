
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



#include <OcpiDriver.h>
#include <OcpiOsAssert.h>
#include <OcpiTimeEmit.h>



using namespace OCPI::Util;
using namespace OCPI::Time;
class MyDriver;

class MyDevice : public Device, public Emit {
public:
  MyDevice( MyDriver& parent, const char* i_name )
    throw ( OCPI::Util::EmbeddedException );
  void start(){};
  void stop(){};

  virtual ~MyDevice()
    throw(){}

private:
};


class MyDriver : public Driver , public Emit {
public:
  MyDriver()
    :Driver("testdriver","d1", true), Emit( "MyDriver" )  {}

  virtual Device *probe(const OCPI::Util::PValue*, const char *which )
    throw ( OCPI::Util::EmbeddedException )
  {
    ( void ) which;
    printf("In myProbe routine\n");
    //    Device* child = findChild( which );
    return NULL;
  }

  virtual unsigned search(const PValue* props, const char **exclude)
    throw ( OCPI::Util::EmbeddedException )
  { 
    ( void ) props;
    ( void ) exclude;
    printf("In MySearch\n");
    new MyDevice(*this,"dev1");
    new MyDevice(*this,"dev2");
    return 0;
  }

  virtual ~MyDriver()
    throw() {}

};


MyDevice::MyDevice( MyDriver& parent, const char* i_name )
  throw ( OCPI::Util::EmbeddedException )
  : Device(parent, i_name), Emit( &parent, "MyDevice")
{
}


DriverManager dm("testdriver");
static MyDriver* md = new MyDriver;

int main(int argc, char** argv)
{
  ( void ) argc;
  ( void ) argv;  

  dm.discoverDevices(0,0);

  Device * dev1 = dm.getDevice( 0, "dev1" );
  ocpiCheck( dev1 );
  Device * dev2 = dm.getDevice( 0, "dev2" );
  ocpiCheck( dev2 );
  Device * dev3 = dm.getDevice( 0, "dev3" );
  ocpiCheck( dev3 == NULL );
  
  OCPI::Time::EmitFormatter tfb(OCPI::Time::EmitFormatter::OCPIReadable);
  std::cout << tfb;  

  return 0;
}
