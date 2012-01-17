#ifndef __SEQUENCING_TEST_H__
#define __SEQUENCING_TEST_H__

#include <cxxtest/TestSuite.h>
#include "../SigProc.h"

//
// A simple test suite: Just inherit CxxTest::TestSuite and write tests!
//

class PNSequence_Testsuite : public CxxTest::TestSuite
{
public:

    void test_constructor_01() {
        unsigned long g(0x0007);
        unsigned long a(0x0001);
        SigProc::PNSequence pn(g, a);
        TS_ASSERT_EQUALS(pn.m, 2);
        TS_ASSERT_EQUALS(pn.n, 3);

        // TODO: Generate output sequence
    }

    void test_constructor_02() {
        unsigned long g(0x0043);
        unsigned long a(0x0001);
        SigProc::PNSequence pn(g, a);
        TS_ASSERT_EQUALS(pn.m, 6);
        TS_ASSERT_EQUALS(pn.n, 63);

        // TODO: Generate output sequence
    }

};


#endif // __SEQUENCING_TEST_H__

