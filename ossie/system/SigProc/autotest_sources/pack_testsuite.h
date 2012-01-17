#ifndef __PACK_BYTE_TEST_H__
#define __PACK_BYTE_TEST_H__

#include <cxxtest/TestSuite.h>
#include "../SigProc.h"

using namespace SigProc;

//
// A simple test suite: Just inherit CxxTest::TestSuite and write tests!
//

class pack_bytes_Testsuite : public CxxTest::TestSuite
{
public:

//
// pack_bytes
//

    void test_pack_bytes_01() {
        unsigned char output[8];
        unsigned int N;

        unsigned char input[36] = {
            0, 0, 0, 0, 0, 0, 0, 0, // 0:   0000 0000
            1, 1, 1, 1, 1, 1, 1, 1, // 255: 1111 1111
            0, 0, 0, 0, 1, 1, 1, 1, // 15:  0000 1111
            1, 0, 1, 0, 1, 0, 1, 0  // 170: 1010 1010
        };

        // Test packing entire array
        char output_test_01[4] = {0x00, 0xFF, 0x0F, 0xAA};
        pack_bytes( input, 32, output, 8, &N );
        TS_ASSERT_EQUALS( N, 4 );
        TS_ASSERT_SAME_DATA( output, output_test_01, 4 );

        // Test packing only 28 elements
        char output_test_02[4] = {0x00, 0xFF, 0x0F, 0x0A};
        pack_bytes( input, 28, output, 8, &N );
        TS_ASSERT_EQUALS( N, 4 );
        TS_ASSERT_SAME_DATA( output, output_test_02, 4 );

        // Test packing only 25 elements
        char output_test_03[4] = {0x00, 0xFF, 0x0F, 0x01};
        pack_bytes( input, 25, output, 8, &N );
        TS_ASSERT_EQUALS( N, 4 );
        TS_ASSERT_SAME_DATA( output, output_test_03, 4 );

        // Test packing only 24 elements (3 bytes)
        char output_test_04[3] = {0x00, 0xFF, 0x0F};
        pack_bytes( input, 24, output, 8, &N );
        TS_ASSERT_EQUALS( N, 3 );
        TS_ASSERT_SAME_DATA( output, output_test_04, 3 );

    }


//
// unpack_bytes
//

    void test_unpack_bytes_01() {
        unsigned char input[5] = {0x00, 0x01, 0xFF, 0x0F, 0xAA};

        unsigned char output[64];
        unsigned int N;

        char output_test[40] = {
            0, 0, 0, 0, 0, 0, 0, 0, // 0:   0000 0000
            0, 0, 0, 0, 0, 0, 0, 1, // 1:   0000 0001
            1, 1, 1, 1, 1, 1, 1, 1, // 255: 1111 1111
            0, 0, 0, 0, 1, 1, 1, 1, // 15:  0000 1111
            1, 0, 1, 0, 1, 0, 1, 0  // 170: 1010 1010
        };

        // Test packing entire array
        unpack_bytes( input, 4, output, 40, &N );
        TS_ASSERT_EQUALS( N, 32 );
        TS_ASSERT_SAME_DATA( output, output_test, 32 );

    }


//
// repack_bytes
//

    void test_repack_bytes_01() {
        // (3, 2)
        unsigned char input[] = {
            0x07,   // 111
            0x00,   // 000
            0x06,   // 110
            0x07    // 111
        };

        char output_test[] = {
            0x03,   // 11
            0x02,   // 10
            0x00,   // 00
            0x03,   // 11
            0x01,   // 01
            0x03    // 11
        };

        unsigned char output[8];
        unsigned int N;

        repack_bytes( input, 3, 4, output, 2, 8, &N );

        TS_ASSERT_EQUALS( N, 6 );
        TS_ASSERT_SAME_DATA( output, output_test, 6 );
    }

    void test_repack_bytes_02() {
        // (5, 3)
        unsigned char input[] = {
            0x01,   // 00001
            0x02,   // 00010
            0x04    // 00100
        };

        char output_test[] = {
            0x00,   // 000
            0x02,   // 010
            0x01,   // 001
            0x00,   // 000
            0x04    // 100
        };

        unsigned char output[8];
        unsigned int N;

        repack_bytes( input, 5, 3, output, 3, 8, &N );

        TS_ASSERT_EQUALS( N, 5 );
        TS_ASSERT_SAME_DATA( output, output_test, 5 );
    }

    void test_repack_bytes_03() {
        // (3, 5)
        unsigned char input[] = {
            0x00,   // 000
            0x02,   // 010
            0x01,   // 001
            0x00,   // 000
            0x04    // 100
        };

        char output_test[] = {
            0x01,   // 00001
            0x02,   // 00010
            0x04    // 00100
        };

        unsigned char output[8];
        unsigned int N;

        repack_bytes( input, 3, 5, output, 5, 8, &N );

        TS_ASSERT_EQUALS( N, 3 );
        TS_ASSERT_SAME_DATA( output, output_test, 3 );
    }

    void test_repack_bytes_04() {
        // (4, 8)
        unsigned char input[] = {
            0x00,   // 0000
            0x01,   // 0001
            0x02,   // 0010
            0x04,   // 0100
            0x04,   // 0100
            0x08    // 1000
        };

        char output_test[] = {
            0x01,   // 0000 0001
            0x24,   // 0010 0100
            0x48    // 0100 1000
        };

        unsigned char output[8];
        unsigned int N;

        repack_bytes( input, 4, 6, output, 8, 8, &N );

        TS_ASSERT_EQUALS( N, 3 );
        TS_ASSERT_SAME_DATA( output, output_test, 3 );
    }

    void test_repack_bytes_05() {
        // (1, 4)
        unsigned char input[] = {
            0x00,   // 0
            0x01,   // 1
            0x01,   // 1
            0x00,   // 0
            0x00,   // 0
            0x00,   // 0
            0x00,   // 0
            0x01    // 1
        };

        char output_test[] = {
            0x06,   // 0110
            0x01    // 0001
        };

        unsigned char output[8];
        unsigned int N;

        repack_bytes( input, 1, 8, output, 4, 8, &N );

        TS_ASSERT_EQUALS( N, 2 );
        TS_ASSERT_SAME_DATA( output, output_test, 2 );
    }

    void test_repack_bytes_06() {
        // (4, 1)
        unsigned char input[] = {
            0x06,   // 0110
            0x01    // 0001
        };

        char output_test[] = {
            0x00,   // 0
            0x01,   // 1
            0x01,   // 1
            0x00,   // 0
            0x00,   // 0
            0x00,   // 0
            0x00,   // 0
            0x01    // 1
        };

        unsigned char output[8];
        unsigned int N;

        repack_bytes( input, 4, 2, output, 1, 10, &N );

        TS_ASSERT_EQUALS( N, 8 );
        TS_ASSERT_SAME_DATA( output, output_test, 8 );
    }

//
//  Test packing bytes where input symbo/buffer size is
//  uneven with output
//
    void test_repack_bytes_07() {
        // (1, 4)
        unsigned char input[] = {
            0x01,   // 1
            0x00,   // 0
            0x00,   // 0
            0x01,   // 1
            0x01    // 1
        };

        char output_test[] = {
            0x09,   // 1001
            0x08    // 1[000]
        };

        unsigned char output[8];
        unsigned int N;

        repack_bytes( input, 1, 5, output, 4, 8, &N );

        TS_ASSERT_EQUALS( N, 2 );
        TS_ASSERT_SAME_DATA( output, output_test, 2 );
    }

//
//  Test packing bytes where input symbo/buffer size is
//  uneven with output
//
    void test_repack_bytes_08() {
        // (3, 4)
        unsigned char input[] = {
            0x01,   // 001
            0x02,   // 010
            0x03,   // 011
            0x04,   // 100
            0x05    // 101
        };

        char output_test[] = {
            0x02,   // 0010
            0x09,   // 1001
            0x0C,   // 1100
            0x0A    // 101[0]
        };

        unsigned char output[8];
        unsigned int N;

        repack_bytes( input, 3, 5, output, 4, 8, &N );

        TS_ASSERT_EQUALS( N, 4 );
        TS_ASSERT_SAME_DATA( output, output_test, 4 );
    }

};


#endif // __PACK_BYTE_TEST_H__

