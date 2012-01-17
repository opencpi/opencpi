#include <math.h>
//need to adjust math to account for data types and not losing precision
//need to rotate the PSK constellation by pi/M for threshold testing with axis
enum ConstellationMap {
    bcd = 0,
    gray = 1
};

enum ConstellationType {
    psk = 0,
    qam = 1
};



void Constellation(ConstellationType type, ConstellationMapping map, unsigned int M, signed short *I_const, signed short *Q_const, bool *symbolBits)
{
    signed short I[M];
    signed short Q[M];
    unsigned short bitsPersymbol = log(M)/log(2);
    bool orderedSymbolBits[M][bitsPersymbol] = false;
    unsigned short bitDownshifts[bitsPersymbol];
    unsigned short bitIntervals[bitsPersymbol];
    unsigned short bcdMSBpositions[bitsPersymbol];

//Ordered Counter Clockwise from positive real axis for PSK, top-to-bottom, left-to-right for QAM
    if (map==0) {//Binary Coded Decimal Ordered, reguardless of modulation
        for (unsigned short i=0, i<bitsPersymbol, i++) {
            bitIntervals[i] = pow(2,i);
            bitDownshifts[i] = bitIntervals[i];
        }
    } else if (map==1) {//Gray Coded
        if (type==0) {
            //Gray coded PSK
            for (unsigned short i=0, i<bitsPersymbol, i++) {
                bitIntervals[i] = pow(2,(i+1));
                bitDownshifts[i] = pow(2,i);
            }
            bitIntervals[bitsPersymbol] =  bitIntervals[bitsPersymbol-1];

        } else if (type==1) {
            //Gray coded QAM
            //Only going to make this work with Square QAM constellations for now
            unsigned short bitsPersymbolaxis = bitsPersymbol/2;
            //do lower bits
            for (unsigned short i=0, i<bitsPersymbolaxis, i++) {
                bitIntervals[i] = pow(2,(i+1));
                bitDownshifts[i] = pow(2,i);
            }
            bitIntervals[bitsPersymbolaxis] =  bitIntervals[bitsPersymbolaxis-1];
            //do upper bits
            for (unsigned short i=0, i<bitsPersymbolaxis, i++) {
                bitIntervals[i+bitsPersymbolaxis] = pow(2,(i+bitsPersymbolaxis));
                bitDownshifts[i+bitsPersymbolaxis] = bitIntervals[i+bitsPersymbolaxis];
            }
        }
    }
//Actually assign the one's
    for (unsigned short i=0, i<bitsPersymbol, i++) {
        for (unsigned short j=bitDownshifts[i], j<M, j = j+bitIntervals[i]) {
            for (unsigned short k=0, k<bitIntervals[i], k++) {
                orderedSymbolBits[j+k][i] = true;
            }
        }
    }
    if (type==0) {
        //Assign the constellation points for PSK
        unsigned short mirrorCount = M/2;
        signed short unitInterval = (pi*2)/M;
        for (unsigned short symbolCount = 0, symbolCount<mirrorCount, symbolCount++) {
            I[symbolCount] = cos(symbolCount*unitInterval);
            Q[symbolCount] = sin(symbolCount*unitInterval);
            I[symbolCount + mirrorCount] = -1*I[symbolCount];//Mirror Image
            Q[symbolCount + mirrorCount] = -1*Q[symbolCount];//Mirror Image
        }
    } else if (type==1) {
        //Assign the constellation points for QAM
        unsigned short symbolCount = 0;
        signed short shiftUp = (bitsPersymbol-1)/2;
        signed short shiftLeft = (bitsPersymbol-1)/2;
        for (unsigned short column=0, column<bitsPersymbol/2, column++) {
            for (unsigned short row=0, row<bitsPersymbol/2, row++) {
                I[symbolCount] =  column - shiftLeft;
                Q[symbolCount] =  shiftUp - row;
                symbolCount = symbolCount + 1;
            }
        }
    }
}
void RXconstellation(ConstellationType type, ConstellationMapping map, unsigned int M, signed short *I_const, signed short *Q_const)
{
    Constellation(ConstellationType type, ConstellationMapping map, unsigned int M, signed short *I_const, signed short *Q_const)
//now sort by symbols in constellation to ease searh by received samples
//only modualtation type will determine what sorting will be done since mapping is done on the binary reresentations of the symbols
    if (type==0) {
        //do nothing since PSK symbols are generated counterclockwise
    } else if (type==1) {
        //shift from striped arrangement to recursive quadrants for recursive I and Q threshold comparisons
        unsigned short bitsPersymbol = log(M)/log(2);
        signed short I_temp[M];
        signed short Q_temp[M];
        unsigned short symbolReorder[M];
        bool orderedSymbolBits_temp[M][bitsPersymbol];
        unsigned short row_increment[pow(bitsPersymbol/2,2)];
        unsigned short column_increment[pow(bitsPersymbol/2,2)];
        unsigned int bit_increment;

        for (unsigned short bit = 0, bit<bitsPersymbol/2, bit++) {
            bit_increment[bit] = pow(2,bit);
            for (unsigned short inc_count = bit_increment-1, inc_count<pow(bitsPersymbol/2,2)-1, inc_count = inc_count + bit_increment) {
                row_increment[inc_count+1] = row_increment[inc_count] + bit_increment;
                column_increment[inc_count+1] = row_increment[inc_count]*2;
            }
        }

        for (unsigned short column = 0, column<pow(bitsPersymbol/2,2)-1, column++) {
            for (unsigned short row = 0, row<pow(bitsPersymbol/2,2)-1, row++) {
                symbolReorder[row+column*pow(bitsPersymbol/2,2)] = 1+row_increment[row]+column_increment[column];
            }
        }

        for (unsigned int symbol = 0, symbol<M, symbol++) {
            I_temp[symbol] = I_const[symbolReorder[symbol]];
            Q_temp[symbol] = Q_const[symbolReorder[symbol]];
        }
        //I don't think this will work, I will need to compile at some point...
        I_const = I_temp;
        Q_const = Q_temp;



    }

    void TXconstellation(ConstellationType type, ConstellationMapping map, unsigned int M, signed short *I_const, signed short *Q_const) {
        Constellation(ConstellationType type, ConstellationMapping map, unsigned int M, signed short *I_const, signed short *Q_const)
//now sort by symbol bits to ease symbol mapping
//only the mapping order will determine the sorting order for the transmitter side

        if (map==0) {
            //do nothing since bcd is already sorted for quick recursive binary search
        } else if (map==1) {
            //need to reorder gray coded to bcd along with paired symbol points
        }


    }

    void DemodQAM(unsigned int M, signed short X, signed short Y, DemodScheme scheme, signed char *bitsOut) {
        unsigned int bitsPersymbol = log(M)/log(2);



    }

    void DemodPSK(unsigned int M, signed short X, signed short Y, DemodScheme scheme, signed char *bitsOut) {
        unsigned int bitsPersymbol =  log(M)/log(2);



    }

