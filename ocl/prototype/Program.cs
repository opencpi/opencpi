using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace OclPrototype2
{
    class Program
    {
        const int BLOCK_SIZE = 16;
        const int WA = (5 * BLOCK_SIZE); // Matrix A width
        const int HA = (10 * BLOCK_SIZE); // Matrix A height
        const int WB = (5 * BLOCK_SIZE); // Matrix B width
        const int HB = WA;  // Matrix B height
        const int WC = WB;  // Matrix C width 
        const int HC = HA;  // Matrix C height

        static void Main(string[] args)
        {
            // Get a list of all available devices
            List<OCLDeviceDescription> list = OCLContainer.getAvailableDevices();

            // Just get the first device on the AMD platform.
            // If this isn't found, just use the first device on the first platform
            int deviceIndex = 0;
            foreach (OCLDeviceDescription descrip in list)
            {
                if (descrip.platformName == "ATI Stream") break;

                deviceIndex++;
            }

            // If we went through the list without finding anything
            // then just pick the first one
            if (deviceIndex >= list.Count)
                deviceIndex = 0;

            OCLContainer container = new OCLContainer(deviceIndex);

            OCLWorker worker = new OCLWorker(Directory.GetCurrentDirectory() + @"\..\..\components\matrixMul.ocl", "matrixMul",
                container);

            // Set the worker properties
            worker.setProperty("WA", WA);       // Width of A matrix
            worker.setProperty("WB", WB);       // Width of B matrix

            // Create external ports to interface to our component
            OCLPort matrixInput = worker.portDictionary["in"];
            OCLPort result = worker.portDictionary["out"];

            // Start the worker
            worker.start();

            uint size_A = WA * HA;
            uint size_B = WB * HB;
            uint size_C = WC * HC;

            OCLBuffer[] matrixInputBuffer = new OCLBuffer[OCLPort.NUM_BUFFERS];

            // Allocate space for both A and B matrices
            // For now, since this is a prototype, I will assume
            // that the buffer is big enough to hold all the data
            float[][] h_AB_data = new float[OCLPort.NUM_BUFFERS][];
            float[][] reference = new float[OCLPort.NUM_BUFFERS][];

            uint buffIndex = 0;
            for (buffIndex = 0; buffIndex < OCLPort.NUM_BUFFERS; buffIndex++)
            {
                matrixInputBuffer[buffIndex] = matrixInput.getBuffer();
                h_AB_data[buffIndex] = matrixInputBuffer[buffIndex].getBufferStorage();

                // Fill the matrices with random data
                Random rand = new Random();

                for (int i = 0; i < size_A; i++)
                {
                    h_AB_data[buffIndex][i] = (float)(rand.NextDouble() * 100);
                }
                for (int i = 0; i < size_B; i++)
                {
                    h_AB_data[buffIndex][size_A + i] = (float)(rand.NextDouble() * 100);
                }

                reference[buffIndex] = new float[size_C];

                // Compute the matrix multiply here for comparison 
                computeGold(reference[buffIndex], h_AB_data[buffIndex], HA, WA, WB);
            }

            for (buffIndex = 0; buffIndex < OCLPort.NUM_BUFFERS; buffIndex++)
            {
                // Place the A,B matrix data into the component input port
                matrixInputBuffer[buffIndex].put(size_A + size_B);
            }

            buffIndex = 0;

            // This is designed to run forever.
            // It will only stop (and throw an exception) if the output is not what was expected.
            while (true)
            {
                // Get the result (matrix C), which is the multiplication of A and B
                OCLBuffer resultBuffer = result.getBuffer();
                float[] h_C = resultBuffer.getBufferStorage();

                // Compare the arrays
                bool arraysAreEqual = compareTheArrays(h_C, reference[buffIndex]);

                if (!arraysAreEqual)
                {
                    throw new OCLException("Arrays did not compare");
                }

                // Just to make sure results are getting copied to the host each time.
                h_C[0] = 0;

                resultBuffer.put();

                matrixInputBuffer[0] = matrixInput.getBuffer();
//                h_AB_data[0] = matrixInputBuffer[buffIndex].getBufferStorage();

                // Normally would place something in the buffer here, but we'll skip that step
                // since the same data should be there already.

                matrixInputBuffer[0].put();

                buffIndex = (buffIndex + 1) % OCLPort.NUM_BUFFERS;

            }

#if BLAH
            // Allocate space for both A and B matrices
            // For now, since this is a prototype, I will assume
            // that the buffer is big enough to hold all the data
            OCLBuffer matrixInputBuffer = matrixInput.getBuffer();
            float[] h_AB_data = matrixInputBuffer.getBufferStorage();

            // Fill the matrices with random data
            Random rand = new Random();

            for (int i = 0; i < size_A; i++)
            {
                h_AB_data[i] = (float)(rand.NextDouble() * 100);
            }
            for (int i = 0; i < size_B; i++)
            {
                h_AB_data[size_A + i] = (float)(rand.NextDouble() * 100);
            }

            // Place the A,B matrix data into the component input port
            // For now, since this is a prototype, we make this simple
            // by assuming that the put operates on the last buffer
            // that was obtained by the "getBuffer()" call.
            matrixInputBuffer.put();

            // Get the result (matrix C), which is the multiplication of A and B
            OCLBuffer resultBuffer = result.getBuffer();
            float[] h_C = resultBuffer.getBufferStorage();

            uint size_C = WC * HC;

            float[] reference = new float[size_C];
 
            // Compute the matrix multiple here for comparison 
            computeGold(reference, h_AB_data, HA, WA, WB);

            // Compare the arrays
            bool arraysAreEqual = compareTheArrays(h_C, reference);

            if (arraysAreEqual)
                Console.WriteLine("PASSED");
            else
                Console.WriteLine("FAILED");
#endif
        }
        
        static bool compareTheArrays(float[] arr1, float[] arr2)
        {
            // Check that the arrays are equal with some allowance for differences
            // in floating point computation
            for (int i = 0; i < arr2.Length; i++)
            {
                float diff = Math.Abs(arr1[i] - arr2[i]);

                if (diff > 0.125)
                    return false;
            }

            return true;
        }

        
        static void computeGold(float[] C, float[] AB, uint hA, uint wA, uint wB)
        {
            // Perform matrix multiply
            // Matrices A and B are together in one array, with A being first

            uint size_A = hA * wA;

            for (uint i = 0; i < hA; ++i)
                for (uint j = 0; j < wB; ++j)
                {
                    double sum = 0;
                    for (uint k = 0; k < wA; ++k)
                    {
                        double a = AB[i * wA + k];
                        double b = AB[k * wB + j + size_A];
                        sum += a * b;
                    }
                    C[i * wB + j] = (float)sum;
                }
        }
    }
}
