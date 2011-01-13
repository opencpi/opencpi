
#include "OcpiTimerListener.h"
#include "OcpiTimerModel.h"

#include <cppunit/Test.h>

#include <cstdio>
#include <iostream>

OCPI::TimerListener::TimerListener ( OCPI::TimerModel* model,
                                     bool text )
  : d_model( model ),
    d_text( text )
{
  // Empty
}


OCPI::TimerListener::~TimerListener ( )
{
  // Empty
}


void OCPI::TimerListener::startTestRun ( CPPUNIT_NS::Test* test,
                                         CPPUNIT_NS::TestResult* event_manager )
{
  ( void ) event_manager;
  d_model->setExpectedTestCount ( test->countTestCases() * 2 );
}


void OCPI::TimerListener::endTestRun ( CPPUNIT_NS::Test* test,
                                       CPPUNIT_NS::TestResult* event_manager )
{
  ( void ) test;
  ( void ) event_manager;

  if ( d_text )
  {
    printStatistics ( );
  }
}


void OCPI::TimerListener::startTest ( CPPUNIT_NS::Test* test )
{
  d_model->enterTest ( test, false );
}


void OCPI::TimerListener::endTest ( CPPUNIT_NS::Test* test )
{
  d_model->exitTest ( test, false );
}


void OCPI::TimerListener::startSuite ( CPPUNIT_NS::Test* suite )
{
  d_model->enterTest ( suite, true );
}


void OCPI::TimerListener::endSuite ( CPPUNIT_NS::Test* suite )
{
  d_model->exitTest ( suite, true );
}


void OCPI::TimerListener::printStatistics ( ) const
{
  printTest ( 0, "" );
  std::cout << "\nTotal elapsed time: ";
  printTime ( d_model->totalElapsedTime() );
  std::cout << ", average test case time: ";
  printTime ( d_model->averageTestCaseTime() );
}


void OCPI::TimerListener::printTest( int test_index,
                                     const std::string& indent_string ) const
{
  std::string indent = indent_string;

  const int indent_length = 3;

  printTestIndent ( indent_string, indent_length );

  printTime ( d_model->testTimeFor ( test_index ) );

  std::cout << d_model->testPathFor ( test_index ).getChildTest ( )->getName ( )
            << std::endl;

  if ( d_model->childCountFor ( test_index ) == 0 )
  {
    indent += std::string ( indent_length, ' ' );
  }
  else
  {
    indent += "|" + std::string ( indent_length - 1, ' ' );
  }

  for ( int n = 0; n < d_model->childCountFor ( test_index ); n++ )
  {
    printTest ( d_model->childAtFor ( test_index, n ), indent );
  }

}


void OCPI::TimerListener::printTestIndent ( const std::string& indent,
                                            const int indent_length ) const
{
  if ( indent.empty() )
  {
    return;
  }

  std::cout << "   "
            << indent.substr( 0, indent.length() - indent_length )
            << "+" << std::string( indent_length -1, '-' );
}


void OCPI::TimerListener::printTime ( double time ) const
{
  std::cout << '(' << OCPI::TimerModel::timeStringFor ( time ) << "s) ";
}
