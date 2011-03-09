using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Cloo;
using Cloo.Bindings;
using System.Runtime.InteropServices;
using System.Collections.ObjectModel;
using System.Threading;

namespace OclPrototype2
{
    class OclBufferAvailableEvent
    {
        public ICollection<ComputeEventBase> theEvents;
        public OCLBuffer theBuffer;
        public OCLKernelInstance theKernelInstance;
        public OclBufferAvailableEvent(OCLBuffer buffer_, ICollection<ComputeEventBase> theEvents_, OCLKernelInstance kernelInstance_)
        {
            theBuffer = buffer_;
            theEvents = theEvents_;
            theKernelInstance = kernelInstance_;
        }
    }

    class OCLPort
    {
        public enum Type { OUTPUT, INPUT };
        private string m_name;
        public Type m_type;
        public OCLContainer m_container;
        public OCLWorker m_worker;
        public ComputeBuffer<float>[] m_computeBufferArray;
        // This is an array of arrays (known as a jagged array)
        public float[][] m_bufferArray;
#if MAP_UNMAP_APPROACH
#else
        private GCHandle[] m_bufferArrayHandle;
#endif
        private Queue<OCLBuffer> m_buffersAvailable;
        private Queue<OclBufferAvailableEvent> m_eventQueueTiedToBufferAvailability;
        public OCLBuffer m_currentBufferForKernelInstance;
        public uint m_operation_or_exception_ordinal;

        // The number and length of the buffers should probably be put in metadata somewhere,
        // but since this is a prototype, we will make it simple
        public const int NUM_BUFFERS = 2;
        public const int BUFFER_LEN = 19200;

        public OCLPort(string name_, Type type_, OCLWorker worker_)
        {
            m_name = name_;
            m_type = type_;
            m_worker = worker_;
            m_container = m_worker.m_container;
            m_buffersAvailable = new Queue<OCLBuffer>();
            m_eventQueueTiedToBufferAvailability = new Queue<OclBufferAvailableEvent>();
            m_currentBufferForKernelInstance = null;

            ComputeMemoryFlags flags;
            // These flags are from the perspective of the device
            // So, if an input port into the device, the device can only read it
            // If an output port out of the device, the device can only write to it
            if (m_type == Type.INPUT)
                flags = ComputeMemoryFlags.ReadOnly;
            else
                flags = ComputeMemoryFlags.WriteOnly;

            // Create one big OpenCL buffer, which will contain a number of smaller buffers that we will
            // keep track of.

            m_computeBufferArray = new ComputeBuffer<float>[NUM_BUFFERS];
            m_bufferArray = new float[NUM_BUFFERS][];
#if MAP_UNMAP_APPROACH
#else
            m_bufferArrayHandle = new GCHandle[NUM_BUFFERS];
#endif

            for (uint i = 0; i < NUM_BUFFERS; i++)
            {
                m_computeBufferArray[i] = new ComputeBuffer<float>(m_container.m_context, flags, BUFFER_LEN);

                // For C#, we'll also need to keep a managed array of buffers
                m_bufferArray[i] = new float[BUFFER_LEN];
#if MAP_UNMAP_APPROACH
#else
                m_bufferArrayHandle[i] = GCHandle.Alloc(m_bufferArray[i], GCHandleType.Pinned);
#endif

                OCLBuffer buffer = new OCLBuffer(i, this);
                m_buffersAvailable.Enqueue(buffer);
            }

        }

        public OCLBuffer getBuffer()
        {
            OCLBuffer buffer = null;

            // For input ports, we will dequeue a buffer from the buffers available queue
            // until that queue is emptied.  Thereafter, we will go to the queue which
            // matches an event with the next buffer.  When an event completes, that
            // buffer then becomes available.

            if (m_type == Type.INPUT)
            {
                if (m_buffersAvailable.Count != 0)
                {
                    buffer = m_buffersAvailable.Dequeue();
                    buffer.allocateByUser();
                    return buffer;
                }
            }

            // We always go here for output buffers
            // We go here for input buffers once the first group of buffers has been used

            OclBufferAvailableEvent theEvent = m_eventQueueTiedToBufferAvailability.Dequeue();
            if (theEvent == null)
            {
                // You might get here if the user tries to get a buffer on an output port 
                // before any input_buffer->put() operations have started the kernel execution process.

                // This should not ever happen for input ports unless there is a bug in the code.
                throw new OCLException("Probably tried to do output_port->getBuffer() before kernel execution");
            }
            ICollection<ComputeEventBase> theEventsToWaitOn = theEvent.theEvents;
            ComputeEventList.Wait(theEventsToWaitOn);
            buffer = theEvent.theBuffer;
            // Dispose of the kernel associated with this buffer (if not already disposed)
            theEvent.theKernelInstance.disposeOfKernel();

            buffer.allocateByUser();

            return buffer;
        }

        public OCLBuffer allocateOutputBuffForSystem()
        {
            OCLBuffer buffer = m_buffersAvailable.Dequeue();
            setBufferForNextKernelExecution(buffer);
            buffer.allocateBySystem();

            return buffer;
        }

        public void setBufferForNextKernelExecution(OCLBuffer buffer_)
        {
            m_currentBufferForKernelInstance = buffer_;
        }
        public void write(ICollection<ComputeEventBase> events_, OCLBuffer buffer_)
        {
            if (m_type == Type.INPUT)
            {
#if MAP_UNMAP_APPROACH
                // Copy the data from the managed array to the mapped area before executing the unmap
                // This is a C# thing only
                Marshal.Copy(m_bufferArray[buffer_.m_index], 0, buffer_.m_mappedPtr, BUFFER_LEN);

                m_container.m_commandQueue.Unmap(m_computeBufferArray[buffer_.m_index], ref buffer_.m_mappedPtr,
                    events_);
#else
                // Non blocking write
                m_container.m_commandQueue.Write<float>(m_computeBufferArray[buffer_.m_index], false, 0, buffer_.m_currentBufferInfo[0].length, 
                    m_bufferArrayHandle[buffer_.m_index].AddrOfPinnedObject(), events_);
#endif
            }
            else
                throw new OCLException("You shouldn't be writing to an output port");
        }

        public void read(ICollection<ComputeEventBase> events_, OCLBuffer buffer_)
        {
            if (m_type == Type.OUTPUT)
#if MAP_UNMAP_APPROACH
                buffer_.m_mappedPtr = m_container.m_commandQueue.Map<float>(m_computeBufferArray[buffer_.m_index],
                    false, ComputeMemoryMappingFlags.Read, 0, BUFFER_LEN, events_);
#else
            // Non blocking read
            // All previous events must complete before the read is performed
            m_container.m_commandQueue.Read<float>(m_computeBufferArray[buffer_.m_index], false, 0, BUFFER_LEN, 
                m_bufferArrayHandle[buffer_.m_index].AddrOfPinnedObject(), events_);
#endif
            else
                throw new OCLException("You shouldn't be reading from an input port");
        }

        public void makeBufferAvailableOnEventsCompletion(OCLBuffer buffer_, ICollection<ComputeEventBase> events_, OCLKernelInstance kernelInstance_)
        {
            OclBufferAvailableEvent theNewEvent = new OclBufferAvailableEvent(buffer_, events_, kernelInstance_);
            m_eventQueueTiedToBufferAvailability.Enqueue(theNewEvent);
        }

        public void makeBufferAvailable(OCLBuffer buffer_)
        {
            // You would only call this for an output buffer

            m_buffersAvailable.Enqueue(buffer_);
        }

        public void setOperationOrExceptionOrdinal(uint value_)
        {
            m_operation_or_exception_ordinal = value_;
        }
    }
}