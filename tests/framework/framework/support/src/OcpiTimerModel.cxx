
#include "OcpiTimerModel.h"

OCPI::TimerModel::TimerModel ( )
    : d_test_case_count ( 0 )
    , d_total_test_case_time ( 0 )
{
  // Empty
}


OCPI::TimerModel::~TimerModel ( )
{
  // Empty
}


void OCPI::TimerModel::setExpectedTestCount ( int count )
{
  d_tests.reserve ( count );
}


void OCPI::TimerModel::enterTest ( CPPUNIT_NS::Test* test,
                                   bool is_suite )
{
  d_current_path.add ( test );

  int test_index = d_tests.size ( );

  if ( !m_test_indices.empty ( ) )
  {
    d_tests [ m_test_indices.top() ].d_child_indices.push_back ( test_index );
  }

  m_test_indices.push ( test_index );

  d_test_to_indices.insert ( TestToIndexes::value_type ( test, test_index ) );

  TestInfo info;
  info.d_timer.start ( );
  info.d_path = d_current_path;
  info.d_is_suite = is_suite;

  d_tests.push_back ( info );

  if ( !is_suite )
  {
    ++d_test_case_count;
  }

}


void OCPI::TimerModel::exitTest ( CPPUNIT_NS::Test* test,
                                  bool is_suite )
{
  ( void ) test;

  d_tests [ m_test_indices.top() ].d_timer.stop ( );

  if ( !is_suite )
  {
    d_total_test_case_time += d_tests.back ( ).d_timer.elapsedTime ( );
  }

  d_current_path.up ( );
  m_test_indices.pop ( );
}


double OCPI::TimerModel::totalElapsedTime ( ) const
{
  return d_tests [ 0 ].d_timer.elapsedTime ( );
}


double OCPI::TimerModel::averageTestCaseTime ( ) const
{
  double average ( 0 );

  if ( d_test_case_count > 0 )
  {
    average = d_total_test_case_time / d_test_case_count;
  }

  return average;
}


double OCPI::TimerModel::testTimeFor ( int test_index ) const
{
  return d_tests [ test_index ].d_timer.elapsedTime ( );
}


std::string OCPI::TimerModel::timeStringFor ( double time )
{
  char buffer [ 512 ];

  ::sprintf ( buffer, "%10.5f", time );

  return buffer;
}


bool OCPI::TimerModel::isSuite ( int test_index ) const
{
  return d_tests [ test_index ].d_is_suite;
}


const CPPUNIT_NS::TestPath & OCPI::TimerModel::testPathFor ( int test_index ) const
{
  return d_tests [ test_index ].d_path;
}


int OCPI::TimerModel::indexOf ( CPPUNIT_NS::Test *test ) const
{
  TestToIndexes::const_iterator itr = d_test_to_indices.find ( test );

  if ( itr != d_test_to_indices.end ( ) )
  {
    return itr->second;
  }

  return -1;
}


int OCPI::TimerModel::childCountFor ( int test_index ) const
{
  return d_tests [ test_index ].d_child_indices.size ( );
}


int OCPI::TimerModel::childAtFor( int test_index,
                                  int chidIndex ) const
{
  return d_tests [ test_index ].d_child_indices [ chidIndex ];
}
