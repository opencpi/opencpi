#ifndef __IIRFILTERTEST_H
#define __IIRFILTERTEST_H

#include <cxxtest/TestSuite.h>
#include "../SigProc.h"

//
// A simple test suite: Just inherit CxxTest::TestSuite and write tests!
//

class IIRFilter_Testsuite : public CxxTest::TestSuite
{
public:

    void testResponse2ndOrderButterworthFilter() {

        // initialize filter with 2nd-order low-pass butterworth filter
        float a[3] = {1.000000000000000, -0.942809041582063,  0.333333333333333};
        float b[3] = {0.0976310729378175, 0.1952621458756350, 0.0976310729378175};
        SigProc::iir_filter f(a, 3, b, 3);

        // initialize oracle; expected output (generated with octave)
        float v_impulse[15] = {
            9.76310729378175e-02,
            2.87309604180767e-01,
            3.35965474513536e-01,
            2.20981418970514e-01,
            9.63547883225231e-02,
            1.71836926400291e-02,
            -1.59173219853878e-02,
            -2.07348926322729e-02,
            -1.42432702548109e-02,
            -6.51705310050832e-03,
            -1.39657983602602e-03,
            8.55642936806248e-04,
            1.27223450919543e-03,
            9.14259886013424e-04,
            4.37894317157432e-04
        };

        float v_step[15] = {
            0.0976310729378175,
            0.3849406771185847,
            0.7209061516321208,
            0.9418875706026352,
            1.0382423589251584,
            1.0554260515651877,
            1.0395087295798000,
            1.0187738369475272,
            1.0045305666927162,
            0.9980135135922078,
            0.9966169337561817,
            0.9974725766929878,
            0.9987448112021832,
            0.9996590710881966,
            1.0000969654053542
        };

        short x, y(0);
        short c(1000);

        // hit filter with impulse, compare output
        for (unsigned int i=0; i<15; i++) {
            if (i==0)
                x = c;
            else
                x = 0;

            f.do_work(x, y);

            TS_ASSERT_DELTA( v_impulse[i]*float(c), float(y), 0.002f*float(c) );
        }

        // reset filter buffer
        f.ResetBuffer();

        // hit filter with step, compare output
        for (unsigned int i=0; i<15; i++) {
            x = c;

            f.do_work(x, y);

            TS_ASSERT_DELTA( v_step[i]*float(c), float(y), 0.002f*float(c) );
        }

    }

    void testResponse10thOrderButterworthFilter() {

        // initialize filter with 2nd-order low-pass butterworth filter
        float a[11] = {
            1.0000e+00,
            -9.9601e-01,
            1.7596e+00,
            -1.1121e+00,
            8.7474e-01,
            -3.4745e-01,
            1.4416e-01,
            -3.3071e-02,
            6.6919e-03,
            -6.7989e-04,
            3.9147e-05
        };

        float b[11] = {
            0.0012655,
            0.0126552,
            0.0569483,
            0.1518622,
            0.2657588,
            0.3189105,
            0.2657588,
            0.1518622,
            0.0569483,
            0.0126552,
            0.0012655
        };

        SigProc::iir_filter f(a, 11, b, 11);

        // initialize oracle; expected output (generated with octave)
        float v_impulse[25] = {
            0.00126551797777455,
            0.01391564815803274,
            0.06858167841372356,
            0.19709216347010877,
            0.35575957570588063,
            0.39099194423428957,
            0.19305765066333802,
            -0.09872513808128125,
            -0.19841222279283660,
            -0.04120117582379802,
            0.12130889908269875,
            0.08022920306602907,
            -0.05725352799770276,
            -0.07794813251871005,
            0.01619603266767225,
            0.06173727420236504,
            0.00666683404765441,
            -0.04309614882499248,
            -0.01717209444283530,
            0.02665484847082815,
            0.01997555516149800,
            -0.01397262483135252,
            -0.01845322199397569,
            0.00517259064735178,
            0.01491729899923814
        };

        float v_step[25] = {
            0.00126551797777455,
            0.01518116613580729,
            0.08376284454953085,
            0.28085500801963964,
            0.63661458372552027,
            1.02760652795980989,
            1.22066417862314780,
            1.12193904054186633,
            0.92352681774902989,
            0.88232564192523222,
            1.00363454100793081,
            1.08386374407395958,
            1.02661021607625691,
            0.94866208355754733,
            0.96485811622521966,
            1.02659539042758419,
            1.03326222447523830,
            0.99016607565024617,
            0.97299398120741132,
            0.99964882967823954,
            1.01962438483973705,
            1.00565176000838430,
            0.98719853801440893,
            0.99237112866176069,
            1.00728842766099858
        };

        short x, y(0);
        short c(5000);

        // hit filter with impulse, compare output
        for (unsigned int i=0; i<25; i++) {
            if (i==0)
                x = c;
            else
                x = 0;

            f.do_work(x, y);

            TS_ASSERT_DELTA( v_impulse[i]*float(c), float(y), 0.002f*float(c) );
        }

        // reset filter buffer
        f.ResetBuffer();

        // hit filter with step, compare output
        for (unsigned int i=0; i<25; i++) {
            x = c;

            f.do_work(x, y);

            TS_ASSERT_DELTA( v_step[i]*float(c), float(y), 0.002f*float(c) );
        }

    }

};


#endif // __IIRFILTERTEST_H

