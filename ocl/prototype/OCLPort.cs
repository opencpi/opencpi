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
        public OclBufferAvailableEvent(OCLBuffer buffer_, ICollection<ComputeEventBase> theEvents_)
        {
            theBuffer = buffer_;
            theEvents = theEvents_;
        }
    }

    class OCLPort
    {
        public enum Type { OUTPUT, INPUT };
        private string m_name;
        public Type m_type;
        private OCLContainer m_container;
        public OCLWorker m_worker;
        public ComputeBuffer<float>[] m_computeBufferArray;
        // This is an array of arrays (known as a jagged array)
        public float[][] m_bufferArray;
        private GCHandle[] m_bufferArrayHandle;
        private Queue<OCLBuffer> m_buffersAvailable;
        private Queue<OclBufferAvailableEvent> m_eventQueueTiedToBufferAvailability;
        public OCLBuffer m_currentBufferForKernelInstance;

        // The number and length of the buffers should probably be put in metadata somewhere,
        // but since this is a prototype, we will make it simple
        public const int NUM_BUFFERS = 2;
        const int BUFFER_LEN = 19200;

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
            m_bufferArrayHandle = new GCHandle[NUM_BUFFERS];

            for (uint i = 0; i < NUM_BUFFERS; i++)
            {
                m_computeBufferArray[i] = new ComputeBuffer<float>(m_container.m_context, flags, BUFFER_LEN);

                // For C#, we'll also need to keep a managed array of buffers
                m_bufferArray[i] = new float[BUFFER_LEN];
                m_bufferArrayHandle[i] = GCHandle.Alloc(m_bufferArray[i], GCHandleType.Pinned);

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
            // Non blocking write
            m_container.m_commandQueue.Write<float>(m_computeBufferArray[buffer_.m_index], false, 0, BUFFER_LEN, 
                m_bufferArrayHandle[buffer_.m_index].AddrOfPinnedObject(), events_);
        }

        public void read(ICollection<ComputeEventBase> events_, OCLBuffer buffer_)
        {
            // Non blocking read
            // All previous events must complete before the read is performed
            m_container.m_commandQueue.Read<float>(m_computeBufferArray[buffer_.m_index], false, 0, BUFFER_LEN, 
                m_bufferArrayHandle[buffer_.m_index].AddrOfPinnedObject(), events_);
        }

        public void makeBufferAvailableOnEventsCompletion(OCLBuffer buffer_, ICollection<ComputeEventBase> events_)
        {
            OclBufferAvailableEvent theNewEvent = new OclBufferAvailableEvent(buffer_, events_);
            m_eventQueueTiedToBufferAvailability.Enqueue(theNewEvent);
        }

        public void makeBufferAvailable(OCLBuffer buffer_)
        {
            // You would only call this for an output buffer

            m_buffersAvailable.Enqueue(buffer_);
        }
    }
}