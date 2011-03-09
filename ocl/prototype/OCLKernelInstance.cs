using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Cloo;
using System.IO;
using System.Xml.Linq;
using System.Collections.ObjectModel;
using System.Runtime.InteropServices;

namespace OclPrototype2
{

    class OCLKernelInstance
    {
        // This is cheating for now
        // Using constants from the application
        // Doing this temporarily for testing
        const uint BLOCK_SIZE = 16;
        const uint WA = (5 * BLOCK_SIZE); // Matrix A width
        const uint HA = (10 * BLOCK_SIZE); // Matrix A height
        const uint WB = (5 * BLOCK_SIZE); // Matrix B width
        const uint HB = WA;  // Matrix B height
        const uint WC = WB;  // Matrix C width 
        const uint HC = HA;  // Matrix C height

        private ComputeKernel m_kernel;
        public ICollection<ComputeEventBase> m_kernelExecutionEventDependencies;
        private Queue<OCLBuffer> m_buffersTiedToKernelExecution;

        public OCLKernelInstance(ComputeProgram program_)
        {
            m_kernel = program_.CreateKernel("matrixMulRun");
            m_kernelExecutionEventDependencies = new Collection<ComputeEventBase>();
            m_buffersTiedToKernelExecution = new Queue<OCLBuffer>();
        }

        public void addToKernelEventDependencies(ICollection<ComputeEventBase> precedingEvents_)
        {
            // Add to the list of events that must complete before executing the kernel
            foreach (ComputeEventBase thePrecedingEvent in precedingEvents_)
                m_kernelExecutionEventDependencies.Add(thePrecedingEvent);
        }

        public void tieBufferToKernelExecutionEventPriorToKernelExecution(OCLBuffer buffer_)
        {
            // Associate this buffer with the kernel instance prior to its execution
            m_buffersTiedToKernelExecution.Enqueue(buffer_);
        }

        public void disposeOfKernel()
        {
            if (m_kernel != null)
                m_kernel.Dispose();

            m_kernel = null;
        }

        public void execute(Dictionary<string, OCLPort> portDictionary_, OCLWorker.KernelControlStruct[] kernelControl_,
            ComputeBuffer<OCLWorker.PropertyStruct> computeBufferProperties_, OCLContainer container_)
        {
            int index = 0;
            // Setup all of the kernel arguments

            // The port data goes first
            foreach (KeyValuePair<string, OCLPort> pair in portDictionary_)
            {
                OCLPort port = pair.Value;

                m_kernel.SetMemoryArgument(index++, port.m_computeBufferArray[port.m_currentBufferForKernelInstance.m_index]);

                // The current buffer info structure array -- to be passed to the kernel and later passed back from the kernel
                m_kernel.SetMemoryArgument(index++, port.m_currentBufferForKernelInstance.m_computeBufferCurrentBufferInfo);

            }

            // Local memory allocation
            // Should do it as shown in the loop below, but fixed sized buffers in a struct
            // are considered unsafe so do it this way
            m_kernel.SetLocalArgument(index++, kernelControl_[0].lMemSize0);
            m_kernel.SetLocalArgument(index++, kernelControl_[0].lMemSize1);
            //            for (int i = 0; i < OCLWorker.MATRIXMUL_N_LOCAL_MEMORIES; i++)
//                m_kernel.SetLocalArgument(index++, kernelControl_[0].lMemSize[i]);

            // Properties struct
            m_kernel.SetMemoryArgument(index++, computeBufferProperties_);

            long[] globalWorkOffset = { 0, 0 };
            long[] globalWorkSize = { (long)WC, (long)HC };
            long[] localWorkSize = { (long)BLOCK_SIZE, (long)BLOCK_SIZE };

            // Execute the kernel
            container_.m_commandQueue.Execute(m_kernel, globalWorkOffset, globalWorkSize, localWorkSize, m_kernelExecutionEventDependencies);

            // Now that m_kernelExecutionEventDependencies also contains the kernel execution event itself, lets
            // setup all the buffers that can be used again once the kernel terminates

            foreach (OCLBuffer buffer in m_buffersTiedToKernelExecution)
                buffer.m_port.makeBufferAvailableOnEventsCompletion(buffer, m_kernelExecutionEventDependencies, this);


        }

    }
}