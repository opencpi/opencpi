using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
using Cloo;

namespace OclPrototype2
{
    class OCLBuffer
    {
        // The buffer states flow as follows:
        // Input buffer flow
        // -----------------
        //   1) input_port->getBuffer() places input buffer in ALLOCATED_BY_USER state.
        //   2) input_buffer->put() places input buffer in ALLOCATED_BY_SYSTEM state.
        //   3) kernel execution complete event places input buffer in AVAILABLE state
        //      (this is detected in OCLPort::getBuffer()).
        // Output buffer flow
        // ------------------
        //   1) A Worker run condition (usually kicked off by an input_buffer->put() operation)
        //      places the output buffer in ALLOCATED_BY_SYSTEM state.
        //   2) output_port->getBuffer() waits on the next read-to-output-buffer event complete
        //      and places the output buffer in ALLOCATED_BY_USER state.
        //   3) output_buffer->put() places output buffer in AVAILABLE state.

       
        public enum State { ALLOCATED_BY_USER, ALLOCATED_BY_SYSTEM, AVAILABLE };
        public uint m_index;
        private State m_state;
        public OCLPort m_port;
        
        // Structure contains information for the buffer
        [StructLayout(LayoutKind.Sequential)]
        public struct OCLBufferInfoStruct
        {
            public uint length;
            public uint operation_or_exception_ordinal;
            public uint maxLength;
        }
        public OCLBufferInfoStruct[] m_currentBufferInfo;
        public ComputeBuffer<OCLBufferInfoStruct> m_computeBufferCurrentBufferInfo;
        public GCHandle m_currentBufferInfoHandle;

#if MAP_UNMAP_APPROACH
        public IntPtr m_mappedPtr;
#endif
        public OCLBuffer(uint index_, OCLPort port_)
        {
            m_index = index_;
            m_port = port_;
            m_state = State.AVAILABLE;

            m_currentBufferInfo = new OCLBufferInfoStruct[1];
            m_currentBufferInfoHandle = GCHandle.Alloc(m_currentBufferInfo, GCHandleType.Pinned);
            m_computeBufferCurrentBufferInfo = new ComputeBuffer<OCLBufferInfoStruct>(m_port.m_container.m_context, ComputeMemoryFlags.ReadWrite, 1);

            m_currentBufferInfo[0].maxLength = OCLPort.BUFFER_LEN;
        }

        public void allocateByUser()
        {
            m_state = State.ALLOCATED_BY_USER;

// The problem with the map/unmap approach is that the subsequent unmap operation
// on the component input port data will copy the entire buffer's worth of data
// to the device when the input_port_buffer->put() function is called.  But a size
// may be passed to the put() operation that is smaller than the buffer size
#if MAP_UNMAP_APPROACH
            if (m_port.m_type == OCLPort.Type.INPUT)
                m_mappedPtr = m_port.m_container.m_commandQueue.Map<float>(m_port.m_computeBufferArray[m_index], true, 
                    ComputeMemoryMappingFlags.Write, 0, OCLPort.BUFFER_LEN, null);
#endif
        }

        public void allocateBySystem()
        {
            m_state = State.ALLOCATED_BY_SYSTEM;
        }

        public float[] getBufferStorage()
        {
#if MAP_UNMAP_APPROACH
            // This is for C# only -- we must copy from an unmanaged ptr 
            // to a managed array
            if (m_port.m_type == OCLPort.Type.OUTPUT)
                Marshal.Copy(m_mappedPtr, m_port.m_bufferArray[m_index], 0, OCLPort.BUFFER_LEN);
#endif                
            return (m_port.m_bufferArray[m_index]);
        }

        public void put()
        {
            // Call this version of "put" for output buffers (in order to retire them)

            if (m_port.m_type == OCLPort.Type.INPUT)
                throw new OCLException("Should not call this version of put for an input buffer");

#if MAP_UNMAP_APPROACH
            if (m_port.m_type == OCLPort.Type.OUTPUT)
            {
                // I'm done with the output buffer so we can unmap it
                m_port.m_container.m_commandQueue.Unmap(m_port.m_computeBufferArray[m_index], ref m_mappedPtr, null);
            }
#endif
            putBase();
        }

        public uint getLength()
        {
            return m_currentBufferInfo[0].length;
        }

        public uint getOpCode()
        {
            return m_currentBufferInfo[0].operation_or_exception_ordinal;
        }

        public void put(uint opcode_, uint size_)
        {
            // Note: the put operation should also include endOfData but I don't know what to do with it

            // Call this version of "put" for input buffers (so we can specify the size and opcode)

            if (m_port.m_type == OCLPort.Type.OUTPUT)
                throw new OCLException("Should not call this version of put for an output buffer");
            
            
            m_currentBufferInfo[0].length = size_;
            m_currentBufferInfo[0].operation_or_exception_ordinal = opcode_;

            putBase();
        }
        private void putBase()
        {
           
            if (m_port.m_type == OCLPort.Type.INPUT)
            {
                // The buffer should have been allocated by the user (via port->getBuffer()) prior
                // to calling this function
                if (m_state != State.ALLOCATED_BY_USER)
                {
                    throw new OCLException("OCLBuffer::put() state != ALLOCATED_BY_USER");
                }

                // The input buffer now transitions to a state in which it is allocated by the system,
                // meaning it is tied to a kernel execution event before we can release it. 
                allocateBySystem();

                m_port.setBufferForNextKernelExecution(this);

                m_port.m_worker.writeBufferToDeviceAndRunWorkerIfWarranted(this);
            }
            else
            {
                // If you put an output buffer, we are done with it, so make it available again.
                m_state = State.AVAILABLE;
                // Put it back on the port's available buffer queue
                m_port.makeBufferAvailable(this);
            }
        }
    }
}