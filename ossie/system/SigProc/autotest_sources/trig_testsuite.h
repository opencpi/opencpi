#ifndef __TRIG_TEST_H__
#define __TRIG_TEST_H__

#include <cxxtest/TestSuite.h>
#include "../SigProc.h"

//
// A simple test suite: Just inherit CxxTest::TestSuite and write tests!
//

class arctan_Testsuite : public CxxTest::TestSuite
{
public:

//
// arctan( float&, float&, float&)
//

    void test_float_float_float_limits() {
        float x, y, theta;

        x = 0.0f;
        y = 1.0f;
        arctan(x, y, theta);
        TS_ASSERT_DELTA(theta, PI/2, 1e-9);

        x = 0.0f;
        y = -1.0f;
        arctan(x, y, theta);
        TS_ASSERT_DELTA(theta, 3*PI/2, 1e-9);

        x = 0.0f;
        y = 0.0f;
        arctan(x, y, theta);
        TS_ASSERT_DELTA(theta, PI/2, 1e-9);

        x = 1.0;
        y = 0.0f;
        arctan(x, y, theta);
        TS_ASSERT_DELTA(theta, 0, 1e-9);

    }


    void test_float_float_float_quadrants() {
        float x, y, theta;

        // Q1
        x = 1.0f;
        y = 1.0f;
        arctan(x, y, theta);
        TS_ASSERT_DELTA(theta, PI/4, 1e-9);

        // Q2
        x = -1.0f;
        y = 1.0f;
        arctan(x, y, theta);
        TS_ASSERT_DELTA(theta, 3*PI/4, 1e-9);

        // Q3
        x = -1.0f;
        y = -1.0f;
        arctan(x, y, theta);
        TS_ASSERT_DELTA(theta, 5*PI/4, 1e-9);

        // Q4
        x = 1.0f;
        y = -1.0f;
        arctan(x, y, theta);
        TS_ASSERT_DELTA(theta, 7*PI/4, 1e-9);

    }


};

class rotate_Testsuite : public CxxTest::TestSuite
{
public:
    void test_rotate_01() {
        short I_out, Q_out;

        // rotate by 180 degrees
        rotate(1000, 0, PI, &I_out, &Q_out);
        TS_ASSERT_DELTA(I_out, -1000, 1);
        TS_ASSERT_DELTA(Q_out,     0, 1);

        // rotate by 90 degrees
        rotate(1000, 0, PI/2, &I_out, &Q_out);
        TS_ASSERT_DELTA(I_out,     0, 1);
        TS_ASSERT_DELTA(Q_out,  1000, 1);

        // rotate by -90 degrees
        rotate(1000, 0, -PI/2, &I_out, &Q_out);
        TS_ASSERT_DELTA(I_out,     0, 1);
        TS_ASSERT_DELTA(Q_out, -1000, 1);

        // rotate by 45 degrees
        rotate(1000, 0, PI/4, &I_out, &Q_out);
        TS_ASSERT_DELTA(I_out, 707, 1);
        TS_ASSERT_DELTA(Q_out, 707, 1);

        // rotate by 135 degrees
        rotate(1000, 0, 3*PI/4, &I_out, &Q_out);
        TS_ASSERT_DELTA(I_out, -707, 1);
        TS_ASSERT_DELTA(Q_out,  707, 1);

        // rotate by 10 degrees
        rotate(1000, 0, 10.0f*PI/180.0f, &I_out, &Q_out);
        TS_ASSERT_DELTA(I_out, 985, 1);
        TS_ASSERT_DELTA(Q_out, 174, 1);

        // rotate by 112 degrees
        rotate(1000, 0, -112.0f*PI/180.0f, &I_out, &Q_out);
        TS_ASSERT_DELTA(I_out, -375, 1);
        TS_ASSERT_DELTA(Q_out, -927, 1);

    }
};

#endif // __TRIG_TEST_H__

