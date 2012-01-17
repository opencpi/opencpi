#ifndef __FIRPOLYPHASEFILTERBANKTEST_H
#define __FIRPOLYPHASEFILTERBANKTEST_H

#include <cxxtest/TestSuite.h>
#include "../SigProc.h"

//
// A simple test suite: Just inherit CxxTest::TestSuite and write tests!
//

class FIRPolyphaseFilterBank_Testsuite : public CxxTest::TestSuite
{
public:

    void testRRCCoefficientGeneration() {

        // Initialize variables
        unsigned int k(2), m(3), N(4);
        float beta(0.3f);

        // Initialize pre-determined coefficient arrays
        float f0[12] = { -0.0331158, 0.0450158, 0.0565969, -0.1536039, -0.0750015, 0.6153450,
                         1.0819719, 0.6153450, -0.0750015, -0.1536039, 0.0565969, 0.0450158
                       };

        float f1[12] = { -0.0241806, 0.0680333, 0.0164026,-0.1899441, 0.0545106, 0.8003868,
                         1.0480679, 0.4157985, -0.1583851, -0.0986099, 0.0781768, 0.0182609
                       };

        float f2[12] = { -0.0062840, 0.0809189, -0.0381061, -0.1948991, 0.2226868, 0.9503798,
                         0.9503798, 0.2226868, -0.1948991, -0.0381061, 0.0809189, -0.0062840
                       };

        float f3[12] = { 0.0182609, 0.0781768, -0.0986099, -0.1583851, 0.4157985, 1.0480679,
                         0.8003868, 0.0545106, -0.1899441, 0.0164026, 0.0680333, -0.0241806
                       };

        SigProc::FIRPolyphaseFilterBank f("rrcos", k, m, beta, N);

        TS_ASSERT_EQUALS( f.GetNumFilters(), N );

        // 12 = 2*k*m
        TS_ASSERT_EQUALS( f.GetFilterLength(), 12 );

        float * H = f.GetFilterBankCoefficients();

        for (unsigned int i=0; i<12; i++) {
            // Need to multiply by 'k' to compensate for filterbank scaling
            TS_ASSERT_DELTA( H[0 +i]*k, f0[i], 0.00001f );
            TS_ASSERT_DELTA( H[12+i]*k, f1[i], 0.00001f );
            TS_ASSERT_DELTA( H[24+i]*k, f2[i], 0.00001f );
            TS_ASSERT_DELTA( H[36+i]*k, f3[i], 0.00001f );
        }
    }

    void testDerivativeRRCCoefficientGeneration() {
        // Initialize variables
        unsigned int k(2), m(3), N(4);
        float beta(0.3f);

        // Initialize pre-determined coefficient arrays
        float f0[12] = { 1.0888e-10,  2.4886e-02, -3.0887e-02, -4.5667e-02,  1.0645e-01,  1.9229e-01,
                         -5.3702e-10, -1.9229e-01, -1.0645e-01,  4.5667e-02,  3.0887e-02, -2.4886e-02
                       };

        float f1[12] = { 1.3416e-02,  1.7952e-02, -4.7352e-02, -2.0648e-02,  1.4884e-01,  1.6752e-01,
                         -6.5796e-02, -1.9633e-01, -5.9949e-02,  5.7749e-02 , 1.2161e-02, -2.5650e-02
                       };

        float f2[12] = { 2.1221e-02,  5.0718e-03, -5.7506e-02,  1.5779e-02,  1.8064e-01,  1.2384e-01,
                         -1.2384e-01, -1.8064e-01, -1.5779e-02,  5.7506e-02, -5.0718e-03, -2.1221e-02
                       };

        float f3[12] = { 2.5650e-02, -1.2161e-02, -5.7749e-02,  5.9949e-02,  1.9633e-01,  6.5796e-02,
                         -1.6752e-01, -1.4884e-01,  2.0648e-02,  4.7352e-02, -1.7952e-02, -1.3416e-02
                       };

        SigProc::FIRPolyphaseFilterBank f("drrcos", k, m, beta, N);

        TS_ASSERT_EQUALS( f.GetNumFilters(), N );

        // 12 = 2*k*m
        TS_ASSERT_EQUALS( f.GetFilterLength(), 12 );

        float * H = f.GetFilterBankCoefficients();

        for (unsigned int i=0; i<12; i++) {
            // Need to multiply by 'k' to compensate for filterbank scaling
            TS_ASSERT_DELTA( H[0 +i]*k, f0[i], 0.00001f );
            TS_ASSERT_DELTA( H[12+i]*k, f1[i], 0.00001f );
            TS_ASSERT_DELTA( H[24+i]*k, f2[i], 0.00001f );
            TS_ASSERT_DELTA( H[36+i]*k, f3[i], 0.00001f );
        }

    }

    void testLowSignalLevel() {
        // Precision underflow
    }

    void testHighSignalLevel() {
        // Precision overflow

        // Initialize variables
        float h[1] = {2*float(SHRT_MAX)};
        short y;        // output

        // Load filter coefficients externally
        SigProc::FIRPolyphaseFilterBank f(h, 1, 1);

        f.PushInput(10);
        f.ComputeOutput(y, 0);
        TS_ASSERT_EQUALS(y, SHRT_MAX);

        f.ResetBuffer();

        f.PushInput(-10);
        f.ComputeOutput(y, 0);
        TS_ASSERT_EQUALS(y, SHRT_MIN);
    }

    void testImpulseResponse() {

        // Initialize variables
        float h[10] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
        short c(10000); // impulse amplitude
        short y;        // output

        // Load filter coefficients externally
        SigProc::FIRPolyphaseFilterBank f(h, 10, 1);

        TS_ASSERT_EQUALS( f.GetNumFilters(), 1 );

        TS_ASSERT_EQUALS( f.GetFilterLength(), 10 );

        // Hit the filter with an impulse
        f.PushInput(c);

        // Resulting output should be equal to filter coefficients
        for (unsigned int i=0; i<10; i++) {
            f.ComputeOutput(y, 0);
            TS_ASSERT_DELTA( h[10-i-1]*float(c), float(y), 2.0f );
            f.PushInput(0);
        }

        // Impulse response should be finite
        for (unsigned int i=0; i<10; i++) {
            f.ComputeOutput(y, 0);
            TS_ASSERT_DELTA( 0.0f, float(y), 0.001 );
            f.PushInput(0);
        }

    }

    void testFilterBankSelection1() {

        // Initialize variables
        float h[15] = {1.0,  2.0,  3.0,  4.0,  5.0,     // bank 0
                       6.0,  7.0,  8.0,  9.0,  0.0,     // bank 1
                       -1.0, -2.0, -3.0, -4.0, -5.0
                      };    // bank 2
        short c(1000); // impulse amplitude
        short y;        // output
        unsigned int b; // filter bank index

        // Load filter coefficients externally
        SigProc::FIRPolyphaseFilterBank f(h, 5, 3);

        TS_ASSERT_EQUALS( f.GetNumFilters(), 3 );

        TS_ASSERT_EQUALS( f.GetFilterLength(), 5 );

        for (unsigned int k=0; k<25; k++) {
            // Hit the filter with an impulse
            f.PushInput(c);

            // Resulting output should be equal to filter coefficients at
            // randomly-chosen index 'b'
            for (unsigned int i=0; i<5; i++) {
                b = rand() % 3;
                f.ComputeOutput(y, b);
                TS_ASSERT_EQUALS( short(h[b*5+5-i-1])*c, y );
                f.PushInput(0);
            }
        }

    }

    void testFilterBankSelection2() {

        // Initialize variables
        float h[15] = {1.0,  1.0,  1.0,  1.0,     // bank 0
                       1.0, -1.0,  1.0, -1.0,     // bank 1
                       1.0,  2.0,  3.0,  4.0
                      };    // bank 1
        short y;        // output

        // Load filter coefficients externally
        SigProc::FIRPolyphaseFilterBank f(h, 4, 3);

        TS_ASSERT_EQUALS( f.GetNumFilters(), 3 );

        TS_ASSERT_EQUALS( f.GetFilterLength(), 4 );

        // test input 1
        f.PushInput(10);
        f.PushInput(10);
        f.PushInput(10);
        f.PushInput(10);

        f.ComputeOutput(y, 0);
        TS_ASSERT_EQUALS(y, 40);

        f.ComputeOutput(y, 1);
        TS_ASSERT_EQUALS(y, 0);

        f.ComputeOutput(y, 2);
        TS_ASSERT_EQUALS(y, 100);

        // test input 2
        f.PushInput(10);
        f.PushInput(-10);
        f.PushInput(10);
        f.PushInput(-10);

        f.ComputeOutput(y, 0);
        TS_ASSERT_EQUALS(y, 0);

        f.ComputeOutput(y, 1);
        TS_ASSERT_EQUALS(y, 40);

        f.ComputeOutput(y, 2);
        TS_ASSERT_EQUALS(y, -20);

        // test input 3
        f.PushInput(10);
        f.PushInput(10);
        f.PushInput(-10);
        f.PushInput(-10);

        f.ComputeOutput(y, 0);
        TS_ASSERT_EQUALS(y, 0);

        f.ComputeOutput(y, 1);
        TS_ASSERT_EQUALS(y, 0);

        f.ComputeOutput(y, 2);
        TS_ASSERT_EQUALS(y, -40);

    }


    void testFilterBankSelectionExternalBuffer() {

        // Initialize variables
        float h[15] = {1.0,  1.0,  1.0,  1.0,     // bank 0
                       1.0, -1.0,  1.0, -1.0,     // bank 1
                       1.0,  2.0,  3.0,  4.0
                      };    // bank 1
        short y;        // output

        // Load filter coefficients externally
        SigProc::FIRPolyphaseFilterBank f(h, 4, 3);

        // Create circular buffer for external array
        SigProc::CircularBuffer <short> v(4);

        TS_ASSERT_EQUALS( f.GetNumFilters(), 3 );

        TS_ASSERT_EQUALS( f.GetFilterLength(), 4 );

        TS_ASSERT_EQUALS( f.GetFilterLength(), v.GetBufferSize() );

        // test input 1
        v.Push(10);
        v.Push(10);
        v.Push(10);
        v.Push(10);

        f.ComputeOutput(y, 0, v.GetHeadPtr());
        TS_ASSERT_EQUALS(y, 40);

        f.ComputeOutput(y, 1, v.GetHeadPtr());
        TS_ASSERT_EQUALS(y, 0);

        f.ComputeOutput(y, 2, v.GetHeadPtr());
        TS_ASSERT_EQUALS(y, 100);

        // test input 2
        v.Push(10);
        v.Push(-10);
        v.Push(10);
        v.Push(-10);

        f.ComputeOutput(y, 0, v.GetHeadPtr());
        TS_ASSERT_EQUALS(y, 0);

        f.ComputeOutput(y, 1, v.GetHeadPtr());
        TS_ASSERT_EQUALS(y, 40);

        f.ComputeOutput(y, 2, v.GetHeadPtr());
        TS_ASSERT_EQUALS(y, -20);

        // test input 3
        v.Push(10);
        v.Push(10);
        v.Push(-10);
        v.Push(-10);

        f.ComputeOutput(y, 0, v.GetHeadPtr());
        TS_ASSERT_EQUALS(y, 0);

        f.ComputeOutput(y, 1, v.GetHeadPtr());
        TS_ASSERT_EQUALS(y, 0);

        f.ComputeOutput(y, 2, v.GetHeadPtr());
        TS_ASSERT_EQUALS(y, -40);

    }

};


#endif // __FIRPOLYPHASEFILTERBANKTEST_H

