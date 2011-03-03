using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

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
        public uint m_size;

        public OCLBuffer(uint index_, OCLPort port_)
        {
            m_index = index_;
            m_port = port_;
            m_state = State.AVAILABLE;
        }

        public void allocateByUser()
        {
            m_state = State.ALLOCATED_BY_USER;
        }

        public void allocateBySystem()
        {
            m_state = State.ALLOCATED_BY_SYSTEM;
        }

        public float[] getBufferStorage()
        {
            return (m_port.m_bufferArray[m_index]);
        }

        public void put()
        {
            // Call this version of "put" for output buffers (in order to retire them)
            putBase();
        }

        public void put(uint size_)
        {
            // Call this version of "put" for input buffers (so we can specify the size)
            m_size = size_;
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

                m_port.m_worker.run(this);
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