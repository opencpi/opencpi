/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
This worker only exists to test the unit test generation. It needs to be able to
operate and move data to exercise the code
 */

#include "unit_tester-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Unit_testerWorkerTypes;

class Unit_testerWorker : public Unit_testerWorkerBase {
    size_t myBufferIndex1, myBufferIndex2, myBufferIndex3;
    size_t myBufferSize1, myBufferSize2, myBufferSize3;
    bool lastBufferComplete;

    RCCResult start ()
    {
        myBufferIndex1 = 0;
        myBufferSize1 = 0;

        myBufferIndex2 = 0;
        myBufferSize2 = 0;

        myBufferIndex3 = 0;
        myBufferSize3 = 0;

        lastBufferComplete = true;

        return RCC_OK;
    }

    RCCResult stop ()
    {
        return RCC_OK;
    }

    RCCResult run(bool /*timedout*/) {
        out1.setOpCode(in1.opCode());        // Set the metadata for the output message
        out2.setOpCode(in2.opCode());
        out3.setOpCode(in3.opCode());

        // Allow ZLMs to pass through unmolested.
        if (in1.length() < 1) {
            out1.setLength(0);
            out2.setLength(0);
            out3.setLength(0);
            return RCC_ADVANCE;
        }

        //treating the buffers as arrays of unsigned bytes
         const uint8_t  *inData1  = static_cast<const uint8_t*>(in1.data());
         uint8_t *outData1 =  static_cast<uint8_t*>(out1.data());

         const uint8_t  *inData2  = static_cast<const uint8_t*>(in2.data());
         uint8_t *outData2 =  static_cast<uint8_t*>(out2.data());

         const uint8_t  *inData3  = static_cast<const uint8_t*>(in3.data());
         uint8_t *outData3 =  static_cast<uint8_t*>(out3.data());

        // if the output buffer is larger than the input, then the data can safely
        // be copied from input to output
        if (in1.length() <= out1.length()) {
            memcpy(outData1, inData1, in1.length());
            out1.setLength( in1.length() );
            memcpy(outData2, inData2, in2.length());
            out2.setLength( in2.length() );
            memcpy(outData3, inData3, in3.length());
            out3.setLength( in3.length() );
            lastBufferComplete = true;
            return RCC_ADVANCE;
        } else {
            // if the input buffer is larger than output, the data needs to be broken up
            // and sent in chunks.
            if ( lastBufferComplete ) {
                myBufferSize1 = in1.length();
                myBufferIndex1 = 0;
                myBufferSize2 = in2.length();
                myBufferIndex2 = 0;
                myBufferSize3 = in3.length();
                lastBufferComplete = false;
                myBufferIndex3 = 0;
            }

            size_t copyAmount1 = myBufferSize1 - myBufferIndex1;
            size_t copyAmount2 = myBufferSize2 - myBufferIndex2;
            size_t copyAmount3 = myBufferSize3 - myBufferIndex3;
            // If this the end of the input buffer, only copy the remaining data
            // Else, copy the maximum amount allowed by the size of the output buffer
            if (copyAmount1 <= out1.length()){
                memcpy(outData1, &inData1[myBufferIndex1], copyAmount1);
                out1.setLength( copyAmount1 );
                memcpy(outData2, &inData2[myBufferIndex2], copyAmount2);
                out2.setLength( copyAmount2 );
                memcpy(outData3, &inData3[myBufferIndex3], copyAmount3);
                out3.setLength( copyAmount3 );
                lastBufferComplete = true;
                return RCC_ADVANCE;
            } else{
                memcpy(outData1, &inData1[myBufferIndex1], out1.length());
                myBufferIndex1 += out1.length();
                out1.setLength(out1.length());
                out1.advance();
                memcpy(outData2, &inData2[myBufferIndex2], out2.length());
                myBufferIndex2 += out2.length();
                out2.setLength(out2.length());
                out2.advance();
                memcpy(outData3, &inData3[myBufferIndex3], out3.length());
                myBufferIndex3 += out3.length();
                out3.setLength(out3.length());
                out3.advance();
                return RCC_OK;
            }
        }
    }
};

UNIT_TESTER_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
UNIT_TESTER_END_INFO
