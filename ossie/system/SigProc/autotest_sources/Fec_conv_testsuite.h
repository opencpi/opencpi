#ifndef __FECCONVTEST_H__
#define __FECCONVTEST_H__

#include <cxxtest/TestSuite.h>
#include "../SigProc.h"


//
// A simple test suite: Just inherit CxxTest::TestSuite and write tests!
//

// NOTE: " : public CxxTest::TestSuite" must be on the same line as the class
//       definition, otherwise the python script does not recognize it
//       as a test class
class Fec_conv_Testsuite : public CxxTest::TestSuite
{
public:

    void test_TrellisTable() {
        unsigned int testVectorNextState[64] [4] = {{0,16,32,48},{0,16,32,48},{0,16,32,48},{0,16,32,48},{1,17,33,49},{1,17,33,49},{1,17,33,49},{1,17,33,49},{2,18,34,50},{2,18,34,50},{2,18,34,50},{2,18,34,50},{3,19,35,51},{3,19,35,51},{3,19,35,51},{3,19,35,51},{4,20,36,52},{4,20,36,52},{4,20,36,52},{4,20,36,52},{5,21,37,53},{5,21,37,53},{5,21,37,53},{5,21,37,53},{6,22,38,54},{6,22,38,54},{6,22,38,54},{6,22,38,54},{7,23,39,55},{7,23,39,55},{7,23,39,55},{7,23,39,55},{8,24,40,56,},{8,24,40,56},{8,24,40,56},{8,24,40,56},{9,25,41,57},{9,25,41,57},{9,25,41,57},{9,25,41,57},{10,26,42,58},{10,26,42,58},{10,26,42,58},{10,26,42,58},{11,27,43,59},{11,27,43,59},{11,27,43,59},{11,27,43,59},{12,28,44,60},{12,28,44,60},{12,28,44,60},{12,28,44,60},{13,29,45,61},{13,29,45,61},{13,29,45,61},{13,29,45,61},{14,30,46,62},{14,30,46,62},{14,30,46,62},{14,30,46,62},{15,31,47,63},{15,31,47,63},{15,31,47,63},{15,31,47,63}};

        unsigned int testVectorOutput[64][4]={{0,3,5,6},{3,0,6,5},{5,6,0,3},{6,5,3,0},{7,4,2,1},{4,7,1,2},{2,1,7,4},{1,2,4,7},{7,4,2,1},{4,7,1,2},{2,1,7,4},{1,2,4,7},{0,3,5,6},{3,0,6,5},{5,6,0,3},{6,5,3,0},{5,6,0,3},{6,5,3,0},{0,3,5,6},{3,0,6,5},{2,1,7,4},{1,2,4,7},{7,4,2,1},{4,7,1,2},{2,1,7,4},{1,2,4,7},{7,4,2,1},{4,7,1,2},{5,6,0,3},{6,5,3,0},{0,3,5,6},{3,0,6,5},{2,1,7,4},{1,2,4,7},{7,4,2,1},{4,7,1,2},{5,6,0,3},{6,5,3,0},{0,3,5,6},{3,0,6,5},{5,6,0,3},{6,5,3,0},{0,3,5,6},{3,0,6,5},{2,1,7,4},{1,2,4,7},{7,4,2,1},{4,7,1,2},{7,4,2,1},{4,7,1,2},{2,1,7,4},{1,2,4,7},{0,3,5,6},{3,0,6,5},{5,6,0,3},{6,5,3,0},{0,3,5,6},{3,0,6,5},{5,6,0,3},{6,5,3,0},{7,4,2,1},{4,7,1,2},{2,1,7,4},{1,2,4,7},};
        unsigned int genPoly[3];
        genPoly[0]=236;//158;
        genPoly[1]=155;//109;
        genPoly[2]=337;//223;
        SigProc::trellisTable *theTrellisTable;
        theTrellisTable=new SigProc::trellisTable(genPoly,
                (short unsigned int)2,
                (short unsigned int)3,
                (short unsigned int)4);

        //std::cout<<std::endl;

        TS_ASSERT_EQUALS(theTrellisTable->numberOfTrellisStates,64);
        TS_ASSERT_EQUALS(theTrellisTable->numberOfInputStates,4);

        for (unsigned int i=0; i<theTrellisTable->numberOfTrellisStates; i++) {
            //std::cout<<i<<"|";
            // std::cout<<"{";
            for (unsigned int j=0; j<theTrellisTable->numberOfInputStates; j++) {

                //std::cout<<theTrellisTable->nextState[i][j];
                //  std::cout<<theTrellisTable->output[i][j];
                //if (j!=3) std::cout<<",";

            };

            //std::cout<<"}"<<",";//std::endl;
            TS_ASSERT_SAME_DATA(theTrellisTable->nextState[i],testVectorNextState[i],4*sizeof(unsigned int));
            TS_ASSERT_SAME_DATA(theTrellisTable->output[i],testVectorOutput[i],4*sizeof(unsigned int));
        };

        // TS_ASSERT_SAME_DATA(theTrellisTable->nextState[0],testVector1,4*sizeof(unsigned int));

        //TS_ASSERT_EQUALS(1, 2);

        delete theTrellisTable;

    };

    void testEncoder() {
        unsigned int inputVector[20]={1,0,0,0,1,0,0,1,1,1,0,1,1,0,1,0,1,1,1,0};
        unsigned int outputVector[35];
        unsigned int outputVectorTest[35]={1,1,0,1,1,1,1,1,0,1,0,1,0,1,1,0,0,1,1,0,1,1,0,1,0,1,0,1,0,1,0,0,0,0,0};
        unsigned short int data2Enc[20],encData[20],noOfSymbols=0;
        signed short int tmp;
        unsigned short int numberOfBits=0, numberOfOutBits=0;
        unsigned int genPoly[5];
        genPoly[0]=237;//159;
        genPoly[1]=274;//188;
        genPoly[2]=156;//110;
        genPoly[3]=255;//173;
        genPoly[4]=337;//223;
        SigProc::trellisTable *theTrellisTable;
        theTrellisTable=new SigProc::trellisTable(genPoly,
                (short unsigned int)4,
                (short unsigned int)5,
                (short unsigned int)2);
        SigProc::fec_conv_encoder  * encoder;
        encoder=new SigProc::fec_conv_encoder();
        encoder->SetTrellisTable(theTrellisTable);


        numberOfBits=20;
        noOfSymbols=numberOfBits/theTrellisTable->k;
        unsigned short int mode =1;

        ///Adjust the output lengh according to the encoding mode
        switch (mode) {
        case 0:
            ///The encoder starts from zero state
            numberOfOutBits=noOfSymbols*theTrellisTable->n;
            break;
        case 1:
            ///The encoder starts and ends at the zero state
            numberOfOutBits=(noOfSymbols+theTrellisTable->K)*theTrellisTable->n;
            break;
        };

        encoder->ResetState();

        for (unsigned int i=0; i<noOfSymbols; i++) {
            for (unsigned int j=0; j<theTrellisTable->k; j++) {
                data2Enc[j]=inputVector[i*theTrellisTable->k+j];
            };



            encoder->Encode(data2Enc,encData);

            for (int j=0; j<theTrellisTable->n; j++) {
                tmp=(short int )encData[j];
                outputVector[i*theTrellisTable->n+j]=tmp;
                // std::cout<<tmp<<",";
            };
        };

        ///If mode=1 add additional symbols to make the last state of the encoder zero
        if (mode==1) {

            for (unsigned int j=0; j<theTrellisTable->k; j++) {
                data2Enc[j]=0;
            };

            for (unsigned int i=noOfSymbols; i<noOfSymbols+theTrellisTable->K; i++) {

                encoder->Encode(data2Enc,encData);

                for (int j=0; j<theTrellisTable->n; j++) {
                    tmp=(short int )encData[j];
                    outputVector[i*theTrellisTable->n+j]=tmp;
                    //std::cout<<tmp<<",";
                };

            };

        }; //end if mode==1

        TS_ASSERT_SAME_DATA(outputVector,outputVectorTest,numberOfOutBits*sizeof(unsigned int));

        delete theTrellisTable;
        delete encoder;
    };

    void testEncoderAndDecoder() {
        unsigned int inputVector[20]={1,0,0,0,1,0,0,1,1,1,0,1,1,0,1,0,1,1,1,0};
        unsigned int outputVector[216];
        unsigned int outputVectorTest[216]={1,1,1,1,1,1,1,1,1,0,1,1,0,0,1,0,0,0,1,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,0,0,0,0,0,0,1,0,0,0,0,1,1,1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,1,1,0,0,0,1,0,1,1,1,0,0,0,1,1,1,0,1,0,1,1,0,1,0,0,1,1,0,0,0,0,0,0,1,0,1,1,0,0,1,1,0,0,1,1,0,0,0,1,1,1,0,0,1,1,1,0,1,1,1,1,1,1,0,1,1,1,1,1,1,0,1,1,1,1,1,1,0,0,1,0,1,0,0,0,0,0,1,1,0,0,0,0,0,1,0,0,1,1,0,0,0,0,0,1,1,0,0,0,0,1,0,1,0,0,0,0,1,1,0,1,1,0,1,1,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        unsigned short int data2Enc[20],encData[20],noOfSymbols=0;
        signed short int tmp;
        unsigned short int numberOfBits=0, numberOfOutBits=0;
        unsigned int genPoly[8];
        genPoly[0]=153;//107;
        genPoly[1]=111;//73;
        genPoly[2]=165;//117;
        genPoly[3]=173;//123;
        genPoly[4]=135;//93;
        genPoly[5]=135;//93;
        genPoly[6]=147;//103;
        genPoly[7]=137;//95;
        SigProc::trellisTable *theTrellisTable;
        theTrellisTable=new SigProc::trellisTable(genPoly,
                (short unsigned int)1,
                (short unsigned int)8,
                (short unsigned int)7);
        SigProc::fec_conv_encoder  * encoder;
        encoder=new SigProc::fec_conv_encoder();
        encoder->SetTrellisTable(theTrellisTable);


        numberOfBits=20;
        noOfSymbols=numberOfBits/theTrellisTable->k;
        unsigned short int mode =1;

        ///Adjust the output lengh according to the encoding mode
        switch (mode) {
        case 0:
            ///The encoder starts from zero state
            numberOfOutBits=noOfSymbols*theTrellisTable->n;
            break;
        case 1:
            ///The encoder starts and ends at the zero state
            numberOfOutBits=(noOfSymbols+theTrellisTable->K)*theTrellisTable->n;
            break;
        };

        encoder->ResetState();

        for (unsigned int i=0; i<noOfSymbols; i++) {
            for (unsigned int j=0; j<theTrellisTable->k; j++) {
                data2Enc[j]=inputVector[i*theTrellisTable->k+j];
            };



            encoder->Encode(data2Enc,encData);

            for (int j=0; j<theTrellisTable->n; j++) {
                tmp=(short int )encData[j];
                outputVector[i*theTrellisTable->n+j]=tmp;
                //std::cout<<tmp<<",";
            };
        };

        ///If mode=1 add additional symbols to make the last state of the encoder zero
        if (mode==1) {

            for (unsigned int j=0; j<theTrellisTable->k; j++) {
                data2Enc[j]=0;
            };

            for (unsigned int i=noOfSymbols; i<noOfSymbols+theTrellisTable->K; i++) {

                encoder->Encode(data2Enc,encData);

                for (int j=0; j<theTrellisTable->n; j++) {
                    tmp=(short int )encData[j];
                    outputVector[i*theTrellisTable->n+j]=tmp;
                    //std::cout<<tmp<<",";
                };

            };

        }; //end if mode==1

        TS_ASSERT_SAME_DATA(outputVector,outputVectorTest,numberOfOutBits*sizeof(unsigned int));

        //DECODER PART
        unsigned short int data2Dec[20],decData[20];

        SigProc::fec_conv_decoder  * decoder;
        decoder=new SigProc::fec_conv_decoder();
        decoder->SetTrellisTable(theTrellisTable);



        numberOfBits=numberOfOutBits;
        noOfSymbols=numberOfBits/theTrellisTable->n;


        ///Adjust the output lengh according to the encoding mode
        switch (mode) {
        case 0:
            ///The encoder starts from zero state
            numberOfOutBits=noOfSymbols*theTrellisTable->k;
            decoder->SetMode(0);
            break;
        case 1:
            ///The encoder starts and ends at the zero state
            numberOfOutBits=(noOfSymbols-theTrellisTable->K)*theTrellisTable->k;
            decoder->SetMode(1);
            break;
        };

        decoder->SetNoOfSymbols2TraceBack(noOfSymbols);

        for (unsigned int i=0; i<noOfSymbols; i++) {
            for (unsigned int j=0; j<theTrellisTable->n; j++) {
                data2Dec[j]=outputVector[i*theTrellisTable->n +j];

            }
            decoder->Symbol2Decode(data2Dec);
        }

        decoder->TraceBackTrellis();

        for (int i=0; i<numberOfOutBits/theTrellisTable->k; i++) {
            decoder->GetDecodedSymbol(decData);

            for (int j=0; j<theTrellisTable->k; j++) {
                outputVector[i*theTrellisTable->k+j]=(short int )decData[j];
            }
        }

        TS_ASSERT_SAME_DATA(outputVector,inputVector,numberOfOutBits*sizeof(unsigned int));


        delete theTrellisTable;
        delete encoder;
    };
};





#endif

