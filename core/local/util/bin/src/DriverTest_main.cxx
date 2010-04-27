

#include <CpiDriver.h>
#include <CpiOsAssert.h>
#include <CpiTimeEmit.h>



using namespace CPI::Util;
using namespace CPI::Time;
class MyDriver;

class MyDevice : public Device, public Emit {
public:
  MyDevice( MyDriver& parent, const char* i_name )
    throw ( CPI::Util::EmbeddedException );
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

  virtual Device *probe(const CPI::Util::PValue*, const char *which )
    throw ( CPI::Util::EmbeddedException )
  {
    printf("In myProbe routine\n");
    //    Device* child = findChild( which );
    return NULL;
  }

  virtual unsigned search(const PValue* props, const char **exclude)
    throw ( CPI::Util::EmbeddedException )
  {
    printf("In MySearch\n");
    new MyDevice(*this,"dev1");
    new MyDevice(*this,"dev2");
    return 0;
  }

  virtual ~MyDriver()
    throw() {}

};


MyDevice::MyDevice( MyDriver& parent, const char* i_name )
  throw ( CPI::Util::EmbeddedException )
  : Device(parent, i_name), Emit( &parent, "MyDevice")
{
}


DriverManager dm("testdriver");
static MyDriver* md = new MyDriver;

int main(int argc, char** argv)
{
  

  dm.discoverDevices(0,0);

  Device * dev1 = dm.getDevice( 0, "dev1" );
  cpiAssert( dev1 );
  Device * dev2 = dm.getDevice( 0, "dev2" );
  cpiAssert( dev2 );
  Device * dev3 = dm.getDevice( 0, "dev3" );
  cpiAssert( dev3 == NULL );
  
  CPI::Time::EmitFormatter tfb(CPI::Time::EmitFormatter::CPIReadable);
  std::cout << tfb;  

  return 0;
}
