/****************************************************************************
Copyright 2005,2006 Virginia Polytechnic Institute and State University

This file is part of the OSSIE Signal Processing Library.

OSSIE Signal Processing Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

OSSIE Signal Processing Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with OSSIE Signal Processing Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA


****************************************************************************/

#include "SigProc.h"

namespace SigProc
{

//----------------------------Trellis Entry-------------------------------------
TrellisEntry::TrellisEntry()
{
    previousState=0;
    symbolNo=0;
    distance=-1;
}

TrellisEntry::~TrellisEntry()
{


}

//----------------------------trellisTable--------------------------------------

trellisTable::trellisTable(
    unsigned int * generatorPolynomials,
    unsigned short int k_in,
    unsigned short int n_in,
    unsigned short K_in)
{
//Build trellis table
    trellisTable::k=k_in;
    trellisTable::n=n_in;
    trellisTable::K=K_in;

//std::cout<<"Constructor, generatorPolynomials:";
// for (int i=0;i<n;i++){

//   std::cout<<generatorPolynomials[i]<<" ";
//}
//std::cout<<std::endl;

//std::cout<<"Input bits:"<<k<<" Contraint Length:"<<K<<std::endl;
    trellisTable::nextState=NULL;
    trellisTable::output=NULL;
    trellisTable::GenerateTrellisTable(generatorPolynomials);
}

trellisTable::~trellisTable()
{
//Deallocate memory
    if (trellisTable::nextState!=NULL) {
        for (unsigned int i = 0; i < trellisTable::numberOfTrellisStates; i++) {
            delete []trellisTable::nextState[i];
        }
        delete []trellisTable::nextState;
    }

    if (trellisTable::output!=NULL) {
        for (unsigned int i = 0; i < trellisTable::numberOfTrellisStates; i++) {
            delete []trellisTable::output[i];
        }
        delete []trellisTable::output;
    }
}


void trellisTable::GenerateTrellisTable(unsigned int * generatorPolynomialsOct)
{

    numberOfTrellisStates=int(pow(2,(k*(K-1))));
    numberOfInputStates=int(pow(2,k));

//std::cout<<"NumberOfTrellisStates="<<numberOfTrellisStates<<std::endl;
//std::cout<<"numberOfInputStates="<<numberOfInputStates<<std::endl;

    nextState = new unsigned int *[numberOfTrellisStates];

    for (unsigned int i = 0; i < numberOfTrellisStates; i++) {
        nextState[i] = new unsigned int[numberOfInputStates];
    }


    output = new unsigned int *[numberOfTrellisStates];
    for (unsigned int i = 0; i < numberOfTrellisStates; i++) {
        output[i] = new unsigned int[numberOfInputStates];
    }

///Convert Octal Polynomials to Decimal
    unsigned int generatorPolynomials[n];
    for (unsigned short int bitNo=0; bitNo<n; bitNo++) {
        generatorPolynomials[bitNo]=Oct2Dec(generatorPolynomialsOct[bitNo]);

    }

    unsigned int ENCregister=0, tempOut=0;

    for (unsigned int tstate=0; tstate<numberOfTrellisStates; tstate++) {
        //std::cout<<tstate<<":";
        for (unsigned short int inputIdx=0; inputIdx<numberOfInputStates; inputIdx++) {

            ENCregister=inputIdx*numberOfTrellisStates+tstate;
            nextState[tstate][inputIdx]=(unsigned int)floor(ENCregister/pow(2,k));
            tempOut=0;

            for (unsigned short int bitNo=0; bitNo<n; bitNo++) {
                // std::cout<<"\nBitNo:"<<bitNo<<" ENCregister:"<<ENCregister<<" g:"<<generatorPolynomials[bitNo]<<" &:"<<(ENCregister&generatorPolynomials[bitNo])
                //	<<"power:"<<(unsigned short int)pow(2,n-bitNo-1)<<"tempOut:"<<tempOut<<"\n";
                // std::cout<<"Mod2add:"<<Modulo2BitWiseAdd((ENCregister&generatorPolynomials[bitNo])*(unsigned short int)pow(2,n-bitNo-1))<<"\n";

                tempOut+=Modulo2BitWiseAdd(ENCregister&generatorPolynomials[bitNo])* (unsigned short int)pow(2,n-bitNo-1);
            }
            output[tstate][inputIdx]=tempOut;
            //std::cout<<output[tstate][inputIdx]<<" ("<<nextState[tstate][inputIdx]<<") ";


        }
        //std::cout<<std::endl;
    }
}

unsigned short int trellisTable::Modulo2BitWiseAdd(unsigned short int inputNumber)
{
//std::cout<<"IN:"<<inputNumber;
    unsigned int tempOut=0;
    tempOut=inputNumber % 2;
    inputNumber>>=1;

    while (inputNumber>0) {
        tempOut^=inputNumber % 2;
        inputNumber>>=1;
    }
// std::cout<<" Out:"<<tempOut<<" ";
    return tempOut;
}

///Converts an Octal Number to a Decimal number
unsigned int trellisTable::Oct2Dec(unsigned int octNumber)
{
    unsigned int r,decNumber=0,i;

    for (i=0; octNumber!=0; i++) {
        r=octNumber%10;
        decNumber=decNumber+r*(unsigned int)pow(8,i);
        octNumber=octNumber/10;
    }

    return decNumber;
}


//-------------------------------fec_conv--------------------------------------

void fec_conv::SetTrellisTable(trellisTable *theTrellisTableIn)
{
    theTrellisTable=theTrellisTableIn;
}

void fec_conv::Dec2Bin(unsigned int decNumber,unsigned short int * outputData,unsigned short int numberOfBits)
{
    unsigned short int i;
    for (i=numberOfBits; i>0; i--) {
        outputData[i-1]=decNumber % 2;

        if (decNumber>0) decNumber>>=1;
    }
}

fec_conv_encoder::fec_conv_encoder()
{
    currentState=0;
}

fec_conv_encoder::~fec_conv_encoder()
{

}

void fec_conv_encoder::ResetState()
{
    currentState=0;
}

unsigned int fec_conv_encoder::GetState()
{
    return currentState;
}

void fec_conv_encoder::Encode(unsigned short int * inputData,unsigned short int * outputData)
{
    unsigned int inbits=0,tmp=0,outbits=0;

    for (unsigned short int i=0; i<theTrellisTable->k; i++)    {
        tmp=inputData[i];
        //std::cout<<"inputData:"<<tmp<<"\n";
        tmp<<=theTrellisTable->k-i-1;
        //std::cout<<"temp"<<tmp<<"\n";
        inbits+=tmp;
    }

    if (inbits>(unsigned int)(theTrellisTable->numberOfInputStates-1)) {
        std::cout<<"ERROR:fec_conv_encoder::Encode inbits>numberOfInputStates-1";
        std::cout<<"inbits:"<<inbits<<" numberOfInputStates:"<<theTrellisTable->numberOfInputStates;
        throw 0;
    };

//std::cout<<"state:"<<currentState<<" inbits:"<<inbits<<" outbits:"<<outbits<<"\n";
    outbits=theTrellisTable->output[currentState][inbits];

    currentState=theTrellisTable->nextState[currentState][inbits];
    Dec2Bin(outbits,outputData,theTrellisTable->n);

}

//------------------------------Decoder-----------------------------------------
fec_conv_decoder::fec_conv_decoder()
{
    tracedBackSymbols=NULL;
    theTrellis=NULL;
    decodedSymbolIndex=0;
    currentTrellisIndex=0;
    mode=0;
}

fec_conv_decoder::~fec_conv_decoder()
{
    if (tracedBackSymbols!=NULL) delete  []tracedBackSymbols;

    if (theTrellis!=NULL) {
        for (unsigned int i = 0; i < theTrellisTable->numberOfTrellisStates; i++) {
            delete []theTrellis[i];
        };

        delete []theTrellis;
    }

};
void fec_conv_decoder::SetMode(unsigned short int mode)
{
    fec_conv_decoder::mode=mode;
};

void fec_conv_decoder::Reset()
{
    decodedSymbolIndex=0;
    currentTrellisIndex=0;
    for (unsigned int i=0; i<theTrellisTable->numberOfTrellisStates; i++)
        for (unsigned int j=0; j<=noOfSymbols2TraceBack; j++) {
            theTrellis[i][j].previousState=0;
            theTrellis[i][j].symbolNo=0;
            theTrellis[i][j].distance=(signed int)-1;
        }
    theTrellis[0][0].distance=0;
};

void fec_conv_decoder::SetNoOfSymbols2TraceBack(unsigned int traceBackLength)
{
//std::cout<<theTrellis;

    if (fec_conv_decoder::noOfSymbols2TraceBack!=traceBackLength) {
        fec_conv_decoder::noOfSymbols2TraceBack=traceBackLength;
        if (theTrellis!=NULL) {
            for (unsigned int i = 0; i < theTrellisTable->numberOfTrellisStates; i++) {
                delete []theTrellis[i];
            };
            delete []theTrellis;
            theTrellis=NULL;
        }
        //std::cout<<theTrellis;
        //int dummy;
        //std::cin>>dummy;



        theTrellis= new TrellisEntry*[theTrellisTable->numberOfTrellisStates];
        for (unsigned int i = 0; i < theTrellisTable->numberOfTrellisStates; i++) {
            theTrellis[i] =new TrellisEntry[noOfSymbols2TraceBack+1];
        };

        if (tracedBackSymbols!=NULL) {
            delete []tracedBackSymbols;
            tracedBackSymbols=NULL;
        };
        tracedBackSymbols = new unsigned int [noOfSymbols2TraceBack];

    }
    /*
        currentTrellisIndex=0;
        for (unsigned int i=0;i<theTrellisTable->numberOfTrellisStates;i++)
            for (unsigned int j=0;j<=noOfSymbols2TraceBack;j++){
                theTrellis[i][j].previousState=0;
                theTrellis[i][j].symbolNo=0;
                theTrellis[i][j].distance=(signed int)-1;
            }
        theTrellis[0][0].distance=0;
    */
    fec_conv_decoder::Reset();
}

void fec_conv_decoder::Symbol2Decode(unsigned short int * inputData)
{

    unsigned int inbits=0,tmp=0,newState=0;
    unsigned short int symbol;
    signed int distance;

// inbits=0;
    for (unsigned short int i=0; i<theTrellisTable->n; i++) {
        tmp=inputData[i];
        //std::cout<<"inputData:"<<tmp<<"\n";
        tmp<<=theTrellisTable->n-i-1;
        //std::cout<<"temp"<<tmp<<"\n";
        inbits+=tmp;
    }


//std::cout<<"inbits:"<<inbits<<"\n";
    currentTrellisIndex++;

    if (currentTrellisIndex>noOfSymbols2TraceBack) {
        std::cout<<"ERROR:fec_conv_decoder::Symbol2Decode currentTrellisIndex>Traceback length";
        throw 0;
    };

    if (inbits>(unsigned int)(pow(2.0,(float)theTrellisTable->n)-1)) {
        std::cout<<"ERROR:fec_conv_decoder::Symbol2Decode inbits>2^n-1\n";
        std::cout<<"inbits:"<<inbits<<"   2^n-1: "<<(unsigned int)pow(2.0,(float)theTrellisTable->n)-1
                 <<"n="<<theTrellisTable->n<<"\n";
        throw 0;
    };

//std::cout<<"currentTrellisIndex:"<<currentTrellisIndex<<"\n";
    for (unsigned int tstate=0; tstate<theTrellisTable->numberOfTrellisStates; tstate++) {
        if (theTrellis[tstate][currentTrellisIndex-1].distance!=-1) {
            for (unsigned short int inputBits=0; inputBits<theTrellisTable->numberOfInputStates; inputBits++) {
                newState=theTrellisTable->nextState[tstate][inputBits];
                symbol=theTrellisTable->output[tstate][inputBits];

                distance=CalculateDistance(inbits,symbol)+theTrellis[tstate][currentTrellisIndex-1].distance; //Have to calculate;
                if ((theTrellis[newState][currentTrellisIndex].distance==-1)||
                        ((distance<theTrellis[newState][currentTrellisIndex].distance))||
                        (theTrellis[newState][currentTrellisIndex].distance<0)) {
                    theTrellis[newState][currentTrellisIndex].distance=distance;
                    theTrellis[newState][currentTrellisIndex].previousState=tstate;
                    theTrellis[newState][currentTrellisIndex].symbolNo=inputBits;
                };
            };
        } else {
        };

    }

    /*
        for (unsigned int tstate=0;tstate<numberOfTrellisStates;tstate++)
        {
                    std::cout<<"["<<tstate<<"]["<<currentTrellisIndex<<"].previousState:"
                       <<theTrellis[tstate][currentTrellisIndex].previousState<<".distance:"
                        <<theTrellis[tstate][currentTrellisIndex].distance<<".symbol:"<<
                        theTrellis[tstate][currentTrellisIndex].symbolNo<<"\n";
        };
    */
};

signed int fec_conv_decoder::CalculateDistance(unsigned short int inBits, unsigned short int symbol)
{
    unsigned short int tmp;
    signed int distance=0;
    tmp=inBits^symbol;

    while (tmp>0) {
        distance+=(signed int)(tmp % 2);
        tmp>>=1;
    };
    return distance;

}

void fec_conv_decoder::TraceBackTrellis()
{
//std::cout<<"cTi:"<<currentTrellisIndex<<"; cS:"<<currentState<<";";
    unsigned int currentState;
    signed int minDistance=9999;
    unsigned int minDistState=0;

    if (currentTrellisIndex==noOfSymbols2TraceBack) {
        if (mode==1) {
            currentState=0;
        } else {
            for (unsigned int tstate=0; tstate<theTrellisTable->numberOfTrellisStates; tstate++)
                if ((theTrellis[tstate][currentTrellisIndex].distance!=-1)
                        &&(theTrellis[tstate][currentTrellisIndex].distance<minDistance)) {
                    minDistance=theTrellis[tstate][currentTrellisIndex].distance;
                    minDistState=tstate;
                };

            currentState=minDistState;
        }
        // std::cout<<"\nMinDistance:"<<minDistance<<" minDistState:"<<minDistState<<"\n";


        unsigned short int symbol;
        while (currentTrellisIndex>0) {
            symbol=theTrellis[currentState][currentTrellisIndex].symbolNo;
            currentState=theTrellis[currentState][currentTrellisIndex].previousState;
            currentTrellisIndex--;
            //Dec2Bin(symbol,outputData,k);
            tracedBackSymbols[currentTrellisIndex]=symbol;
            // std::cout<<"\nsymbol:"<<symbol<<"outputData"<<outputData[0]<<"\n";
        }

        decodedSymbolIndex=0;
    }
}

void fec_conv_decoder::GetDecodedSymbol(unsigned short int * outputData)
{
    if (currentTrellisIndex==0) {

        if (decodedSymbolIndex<noOfSymbols2TraceBack) {
            Dec2Bin(tracedBackSymbols[decodedSymbolIndex],
                    outputData,
                    theTrellisTable->k);
        } else {
            std::cout<<"ERROR: fec_conv_decoder::GetDecodedSymbol attempt to get more symbols of what is available\n";
            throw 0;

        }

        decodedSymbolIndex++;

    } else {
        std::cout<<"ERROR: fec_conv_decoder::GetDecodedSymbol trellis wasnt traced back properly\n";
        throw 0;
    }
}



} // namespace SigProc

