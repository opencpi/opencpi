#ifndef __MODEMTEST_H__
#define __MODEMTEST_H__

#include <cxxtest/TestSuite.h>
#include "../SigProc.h"

using namespace SigProc;

//
// A simple test suite: Just inherit CxxTest::TestSuite and write tests!
//

// NOTE: " : public CxxTest::TestSuite" must be on the same line as the class
//       definition, otherwise the python script does not recognize it
//       as a test class
class Modulator_testsuite1 : public CxxTest::TestSuite
{
public:

    void test_ModulateBPSK() {
        char bitsIn[2] = {0, 1};
        short I_test[2] = {-BPSK_LEVEL, BPSK_LEVEL};
        short Q_test[2] = {0, 0};

        short * I = new short[2];
        short * Q = new short[2];

        short symbol;
        for (unsigned int i=0; i<2; i++) {
            symbol = short(bitsIn[i]);
            ModulateBPSK(symbol, I[i], Q[i]);
        }

        for (unsigned int i=0; i<2; i++) {
            TS_ASSERT_EQUALS(I[i], I_test[i]);
            TS_ASSERT_EQUALS(Q[i], Q_test[i]);
        }

        delete [] I;
        delete [] Q;
    }

    void test_ModulateQPSK() {
        char bitsIn[8] = {0, 0, 0, 1, 1, 0, 1, 1};
        short I_test[4] = {QPSK_LEVEL,           0,           0, -QPSK_LEVEL};
        short Q_test[4] = {         0, -QPSK_LEVEL,  QPSK_LEVEL,           0};

        short * I = new short[4];
        short * Q = new short[4];

        short symbol;
        unsigned int j(0);
        for (unsigned int i=0; i<8; i+=2, ++j) {
            symbol = bitsIn[i+1] + (bitsIn[i] << 1);
            ModulateQPSK(symbol, I[j], Q[j]);
        }

        for (unsigned int i=0; i<4; i++) {
            TS_ASSERT_EQUALS(I[i], I_test[i]);
            TS_ASSERT_EQUALS(Q[i], Q_test[i]);
        }

        delete [] I;
        delete [] Q;
    }



    void test_ModulateQAM4() {
        char bitsIn[8] = {0, 0, 0, 1, 1, 0, 1, 1};
        short I_test[4] = {QAM4_LEVEL,  QAM4_LEVEL, -QAM4_LEVEL, -QAM4_LEVEL};
        short Q_test[4] = {QAM4_LEVEL, -QAM4_LEVEL,  QAM4_LEVEL, -QAM4_LEVEL};

        short * I = new short[4];
        short * Q = new short[4];

        short symbol;
        unsigned int j(0);
        for (unsigned int i=0; i<8; i+=2, ++j) {
            symbol = bitsIn[i+1] + (bitsIn[i] << 1);
            ModulateQAM4(symbol, I[j], Q[j]);
        }

        for (unsigned int i=0; i<4; i++) {
            TS_ASSERT_EQUALS(I[i], I_test[i]);
            TS_ASSERT_EQUALS(Q[i], Q_test[i]);
        }

        delete [] I;
        delete [] Q;
    }


    void test_Modulate8PSK() {
        char bitsIn[24] = {0, 0, 0,
                           0, 0, 1,
                           0, 1, 0,
                           0, 1, 1,
                           1, 0, 0,
                           1, 0, 1,
                           1, 1, 0,
                           1, 1, 1
                          };

        short I_test[8] = { PSK8_LEVEL_2,
                            PSK8_LEVEL_1,
                            PSK8_LEVEL_1,
                            0,
                            -PSK8_LEVEL_1,
                            0,
                            -PSK8_LEVEL_2,
                            -PSK8_LEVEL_1
                          };

        short Q_test[8] = {            0,
                                       PSK8_LEVEL_1,
                                       -PSK8_LEVEL_1,
                                       -PSK8_LEVEL_2,
                                       PSK8_LEVEL_1,
                                       PSK8_LEVEL_2,
                                       0,
                                       -PSK8_LEVEL_1
                          };

        short * I = new short[8];
        short * Q = new short[8];

        short symbol;
        unsigned int j(0);
        for (unsigned int i=0; i<24; i+=3, ++j) {
            symbol = bitsIn[i+2] + (bitsIn[i+1] << 1) + (bitsIn[i] << 2);
            Modulate8PSK(symbol, I[j], Q[j]);
        }

        for (unsigned int i=0; i<8; i++) {
            TS_ASSERT_EQUALS(I[i], I_test[i]);
            TS_ASSERT_EQUALS(Q[i], Q_test[i]);
        }

        delete [] I;
        delete [] Q;
    }

    void test_Modulate16QAM() {
        short symbolsIn[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        short I_test[16] = {
            -QAM16_LEVEL_2, // 0
            -QAM16_LEVEL_2, // 1
            -QAM16_LEVEL_2, // 2
            -QAM16_LEVEL_2, // 3
            -QAM16_LEVEL_1, // 4
            -QAM16_LEVEL_1, // 5
            -QAM16_LEVEL_1, // 6
            -QAM16_LEVEL_1, // 7
            QAM16_LEVEL_2, // 8
            QAM16_LEVEL_2, // 9
            QAM16_LEVEL_2, // 10
            QAM16_LEVEL_2, // 11
            QAM16_LEVEL_1, // 12
            QAM16_LEVEL_1, // 13
            QAM16_LEVEL_1, // 14
            QAM16_LEVEL_1, // 15
        };

        short Q_test[16] = {
            QAM16_LEVEL_2, // 0
            QAM16_LEVEL_1, // 1
            -QAM16_LEVEL_2, // 2
            -QAM16_LEVEL_1, // 3
            QAM16_LEVEL_2, // 4
            QAM16_LEVEL_1, // 5
            -QAM16_LEVEL_2, // 6
            -QAM16_LEVEL_1, // 7
            QAM16_LEVEL_2, // 8
            QAM16_LEVEL_1, // 9
            -QAM16_LEVEL_2, // 10
            -QAM16_LEVEL_1, // 11
            QAM16_LEVEL_2, // 12
            QAM16_LEVEL_1, // 13
            -QAM16_LEVEL_2, // 14
            -QAM16_LEVEL_1, // 15
        };

        short * I = new short[16];
        short * Q = new short[16];

        for (unsigned int i=0; i<16; i++) {
            Modulate16QAM(symbolsIn[i], I[i], Q[i]);
        }

        for (unsigned int i=0; i<16; i++) {
            TS_ASSERT_EQUALS(I[i], I_test[i]);
            TS_ASSERT_EQUALS(Q[i], Q_test[i]);
        }

        delete [] I;
        delete [] Q;
    }

    void test_Modulate4PAM() {
        short symbolsIn[4] = {0, 1 ,2, 3};
        short I_test[4] = {PAM4_LEVEL_2,  PAM4_LEVEL_1, -PAM4_LEVEL_2, -PAM4_LEVEL_1};
        short Q_test[4] = {0, 0, 0, 0};

        short * I = new short[4];
        short * Q = new short[4];

        for (unsigned int i=0; i<4; i++) {
            Modulate4PAM(symbolsIn[i], I[i], Q[i]);
            TS_ASSERT_EQUALS(I[i], I_test[i]);
            TS_ASSERT_EQUALS(Q[i], Q_test[i]);
        }

        delete [] I;
        delete [] Q;
    }

};

class Demodulator_testsuite1 : public CxxTest::TestSuite
{
public:

    void test_DemodulateBPSK() {
        short I_in[2] = {-10, 10};
        short Q_in[2] = {0, 0};

        short symbols_test[2] = {0, 1};

        short * symbols = new short[2];

        for (unsigned int i=0; i<2; i++) {
            DemodulateBPSK(I_in[i], Q_in[i], symbols[i]);
            TS_ASSERT_EQUALS(symbols[i], symbols_test[i]);
        }

        delete [] symbols;
    }

    void test_DemodulateQPSK() {
        short I_in[4] = {
            10,    // 0
            0,    // 1
            0,    // 2
            -10     // 3
        };

        short Q_in[4] = {
            0,    // 0
            -10,    // 1
            10,    // 2
            0     // 3
        };

        short symbols_test[4] = {0, 1, 2, 3};

        short * symbols = new short[4];

        for (unsigned int i=0; i<4; i++) {
            DemodulateQPSK(I_in[i], Q_in[i], symbols[i]);
            TS_ASSERT_EQUALS(symbols[i], symbols_test[i]);
        }

        delete [] symbols;
    }

    void test_DemodulateQAM4() {
        short I_in[4] = {
            10,    // 0
            10,    // 1
            -10,    // 2
            -10     // 3
        };

        short Q_in[4] = {
            10,    // 0
            -10,    // 1
            10,    // 2
            -10     // 3
        };

        short symbols_test[4] = {0, 1, 2, 3};

        short * symbols = new short[4];

        for (unsigned int i=0; i<4; i++) {
            DemodulateQAM4(I_in[i], Q_in[i], symbols[i]);
            TS_ASSERT_EQUALS(symbols[i], symbols_test[i]);
        }

        delete [] symbols;
    }

    void test_Demodulate8PSK() {
        short I_in[8] = {
            PSK8_LEVEL_2,  // 0
            PSK8_LEVEL_1,  // 1
            PSK8_LEVEL_1,  // 2
            0,  // 3
            -PSK8_LEVEL_1,  // 4
            0,  // 5
            -PSK8_LEVEL_2,  // 6
            -PSK8_LEVEL_1,  // 7
        };

        short Q_in[8] = {
            0,  // 0
            PSK8_LEVEL_1,  // 1
            -PSK8_LEVEL_1,  // 2
            -PSK8_LEVEL_2,  // 3
            PSK8_LEVEL_1,  // 4
            PSK8_LEVEL_2,  // 5
            0,  // 6
            -PSK8_LEVEL_1,  // 7
        };

        short symbols_test[8] = {0, 1, 2, 3, 4, 5, 6, 7};

        short * symbols = new short[8];

        for (unsigned int i=0; i<8; i++) {
            Demodulate8PSK(I_in[i], Q_in[i], symbols[i]);
            TS_ASSERT_EQUALS(symbols[i], symbols_test[i]);
        }

        delete [] symbols;
    }

    void test_Demodulate16QAM() {
        short I_in[16] = {
            -QAM16_LEVEL_2, // 0
            -QAM16_LEVEL_2, // 1
            -QAM16_LEVEL_2, // 2
            -QAM16_LEVEL_2, // 3
            -QAM16_LEVEL_1, // 4
            -QAM16_LEVEL_1, // 5
            -QAM16_LEVEL_1, // 6
            -QAM16_LEVEL_1, // 7
            QAM16_LEVEL_2, // 8
            QAM16_LEVEL_2, // 9
            QAM16_LEVEL_2, // 10
            QAM16_LEVEL_2, // 11
            QAM16_LEVEL_1, // 12
            QAM16_LEVEL_1, // 13
            QAM16_LEVEL_1, // 14
            QAM16_LEVEL_1, // 15
        };

        short Q_in[16] = {
            QAM16_LEVEL_2, // 0
            QAM16_LEVEL_1, // 1
            -QAM16_LEVEL_2, // 2
            -QAM16_LEVEL_1, // 3
            QAM16_LEVEL_2, // 4
            QAM16_LEVEL_1, // 5
            -QAM16_LEVEL_2, // 6
            -QAM16_LEVEL_1, // 7
            QAM16_LEVEL_2, // 8
            QAM16_LEVEL_1, // 9
            -QAM16_LEVEL_2, // 10
            -QAM16_LEVEL_1, // 11
            QAM16_LEVEL_2, // 12
            QAM16_LEVEL_1, // 13
            -QAM16_LEVEL_2, // 14
            -QAM16_LEVEL_1, // 15
        };

        short symbols_test[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

        short * symbols = new short[16];

        for (unsigned int i=0; i<16; i++) {
            Demodulate16QAM(I_in[i], Q_in[i], symbols[i]);
            TS_ASSERT_EQUALS(symbols[i], symbols_test[i]);
        }

        delete [] symbols;
    }

    void test_Demodulate4PAM() {
        short I_in[4] = {
            PAM4_LEVEL_2,    // 0
            PAM4_LEVEL_1,    // 1
            -PAM4_LEVEL_2,    // 2
            -PAM4_LEVEL_1     // 3
        };

        short Q_in[4] = {0, 0, 0, 0};

        short symbols_test[4] = {0, 1, 2, 3};

        short * symbols = new short[4];

        for (unsigned int i=0; i<4; i++) {
            Demodulate4PAM(I_in[i], Q_in[i], symbols[i]);
            TS_ASSERT_EQUALS(symbols[i], symbols_test[i]);
        }

        delete [] symbols;
    }

};

class Modem_testsuite1 : public CxxTest::TestSuite
{
public:

    void test_ModemBPSK() {
        short I, Q, s;
        for (unsigned int i=0; i<2; i++) {
            ModulateBPSK(i, I, Q);
            DemodulateBPSK(I, Q, s);
            TS_ASSERT_EQUALS(i, s);
        }
    }

    void test_ModemQPSK() {
        short I, Q, s;
        for (unsigned int i=0; i<4; i++) {
            ModulateQPSK(i, I, Q);
            DemodulateQPSK(I, Q, s);
            TS_ASSERT_EQUALS(i, s);
        }
    }

    void test_ModemQAM4() {
        short I, Q, s;
        for (unsigned int i=0; i<4; i++) {
            ModulateQAM4(i, I, Q);
            DemodulateQAM4(I, Q, s);
            TS_ASSERT_EQUALS(i, s);
        }
    }

    void test_Modem8PSK() {
        short I, Q, s;
        for (unsigned int i=0; i<8; i++) {
            Modulate8PSK(i, I, Q);
            Demodulate8PSK(I, Q, s);
            TS_ASSERT_EQUALS(i, s);
        }
    }

    void test_Modem16QAM() {
        short I, Q, s;
        for (unsigned int i=0; i<16; i++) {
            Modulate16QAM(i, I, Q);
            Demodulate16QAM(I, Q, s);
            TS_ASSERT_EQUALS(i, s);
        }
    }

    void test_Modem4PAM() {
        short I, Q, s;
        for (unsigned int i=0; i<4; i++) {
            Modulate4PAM(i, I, Q);
            Demodulate4PAM(I, Q, s);
            TS_ASSERT_EQUALS(i, s);
        }
    }

};

#endif

