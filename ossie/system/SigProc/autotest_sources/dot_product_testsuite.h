#ifndef __DOT_PRODUCT_TEST_H__
#define __DOT_PRODUCT_TEST_H__

#include <cxxtest/TestSuite.h>
#include "../SigProc.h"

//
// A simple test suite: Just inherit CxxTest::TestSuite and write tests!
//

class dot_product_Testsuite : public CxxTest::TestSuite
{
public:

//
// dot_product( float*, float*, unsigned int, float&)
//

    void test_float_float_float_01() {
        float x[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        float y[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        float z;
        dot_product(x, y, 4, z);
        TS_ASSERT_EQUALS(z, 0.0f);
    }

    void test_float_float_float_02() {
        float x[4] = {100.0f, 100.0f, 100.0f, 100.0f};
        float y[4] = {100.0f, 100.0f, 100.0f, 100.0f};
        float z;
        dot_product(x, y, 4, z);
        TS_ASSERT_EQUALS(z, 40e3f);
    }

    void test_float_float_float_03() {
        float x[4] = { 10.0f, -10.0f,  10.0f, -10.0f};
        float y[4] = { 10.0f, -10.0f,  10.0f, -10.0f};
        float z;
        dot_product(x, y, 4, z);
        TS_ASSERT_EQUALS(z, 400);
    }

    void test_float_float_float_04() {
        float x[4] = {-10.0f,  10.0f, -10.0f,  10.0f};
        float y[4] = { 10.0f, -10.0f,  10.0f, -10.0f};
        float z;
        dot_product(x, y, 4, z);
        TS_ASSERT_EQUALS(z, -400.0f);
    }

    void test_float_float_float_05() {
        float x[4] = { 10.0f,  10.0f, -10.0f, -10.0f};
        float y[4] = { 10.0f,  10.0f,  10.0f,  10.0f};
        float z;
        dot_product(x, y, 4, z);
        TS_ASSERT_EQUALS(z, 0.0f);
    }


//
// dot_product( short*, short*, unsigned int, short&)
//

    void test_short_short_short_01() {
        short x[4] = {0, 0, 0, 0};
        short y[4] = {0, 0, 0, 0};
        short z;
        dot_product(x, y, 4, z);
        TS_ASSERT_EQUALS(z, 0);
    }

    void test_short_short_short_02() {
        short x[4] = {100, 100, 100, 100};
        short y[4] = {100, 100, 100, 100};
        short z;
        dot_product(x, y, 4, z);
        TS_ASSERT_EQUALS(z, SHRT_MAX);
    }

    void test_short_short_short_03() {
        short x[4] = { 10, -10,  10, -10};
        short y[4] = { 10, -10,  10, -10};
        short z;
        dot_product(x, y, 4, z);
        TS_ASSERT_EQUALS(z, 400);
    }

    void test_short_short_short_04() {
        short x[4] = {-10,  10, -10,  10};
        short y[4] = { 10, -10,  10, -10};
        short z;
        dot_product(x, y, 4, z);
        TS_ASSERT_EQUALS(z, -400);
    }

    void test_short_short_short_05() {
        short x[4] = { 10,  10, -10, -10};
        short y[4] = { 10,  10,  10,  10};
        short z;
        dot_product(x, y, 4, z);
        TS_ASSERT_EQUALS(z, 0);
    }

//
// dot_product( float*, short*, unsigned int, short&)
//

    void test_float_short_short_01() {
        float x[4] = {100.0f, 100.0f, 100.0f, 100.0f};
        short y[4] = {100,    100,    100,    100   };
        short z;
        dot_product(x, y, 4, z);
        TS_ASSERT_EQUALS(z, SHRT_MAX);
    }

//
// dot_product( float*, short*, unsigned int, float&)
//

    void test_float_short_float_01() {
        float x[4] = {100.0f, 100.0f, 100.0f, 100.0f};
        short y[4] = {100,    100,    100,    100   };
        float z;
        dot_product(x, y, 4, z);
        TS_ASSERT_EQUALS(z, 40e3f);
    }




};


#endif // __DOT_PRODUCT_TEST_H__

