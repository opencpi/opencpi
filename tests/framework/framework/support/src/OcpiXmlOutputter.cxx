
/**
  @file
  Formats CppUnit test results using the JUnit XML format instead of the
  CppUnit XML format.

************************************************************************** */

#include "OcpiXmlOutputter.h"

#include <cppunit/Test.h>
#include <cppunit/Exception.h>
#include <cppunit/TestFailure.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/XmlOutputterHook.h>
#include <cppunit/tools/XmlElement.h>
#include <cppunit/tools/XmlDocument.h>

#include <cstdlib>
#include <algorithm>


OCPI::XmlOutputter::XmlOutputter ( CPPUNIT_NS::TestResultCollector* result,
                                   CPPUNIT_NS::OStream& stream,
                                   std::string encoding )
  : CPPUNIT_NS::XmlOutputter ( result, stream, encoding )
{
  m_xml->setStandalone ( true );
}


OCPI::XmlOutputter::~XmlOutputter ( )
{
  // Empty
}

void OCPI::XmlOutputter::setRootNode ( )
{
  CPPUNIT_NS::XmlElement* rootNode = new CPPUNIT_NS::XmlElement ( "testsuite" );

  m_xml->setRootElement ( rootNode );

  for ( Hooks::iterator it = m_hooks.begin(); it != m_hooks.end(); ++it )
  {
    ( *it )->beginDocument ( m_xml );
  }

  FailedTests failedTests;

  fillFailedTestsMap ( failedTests );

  addFailedTests ( failedTests, rootNode );
  addSuccessfulTests ( failedTests, rootNode );
  addStatistics ( rootNode );

  for ( Hooks::iterator it = m_hooks.begin(); it != m_hooks.end(); ++it )
  {
    ( *it )->endDocument ( m_xml );
  }
}


void OCPI::XmlOutputter::addFailedTests ( FailedTests& failedTests,
                                          CPPUNIT_NS::XmlElement* rootNode )
{
  const CPPUNIT_NS::TestResultCollector::Tests& tests = m_result->tests ( );

  for ( std::size_t testNumber = 0; testNumber < tests.size(); ++testNumber )
  {
    CPPUNIT_NS::Test* test = tests [ testNumber ];
    if ( failedTests.find ( test ) != failedTests.end ( ) )
    {
      addFailedTest ( test, failedTests [ test ], testNumber + 1, rootNode );
    }
  }

}


void OCPI::XmlOutputter::addSuccessfulTests ( FailedTests& failedTests,
                                              CPPUNIT_NS::XmlElement* rootNode )
{
  const CPPUNIT_NS::TestResultCollector::Tests& tests = m_result->tests ( );

  for ( std::size_t testNumber = 0; testNumber < tests.size(); ++testNumber )
  {
    CPPUNIT_NS::Test* test = tests [ testNumber ];
    if ( failedTests.find ( test ) == failedTests.end ( ) )
    {
      addSuccessfulTest ( test, testNumber + 1, rootNode );
    }
  }

}


void OCPI::XmlOutputter::addStatistics ( CPPUNIT_NS::XmlElement* rootNode )
{
  rootNode->addAttribute ( "errors",  m_result->testErrors () );
  rootNode->addAttribute ( "failures",  m_result->testFailures () );
  rootNode->addAttribute ( "tests",  m_result->runTests () );
  rootNode->addAttribute ( "name", "OpenCPI CppUnit Tests" );

  char hostname [ 256 ];

  int rc = gethostname ( hostname, sizeof ( hostname ) - 1 );
  if ( rc )
  {
    strncpy ( hostname, "unknown", sizeof ( hostname ) - 1 );
  }

  rootNode->addAttribute ( "hostname", hostname );
}


void OCPI::XmlOutputter::addFailedTest ( CPPUNIT_NS::Test* test,
                                         CPPUNIT_NS::TestFailure* failure,
                                         int testNumber,
                                         CPPUNIT_NS::XmlElement* testsNode )
{
  CPPUNIT_NS::Exception* thrownException = failure->thrownException ( );

  CPPUNIT_NS::XmlElement* testElement =
                                 new CPPUNIT_NS::XmlElement ( "testcase" );
  testsNode->addElement ( testElement );

  size_t test_name_idx = test->getName ( ).rfind ( ':' ) + 1;

  testElement->addAttribute ( "name", test->getName ( ).substr ( test_name_idx ) );
  testElement->addAttribute ( "classname", test->getName ( ).substr ( 0, test_name_idx - 2 ) );

  CPPUNIT_NS::XmlElement* errorElement =
                                 new CPPUNIT_NS::XmlElement ( "error" );

  errorElement->addAttribute ( "message", thrownException->what() );
  errorElement->addAttribute ( "type",  failure->isError ( ) ? "Error"
                                                             : "Assertion" );

  testElement->addElement ( errorElement );

  for ( Hooks::iterator it = m_hooks.begin(); it != m_hooks.end(); ++it )
  {
    ( *it )->failTestAdded ( m_xml, testElement, test, failure );
  }
}


void OCPI::XmlOutputter::addFailureLocation (
     CPPUNIT_NS::TestFailure* failure,
     CPPUNIT_NS::XmlElement* testElement )
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
  CPPUNIT_NS::XmlElement* locationNode =
                                   new CPPUNIT_NS::XmlElement ( "Location" );

  testElement->addElement ( locationNode );

  CPPUNIT_NS::SourceLine sourceLine = failure->sourceLine();

  locationNode->addElement ( new CPPUNIT_NS::XmlElement (
                                 "File",
                                 sourceLine.fileName ( ) ) );
  locationNode->addElement ( new CPPUNIT_NS::XmlElement (
                                 "Line",
                                 sourceLine.lineNumber ( ) ) );
}


void OCPI::XmlOutputter::addSuccessfulTest ( CPPUNIT_NS::Test* test,
                                             int testNumber,
                                             CPPUNIT_NS::XmlElement* testsNode )
{
  CPPUNIT_NS::XmlElement* testElement = new CPPUNIT_NS::XmlElement ( "testcase" );
  testsNode->addElement ( testElement );

  size_t test_name_idx = test->getName ( ).rfind ( ':' ) + 1;

  testElement->addAttribute ( "name", test->getName ( ).substr ( test_name_idx ) );
  testElement->addAttribute ( "classname", test->getName ( ).substr ( 0, test_name_idx - 2 ) );

  for ( Hooks::iterator it = m_hooks.begin(); it != m_hooks.end(); ++it )
  {
    ( *it )->successfulTestAdded ( m_xml, testElement, test );
  }
}
