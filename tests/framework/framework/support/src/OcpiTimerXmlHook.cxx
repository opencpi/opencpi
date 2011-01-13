
/**
  @file
  Adds the "time" attributes to the JUnit test case element. Use this
  hook with the OCPI::XmlOutputter that formats the CppUnit test results
  using the JUnit XML format.

************************************************************************** */

#include "OcpiTimerXmlHook.h"

#include "OcpiTimerModel.h"

#include <cppunit/Test.h>
#include <cppunit/tools/XmlDocument.h>
#include <cppunit/tools/XmlElement.h>

OCPI::TimerXmlHook::TimerXmlHook ( TimerModel* model )
  : d_model( model )
{
  // Empty
}


OCPI::TimerXmlHook::~TimerXmlHook ( )
{
  // Empty
}


void OCPI::TimerXmlHook::endDocument ( CPPUNIT_NS::XmlDocument* document )
{
  ( void ) document;
}


void OCPI::TimerXmlHook::addTimedTest (
     CPPUNIT_NS::XmlElement* parent_element,
     int test_index )
{
  ( void ) parent_element;
  ( void ) test_index;
}


void OCPI::TimerXmlHook::failTestAdded ( CPPUNIT_NS::XmlDocument* document,
                                         CPPUNIT_NS::XmlElement* test_element,
                                         CPPUNIT_NS::Test* test,
                                         CPPUNIT_NS::TestFailure* failure )
{
  ( void ) failure;
  successfulTestAdded ( document, test_element, test );
}


void OCPI::TimerXmlHook::successfulTestAdded (
     CPPUNIT_NS::XmlDocument* document,
     CPPUNIT_NS::XmlElement* test_element,
     CPPUNIT_NS::Test* test )
{
  int test_index = d_model->indexOf ( test );

  double time = ( test_index >= 0 ) ? d_model->testTimeFor ( test_index )
                                    : 0.0;

  test_element->addAttribute ( "time", TimerModel::timeStringFor ( time ) );
}


void OCPI::TimerXmlHook::statisticsAdded (
     CPPUNIT_NS::XmlDocument* document,
     CPPUNIT_NS::XmlElement* statistics_element )
{
  ( void ) document;
  ( void ) statistics_element;
}
