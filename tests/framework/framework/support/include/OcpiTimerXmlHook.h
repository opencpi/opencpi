
#ifndef INCLUDED_TIMER_XML_HOOK_H
#define INCLUDED_TIMER_XML_HOOK_H

#include <cppunit/XmlOutputterHook.h>

namespace OCPI
{
  class TimerModel;

  class TimerXmlHook : public CPPUNIT_NS::XmlOutputterHook
  {
    public:
      TimerXmlHook ( TimerModel* model );

      virtual ~TimerXmlHook ( );

      void endDocument ( CPPUNIT_NS::XmlDocument* document );

      void failTestAdded ( CPPUNIT_NS::XmlDocument*document,
                           CPPUNIT_NS::XmlElement* test_element,
                           CPPUNIT_NS::Test* test,
                           CPPUNIT_NS::TestFailure* failure );

      void successfulTestAdded ( CPPUNIT_NS::XmlDocument* document,
                                 CPPUNIT_NS::XmlElement* test_element,
                                 CPPUNIT_NS::Test* test );

      void statisticsAdded ( CPPUNIT_NS::XmlDocument* document,
                             CPPUNIT_NS::XmlElement* statistics_element );

    private:
      void addTimedTest ( CPPUNIT_NS::XmlElement* parent_element,
                          int test_index );

    private:
      TimerModel* d_model;

    private:
      // Copy constructor - not implemented
      TimerXmlHook ( const TimerXmlHook& );

      // Copy operator - not implemented
      void operator= ( const TimerXmlHook& );

  };

} // End: namespace OCPI

#endif  // INCLUDED_TIMER_XML_HOOK_H
