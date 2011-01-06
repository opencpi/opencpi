
#ifndef INCLUDED_OCPI_TIMER_LISTENER_H
#define INCLUDED_OCPI_TIMER_LISTENER_H

#include <cppunit/TestListener.h>

#include <string>

namespace OCPI
{
  class TimerModel;

  class TimerListener : public CPPUNIT_NS::TestListener
  {
    public:
      TimerListener ( TimerModel* model,
                      bool text );

      virtual ~TimerListener ( );

      void startTestRun ( CPPUNIT_NS::Test* test,
                          CPPUNIT_NS::TestResult* event_manager );

      void endTestRun ( CPPUNIT_NS::Test* test,
                        CPPUNIT_NS::TestResult* event_manager );

      void startTest ( CPPUNIT_NS::Test* test );

      void endTest ( CPPUNIT_NS::Test* test );

      void startSuite ( CPPUNIT_NS::Test* suite );

      void endSuite ( CPPUNIT_NS::Test* suite );

    private:
      void printStatistics ( ) const;

      void printTest ( int test_index,
                       const std::string& indent_string ) const;

      void printTestIndent ( const std::string& indent,
                             const int indent_length ) const;

      void printTime ( double time ) const;

    private:
      TimerModel* d_model;
      bool d_text;

    private:
      // Copy constructor - not implemented
      TimerListener ( const TimerListener& );

      // Copy operator - not implemented
      void operator= ( const TimerListener&  );
  };

} // End: namespace OCPI

#endif // End: #ifndef INCLUDED_OCPI_TIMER_LISTENER_H
