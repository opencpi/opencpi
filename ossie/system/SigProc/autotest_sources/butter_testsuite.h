#ifndef __BUTTER_TEST_H__
#define __BUTTER_TEST_H__

#include <cxxtest/TestSuite.h>
#include "../SigProc.h"

//
// A simple test suite: Just inherit CxxTest::TestSuite and write tests!
//

class design_butter_lowpass_filter_Testsuite : public CxxTest::TestSuite
{
public:

//
// Design 10th order low-pass filter
// MATLAB: [b, a] = butter(10, 0.25);
//
    void test_01() {
        float b_test[11] = {
            1.105590991735961e-005,
            1.105590991735961e-004,
            4.975159462811823e-004,
            1.326709190083153e-003,
            2.321741082645518e-003,
            2.786089299174621e-003,
            2.321741082645518e-003,
            1.326709190083153e-003,
            4.975159462811823e-004,
            1.105590991735961e-004,
            1.105590991735961e-005
        };

        float a_test[11] = {
            1.000000000000000e+000,
            -4.986985260650191e+000,
            1.193643683523469e+001,
            -1.774237180961925e+001,
            1.797322796966024e+001,
            -1.288624174593428e+001,
            6.593202212793411e+000,
            -2.369091691148871e+000,
            5.706327055500219e-001,
            -8.301767850386463e-002,
            5.529714373462724e-003
        };

        float b[11];
        float a[11];

        SigProc::design_butter_lowpass_filter(10, 0.25f, b, a);

        for (unsigned int i=0; i<11; i++) {
            TS_ASSERT_DELTA( b[i], b_test[i], 1E-08 );
            TS_ASSERT_DELTA( a[i], a_test[i], 1E-08 );
        }
    }

//
// Design 7th order low-pass filter
// MATLAB: [b, a] = butter(7, 0.5);
//
    void test_02() {
        float b_test[8] = {
            1.656529381997260e-002,
            1.159570567398082e-001,
            3.478711702194247e-001,
            5.797852836990410e-001,
            5.797852836990410e-001,
            3.478711702194247e-001,
            1.159570567398082e-001,
            1.656529381997260e-002
        };

        float a_test[8] = {
            1.000000000000000e+000,
            -8.430756093247283e-016,
            9.199730030568897e-001,
            -5.625706272110748e-016,
            1.927011550380281e-001,
            -9.603779416448762e-017,
            7.683450861576624e-003,
            -4.265172027549182e-019
        };

        float b[8];
        float a[8];

        SigProc::design_butter_lowpass_filter(7, 0.5f, b, a);

        for (unsigned int i=0; i<8; i++) {
            TS_ASSERT_DELTA( b[i], b_test[i], 1E-08 );
            TS_ASSERT_DELTA( a[i], a_test[i], 1E-08 );
        }
    }
};


#endif // __BUTTER_TEST_H__

