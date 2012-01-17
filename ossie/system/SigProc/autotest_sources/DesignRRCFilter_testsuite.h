#ifndef __DESIGNRRCFILTERTEST_H
#define __DESIGNRRCFILTERTEST_H

#include <cxxtest/TestSuite.h>
#include "../SigProc.h"

//
// A simple test suite: Just inherit CxxTest::TestSuite and write tests!
//

class DesignRRCFilter_Testsuite : public CxxTest::TestSuite
{
public:

    void testRRCCoefficientGeneration1() {

        // Initialize variables
        unsigned int k(2), m(3);
        float beta(0.3f);

        // Initialize pre-determined coefficient arrays
        float h0[13] = { -0.0331158, 0.0450158, 0.0565969, -0.1536039, -0.0750015,  0.6153450,
                         1.0819719, 0.6153450, -0.0750015, -0.1536039, 0.0565969, 0.0450158, -0.0331158
                       };

        // Initialize output pointer and length variables
        unsigned int h_len(2*k*m+1);
        float * h = new float[h_len];

        // Generate filter coefficients
        SigProc::DesignRRCFilter(k, m, beta, h);

        // Assert arrays are the same length
        TS_ASSERT_EQUALS( h_len, 13 );

        // Ensure data are equal
        for (unsigned int i=0; i<13; i++)
            TS_ASSERT_DELTA( h[i], h0[i], 0.00001f );
    }

    void testRRCCoefficientGeneration2() {

        // Initialize variables
        unsigned int k(3), m(4);
        float beta(0.25f);

        // Initialize pre-determined coefficient arrays
        float h0[25] = {
            0.02122065907773706,
            0.00168745629037681,
            -0.03670787615581981,
            -0.03751317973855786,
            0.02604353638820331,
            0.09088339559460432,
            0.05305164737789306,
            -0.09698416497476189,
            -0.20462778400826262,
            -0.06423715229557071,
            0.36324068835899959,
            0.85232463635308764,
            1.06830988618379075,
            0.85232463393678559,
            0.36324068527424419,
            -0.06423714516381475,
            -0.20462778394232523,
            -0.09698416398029090,
            0.05305164801670376,
            0.09088339542845345,
            0.02604353589641688,
            -0.03751317994120094,
            -0.03670787597812086,
            0.00168745650837126,
            0.02122065908010170
        };


        // Initialize output pointer and length variables
        unsigned int h_len(2*k*m+1);
        float * h = new float[h_len];

        // Generate filter coefficients
        SigProc::DesignRRCFilter(k, m, beta, h);

        // Assert arrays are the same length
        TS_ASSERT_EQUALS( h_len, 25 );

        // Ensure data are equal
        for (unsigned int i=0; i<25; i++)
            TS_ASSERT_DELTA( h[i], h0[i], 0.00001f );
    }

};


#endif // __DESIGNRRCFILTERTEST_H

