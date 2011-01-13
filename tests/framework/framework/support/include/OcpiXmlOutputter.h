
#ifndef INCLUDED_OCPI_XML_OUTPUTTER_H
#define INCLUDED_OCPI_XML_OUTPUTTER_H

#include <cppunit/Portability.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/portability/Stream.h>
#include <cppunit/portability/CppUnitMap.h>
#include <cppunit/portability/CppUnitDeque.h>

CPPUNIT_NS_BEGIN
  class Test;
  class TestFailure;
  class TestResultCollector;
  class XmlElement;
  class XmlDocument;
  class XmlOutputterHook;
CPPUNIT_NS_END

namespace OCPI
{
  class XmlOutputter : public CPPUNIT_NS::XmlOutputter
  {
    public:
      XmlOutputter ( CPPUNIT_NS::TestResultCollector* result,
                     CPPUNIT_NS::OStream& stream,
                     std::string encoding = std::string ( "ISO-8859-1" ) );

      virtual ~XmlOutputter ( );

      virtual void setRootNode ( );

      virtual void addFailedTests ( FailedTests& failedTests,
                                    CPPUNIT_NS::XmlElement* rootNode );

      virtual void addSuccessfulTests ( FailedTests& failedTests,
                                        CPPUNIT_NS::XmlElement* rootNode );

      virtual void addStatistics ( CPPUNIT_NS::XmlElement* rootNode );

      virtual void addFailedTest ( CPPUNIT_NS::Test* test,
                                   CPPUNIT_NS::TestFailure* failure,
                                   int testNumber,
                                   CPPUNIT_NS::XmlElement* testsNode );

      virtual void addFailureLocation ( CPPUNIT_NS::TestFailure* failure,
                                        CPPUNIT_NS::XmlElement* testElement );

      virtual void addSuccessfulTest ( CPPUNIT_NS::Test* test,
                                       int testNumber,
                                       CPPUNIT_NS::XmlElement* testsNode );

    private:
      // Copy constructor - not implemented
      XmlOutputter ( const XmlOutputter& );

      // Copy operator  - not implemented
      void operator= ( const XmlOutputter& );
  };

} // namespace OCPI

#endif  // End: #ifndef INCLUDED_OCPI_XML_OUTPUTTER_H

