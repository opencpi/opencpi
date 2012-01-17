#ifndef __CIRCULARBUFFERTEST_H
#define __CIRCULARBUFFERTEST_H

#include <cxxtest/TestSuite.h>
#include "../SigProc.h"

//
// A simple test suite: Just inherit CxxTest::TestSuite and write tests!
//

class CircularBuffer_Testsuite : public CxxTest::TestSuite
{
public:

    void testDefaultConstructor() {
        // Initialize buffer
        SigProc::CircularBuffer <int> b;

        TS_ASSERT_EQUALS( b.GetNumElements(), 0 );
        TS_ASSERT_EQUALS( b.GetBufferSize(), 1 );
    }

    void testInitializingEmptyConstructor() {
        // Initialize buffer
        SigProc::CircularBuffer <int> b(10);

        TS_ASSERT_EQUALS( b.GetNumElements(), 0 );
        TS_ASSERT_EQUALS( b.GetBufferSize(), 10 );
    }

    void testInitializingArrayConstructor() {
        // Initialize array
        float a[6] = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f};
        SigProc::CircularBuffer <float> b(a, 6);

        TS_ASSERT_EQUALS( b.GetNumElements(), 6 );
        TS_ASSERT_EQUALS( b.GetBufferSize(), 6 );

        // Get buffer pointer and ensure it does not equal initial array
        float * p = b.GetHeadPtr();
        TS_ASSERT_DIFFERS( a, p );

        // Ensure data are equal
        TS_ASSERT_SAME_DATA( a, p, 6*sizeof(float) );

    }

    void testInitializingCopyConstructor() {
        // Initialize first buffer
        SigProc::CircularBuffer <float> b1(10);

        // Push 4 elements
        b1.Push(  0.123f );
        b1.Push(  2.843f );
        b1.Push(  12.22f );
        b1.Push( -2.333f );

        SigProc::CircularBuffer <float> b2(b1);

        TS_ASSERT_EQUALS( b2.GetNumElements(), 4 );
        TS_ASSERT_EQUALS( b2.GetBufferSize(), 10 );

        // Get data from buffers
        float * p1, * p2;
        p1 = b1.GetHeadPtr();
        p2 = b2.GetHeadPtr();

        TS_ASSERT_SAME_DATA( p1, p2, 4*sizeof(float) );
    }

    void xtestFunctionsWhichThrowExceptions() {

        // Initialize buffer
        SigProc::CircularBuffer <int> b(5);

        TS_ASSERT_THROWS_NOTHING( b.Push(0) );
        TS_ASSERT_THROWS_NOTHING( b.Push(1) );
        TS_ASSERT_THROWS_NOTHING( b.Push(2) );
        TS_ASSERT_THROWS_NOTHING( b.Push(3) );
        TS_ASSERT_THROWS_NOTHING( b.Push(4) );
        TS_ASSERT_THROWS_NOTHING( b.Push(5) );
        TS_ASSERT_THROWS_NOTHING( b.Push(6) );

        TS_ASSERT_THROWS_NOTHING( b.Pop() );

        TS_ASSERT_THROWS_NOTHING( b.Push(0) );

        b.Release();

        // Buffer should be empty
        TS_ASSERT_THROWS( b.Pop(), int );
    }

    void testBufferFunctions() {

        // Initialize variables
        int testVector[15] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
        int zeroVector[15] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        int * testBuffer, testValue;

        // Initialize buffer
        SigProc::CircularBuffer <int> b(10);

        // Check size of buffer
        TS_ASSERT_EQUALS( b.GetBufferSize(), 10 );

        // Push values into buffer
        for (int i=0; i<10; i++)
            b.Push( testVector[i] );

        testBuffer = b.GetHeadPtr();

        // Data should be equal
        TS_ASSERT_SAME_DATA( testBuffer, testVector, 10*sizeof(int) );

        // Data length should be equal to 10
        TS_ASSERT_EQUALS( b.GetNumElements(), 10 );

        // Release the buffer
        b.Release();

        // Buffer should be empty
        TS_ASSERT_EQUALS( b.GetNumElements(), 0 );

        testBuffer = b.GetHeadPtr();

        // Buffer should be full of zeros
        TS_ASSERT_SAME_DATA( testBuffer, zeroVector, 10*sizeof(int) );

        for (int i=0; i<5; i++) {
            b.Push( testVector[i] );
            testValue = b.Pop();
            TS_ASSERT_EQUALS( testVector[i], testValue );
        }

        // Buffer should be empty
        TS_ASSERT_EQUALS( b.GetNumElements(), 0 );

        for (int i=0; i<10; i++)
            b.Push( testVector[i+3] );

        // Buffer should have exactly 10 values
        TS_ASSERT_EQUALS( b.GetNumElements(), 10 );

        testBuffer = b.GetHeadPtr();

        // Data should be equal
        TS_ASSERT_SAME_DATA( testBuffer, testVector+3, 10*sizeof(int) );

        // Release 4 values
        b.Release(4);

        // Buffer should have exactly 6 values
        TS_ASSERT_EQUALS( b.GetNumElements(), 6 );

    }

    void testDynamicBufferSizeFunctions() {

        // Initialize variables
        int testVector[15] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
        int * testBuffer;

        // Initialize buffer
        SigProc::CircularBuffer <int> b(5);

        TS_ASSERT_EQUALS( b.GetBufferSize(), 5 );

        for (int i=0; i<5; i++)
            b.Push(testVector[i]);

        b.SetBufferSize(10);

        TS_ASSERT_EQUALS( b.GetBufferSize(), 10 );

        testBuffer = b.GetHeadPtr();

        // Data should be equal
        TS_ASSERT_SAME_DATA( testBuffer, testVector, 5*sizeof(int) );

        // Reset
        b.Release();

        // Push 10 values into buffer
        for (int i=0; i<10; i++)
            b.Push(testVector[i]);

        // Downsize buffer to only 6 elements (this should drop the first 4)
        b.SetBufferSize(6);

        testBuffer = b.GetHeadPtr();

        // Data should be equal
        TS_ASSERT_SAME_DATA( testBuffer, testVector+4, 6*sizeof(int) );

    }

    void testOperatorOverloads() {

        int testVector[15] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};

        // initialize buffer to number unequal to testVector length
        SigProc::CircularBuffer <int> b(24);

        for (int i=0; i<4; i++) {
            for (int j=0; j<15; j++)
                b.Push( testVector[j] );

            for (int j=0; j<15; j++)
                TS_ASSERT_EQUALS( b[j], testVector[j] );

            for (int j=0; j<15; j++)
                b.Pop();
        }
    }

};


#endif // __CIRCULARBUFFERTEST_H

