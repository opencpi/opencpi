
#include "OcpiTimerModel.h"
#include "OcpiTimerListener.h"
#include "OcpiTimerXmlHook.h"

#include <cppunit/TestResult.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/plugin/TestPlugIn.h>

namespace OCPI
{
  class TimerPlugIn : public CppUnitTestPlugIn
  {
    public:
      TimerPlugIn ( )
        : d_model ( NULL ),
          d_dumper ( NULL ),
          d_xml_hook ( NULL )
      {
        // Empty
      }


      ~TimerPlugIn ( )
      {
        delete d_model;
        delete d_dumper;
        delete d_xml_hook;
      }


      void initialize ( CPPUNIT_NS::TestFactoryRegistry* registry,
                        const CPPUNIT_NS::PlugInParameters& parameters )
      {
        ( void ) registry;
        bool text = false;

        if ( parameters.getCommandLine() == "text" )
        {
          text = true;
        }

        d_model = new TimerModel ( );
        d_dumper = new TimerListener ( d_model, text );
        d_xml_hook = new TimerXmlHook ( d_model );
      }


      void addListener ( CPPUNIT_NS::TestResult* event_manager )
      {
        event_manager->addListener ( d_dumper );
      }


      void removeListener( CPPUNIT_NS::TestResult *event_manager )
      {
        event_manager->removeListener( d_dumper );
      }


      void addXmlOutputterHooks ( CPPUNIT_NS::XmlOutputter* outputter )
      {
        outputter->addHook ( d_xml_hook );
      }


      void removeXmlOutputterHooks ( )
      {
        // Empty
      }


      void uninitialize ( CPPUNIT_NS::TestFactoryRegistry* registry )
      {
        ( void ) registry;
        // Empty
      }

    private:
      TimerModel* d_model;
      TimerListener* d_dumper;
      TimerXmlHook* d_xml_hook;
  };

} // End: namespace OCPI

CPPUNIT_PLUGIN_EXPORTED_FUNCTION_IMPL( OCPI::TimerPlugIn );

CPPUNIT_PLUGIN_IMPLEMENT_MAIN ( );
