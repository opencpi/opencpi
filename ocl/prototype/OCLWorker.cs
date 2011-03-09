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

    class OCLWorker
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

        private string m_componentDirectory;
        private string m_componentBaseName;
        public OCLContainer m_container;
        private ComputeProgram m_program;
        public Dictionary<string, OCLPort> portDictionary;
//        private Dictionary<string, GCHandle> m_propertyDictionary;

        // Note: This struct definition should be created by the ocpi_gen tool
        // but I will include it here since this is just a prototype
//        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
//        [StructLayout(LayoutKind.Sequential, Size = 8, Pack = 1)]
        [StructLayout(LayoutKind.Sequential)]
        public struct PropertyStruct
        {
//            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 50)]
//            public string myString;
//            [MarshalAs(UnmanagedType.U4)]
            public int wA;
//            [MarshalAs(UnmanagedType.U4)]
            public int wB;
            public int test;
        }

        const uint START = 0;
        public const uint MATRIXMUL_N_LOCAL_MEMORIES = 2;

        [StructLayout(LayoutKind.Sequential)]
        public struct KernelControlStruct
        {
            // written by the kernel
            // This should be layed out as a fixed array - but that is considered
            // unsafe in C# -- therefore do it this way instead.
            public uint lMemSize0;
            public uint lMemSize1;
        }
        private PropertyStruct[] m_properties;
        private ComputeBuffer<PropertyStruct> m_computeBufferProperties;
        private GCHandle m_propertyHandle;
        private List<OCLPort> m_portRunConditionList;
        private KernelControlStruct[] m_kernelControl;
        private ComputeBuffer<KernelControlStruct> m_computeBufferKernelControl;
        private GCHandle m_kernelControlHandle;
        private OCLKernelInstance m_kernelInstance;

        // Constructor
        public OCLWorker(string componentDirectory_, string componentBaseName_, OCLContainer container_)
        {
            m_componentDirectory = componentDirectory_;
            m_componentBaseName = componentBaseName_;
            m_container = container_;
            portDictionary = new Dictionary<string, OCLPort>();
            m_portRunConditionList = new List<OCLPort>();
#if BLAH
            // Concatentate directory name with file name and extension
            string completeFile = m_componentDirectory + @"\" + m_componentBaseName + ".bin";
            
            // Read the binary into an array
            byte[] rawBinary = File.ReadAllBytes(completeFile);
            
            // Put it in a format where OpenCL can use it.
            List<byte[]> binaries = new List<byte[]>();
            binaries.Add(rawBinary);

            // List of compute devices.  We will always just use one device
            List<ComputeDevice> devices = new List<ComputeDevice>();
            devices.Add(m_container.m_device);

            // Define the program
            ComputeProgram program = new ComputeProgram(m_container.m_context, binaries, devices);
#else
            StringBuilder kernelSource = new StringBuilder();
            StreamReader file1 = new StreamReader("matrixMul.h");
            StreamReader file2 = new StreamReader("matrixMul.cl");
            kernelSource.Append(file1.ReadToEnd());
            kernelSource.Append(file2.ReadToEnd());

            m_program = new ComputeProgram(m_container.m_context, kernelSource.ToString());
            m_program.Build(null, "-cl-fast-relaxed-math", null, IntPtr.Zero);
#endif

            // Note, if you were using source code, you would need to do program.Build here.
            // However, we are using binary, so it shouldn't be necessary.

            generatePortList();

            setupProperties();

            setupKernelControlStruct();

        }

        public void addPortToRunCondition(OCLPort port_)
        {
            m_portRunConditionList.Add(port_);
        }

        public void removePortFromRunCondition(OCLPort port_)
        {
            m_portRunConditionList.Remove(port_);
        }

        private bool isPortInRunConditionList(OCLPort port_)
        {
            return m_portRunConditionList.Contains(port_);
        }

        private void setupProperties()
        {
            m_properties = new PropertyStruct[1];
            m_propertyHandle = GCHandle.Alloc(m_properties, GCHandleType.Pinned);
            m_computeBufferProperties = new ComputeBuffer<PropertyStruct>(m_container.m_context, ComputeMemoryFlags.ReadWrite, 1);
        }

        private void setupKernelControlStruct()
        {
            m_kernelControl = new KernelControlStruct[1];
            m_kernelControlHandle = GCHandle.Alloc(m_kernelControl, GCHandleType.Pinned);
            m_computeBufferKernelControl = new ComputeBuffer<KernelControlStruct>(m_container.m_context, ComputeMemoryFlags.ReadWrite, 1);
        }

        // Generate the list of ports assocated with the worker component
        private void generatePortList()
        {
            // Open up the OWD XML metadata file, and find the reference to the
            // OCS XML metadata file

            string owdFile = m_componentDirectory + @"\" + m_componentBaseName + ".xml";

            if (File.Exists(owdFile))
            {
                XDocument doc = XDocument.Load(owdFile);

                string ocsFileNameBase = doc.Root.Element("xi_include").Attribute("href").Value.ToString();

                // The ocs is one directory up in the tree

                string ocsFile = m_componentDirectory + @"\..\" + ocsFileNameBase;

                if (File.Exists(ocsFile))
                {
                    doc = XDocument.Load(ocsFile);

                    // The DataInterfaceSpec element defines a port
                    var q = from c in doc.Descendants("DataInterfaceSpec") select c;

                    OCLPort port;

                    // Loop through each DataInterfaceSpec element (or port)
                    foreach (XElement elem in q)
                    {
                        // A "Producer="true"" attribute means that it is an output port
                        XAttribute producerAttribute = elem.Attribute("Producer");
                        bool producer;

                        // Not every port will have a producer attribute
                        if (producerAttribute != null)
                        {
                            if (producerAttribute.Value.ToString().ToUpper() == "TRUE")
                                producer = true;
                            else
                                producer = false;
                        }
                        else
                        {
                            producer = false;
                        }

                        if (producer)
                            port = new OCLPort(elem.Attribute("Name").Value.ToString(), OCLPort.Type.OUTPUT, this);
                        else
                            port = new OCLPort(elem.Attribute("Name").Value.ToString(), OCLPort.Type.INPUT, this);

                        portDictionary.Add(elem.Attribute("Name").Value.ToString(), port);
                    }
                }

            }

        }

        public void start()
        {
            // Start the worker

            // Write the worker properties structure to the device -- blocking
            m_container.m_commandQueue.Write<PropertyStruct>(m_computeBufferProperties, true, 0, 1, m_propertyHandle.AddrOfPinnedObject(), null);

            // Create the kernel
            ComputeKernel kernel = m_program.CreateKernel("matrixMulOther");

            int index = 0;

            // Set the arguments
            kernel.SetMemoryArgument(index++, m_computeBufferProperties);
            kernel.SetMemoryArgument(index++, m_computeBufferKernelControl);
            kernel.SetValueArgument<uint>(index++, START);

            // One dimensional for the start method
            long[] globalWorkOffset = { 0 };
            long[] globalWorkSize = { (long)1 };
            long[] localWorkSize = { (long)1 };

            // Execute
            m_container.m_commandQueue.Execute(kernel, globalWorkOffset, globalWorkSize, localWorkSize, null);

            // Wait for execution to complete
            m_container.m_commandQueue.Finish();

            // Read down the kernel control structure. Blocking call
            m_container.m_commandQueue.Read<KernelControlStruct>(m_computeBufferKernelControl, true, 0, 1, m_kernelControlHandle.AddrOfPinnedObject(),
                null);

            // Read down the properties - blocking call
            m_container.m_commandQueue.Read<PropertyStruct>(m_computeBufferProperties, true, 0, 1, m_propertyHandle.AddrOfPinnedObject(), 
                null);

            kernel.Dispose();
        }

        public void writeBufferToDeviceAndRunWorkerIfWarranted(OCLBuffer buffer_)
        {
            // Call this function when you want to write a buffer to the device and run the kernel (if warranted by the run conditions)
            // Note: buffer_ represents the input buffer that has new data

            // Start a new kernel instance if needed
            if (m_kernelInstance == null)
                m_kernelInstance = new OCLKernelInstance(m_program);
            
            ICollection<ComputeEventBase> inputBufferWriteEvent = new Collection<ComputeEventBase>();

            // Write the buffer to the device
            buffer_.m_port.write(inputBufferWriteEvent, buffer_);

            // Add this event to the list of events that must occur before executing the kernel
            m_kernelInstance.addToKernelEventDependencies(inputBufferWriteEvent);
            // Associate buffer with the kernel prior to its execution
            m_kernelInstance.tieBufferToKernelExecutionEventPriorToKernelExecution(buffer_);

            writeBufferInfoToDevice(buffer_.m_port);
            
            // If the port is in the run condition list, then execute the kernel
            if (isPortInRunConditionList(buffer_.m_port))
            {
                ICollection<ComputeEventBase> propertiesWriteEvent = new Collection<ComputeEventBase>();

                // Write the worker properties structure to the device -- non blocking
                m_container.m_commandQueue.Write<PropertyStruct>(m_computeBufferProperties, false, 0, 1, m_propertyHandle.AddrOfPinnedObject(), propertiesWriteEvent);

                // Add to the list of events that must complete before executing the kernel
                m_kernelInstance.addToKernelEventDependencies(propertiesWriteEvent);

                // A list of output buffers from the device
                List<OCLBuffer> outputBufferList = new List<OCLBuffer>();

                // Allocate the output buffers associated with the kernel execution
                // Transfer the output buffer info to the device prior to kernel execution
                foreach (KeyValuePair<string, OCLPort> pair in portDictionary)
                {
                    OCLPort port = pair.Value;

                    // If an output port exists, allocate an output buffer
                    if (port.m_type == OCLPort.Type.OUTPUT)
                    {
                        OCLBuffer outputBuffer;
                        outputBuffer = port.allocateOutputBuffForSystem();
                        outputBufferList.Add(outputBuffer);

                        // Write the buffer info at this time only for the output port -- it was already done previously for input ports
                        writeBufferInfoToDevice(port);
                    }
                }

                m_kernelInstance.execute(portDictionary, m_kernelControl, m_computeBufferProperties, m_container);
  
                ICollection<ComputeEventBase> propertyReadEvents = new Collection<ComputeEventBase>();

                foreach (ComputeEventBase thePrecedingEvent in m_kernelInstance.m_kernelExecutionEventDependencies)
                    propertyReadEvents.Add(thePrecedingEvent);

                // Read down the properties
                m_container.m_commandQueue.Read<PropertyStruct>(m_computeBufferProperties, false, 0, 1, m_propertyHandle.AddrOfPinnedObject(), propertyReadEvents);

                foreach (OCLBuffer buffer in outputBufferList)
                {
                    // One new event list per port is needed
                    // Make sure to copy the original events into this one

                    ICollection<ComputeEventBase> outputBufferEvents = new Collection<ComputeEventBase>();
                    foreach (ComputeEventBase thePrecedingEvent in m_kernelInstance.m_kernelExecutionEventDependencies)
                        outputBufferEvents.Add(thePrecedingEvent);

                    // Read the output port data
                    buffer.m_port.read(outputBufferEvents, buffer);

                    // Read down the updated output buffer info structure information that was modified by the kernel
                    m_container.m_commandQueue.Read<OCLBuffer.OCLBufferInfoStruct>(buffer.m_computeBufferCurrentBufferInfo, false, 0, 1,
                        buffer.m_currentBufferInfoHandle.AddrOfPinnedObject(), outputBufferEvents);

                    buffer.m_port.makeBufferAvailableOnEventsCompletion(buffer, outputBufferEvents, m_kernelInstance);
                }

                m_kernelInstance = null;
            }

        }

        private void writeBufferInfoToDevice(OCLPort port_)
        {
            ICollection<ComputeEventBase> bufferInfoWriteEvent = new Collection<ComputeEventBase>();

            // Write the buffer info data to the device
            m_container.m_commandQueue.Write<OCLBuffer.OCLBufferInfoStruct>(port_.m_currentBufferForKernelInstance.m_computeBufferCurrentBufferInfo,
                false, 0, 1, port_.m_currentBufferForKernelInstance.m_currentBufferInfoHandle.AddrOfPinnedObject(), bufferInfoWriteEvent);

            m_kernelInstance.addToKernelEventDependencies(bufferInfoWriteEvent);

        }

        public void setProperty(string propertyName_, int value_)
        {
//            GCHandle handle = m_propertyDictionary[propertyName_];
//            IntPtr ptr = handle.AddrOfPinnedObject();
//            int[] tempArray = {value_};
//            Marshal.Copy(tempArray, 0, ptr, 1);

            // This is cheating, but this is easier to do than to have to
            // deal with pointers in C#.  It's a prototype anyway
            switch (propertyName_)
            {
                case "WA":
                    m_properties[0].wA = value_;
                    break;
                case "WB":
                    m_properties[0].wB = value_;
                    break;
            }
        }

    }
}