
#ifndef INCLUDED_TIMER_MODEL_H
#define INCLUDED_TIMER_MODEL_H

#include "OcpiTimer.h"

#include <cppunit/TestPath.h>
#include <cppunit/portability/CppUnitVector.h>
#include <cppunit/portability/CppUnitMap.h>
#include <cppunit/portability/CppUnitStack.h>

#include <string>

namespace OCPI
{
  class TimerModel
  {
    public:
      TimerModel ( );

      virtual ~TimerModel ( );

      void setExpectedTestCount ( int count );

      void enterTest ( CPPUNIT_NS::Test* test,
                       bool isSuite );

      void exitTest ( CPPUNIT_NS::Test* test,
                      bool isSuite );

      double totalElapsedTime ( ) const;

      double averageTestCaseTime(  ) const;

      double testTimeFor( CPPUNIT_NS::Test* test ) const;

      double testTimeFor ( int test_index ) const;

      static std::string timeStringFor ( double time );

      bool isSuite ( int test_index ) const;

      const CPPUNIT_NS::TestPath& testPathFor ( int test_index ) const;

      int indexOf ( CPPUNIT_NS::Test* test ) const;

      int childCountFor ( int test_index ) const;

      int childAtFor ( int test_index,
                       int chidIndex ) const;

    private:
      struct TestInfo
      {
        Timer d_timer;
        bool d_is_suite;
        CPPUNIT_NS::TestPath d_path;
        CppUnitVector<int> d_child_indices;
      };

    private:
      CPPUNIT_NS::TestPath d_current_path;

      int d_test_case_count;
      double d_total_test_case_time;

      typedef CppUnitMap<CPPUNIT_NS::Test*, int> TestToIndexes;

      TestToIndexes d_test_to_indices;
      CppUnitStack<int> m_test_indices;
      CppUnitVector<TestInfo> d_tests;

    private:
       // Copy constructor - not implemented
      TimerModel ( const TimerModel& );

      // Copy operator - not implemented
      void operator= ( const TimerModel& );
  };

} // End: namespace OCPI

#endif // End: #ifndef INCLUDED_TIMER_MODEL_H
