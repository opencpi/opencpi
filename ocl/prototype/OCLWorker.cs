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
        private ComputeKernel m_kernel;
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
        }
        private PropertyStruct[] m_properties;
        private ComputeBuffer<PropertyStruct> m_computeBufferProperties;
        private GCHandle m_propertyHandle;

        // Structure contains information for the current buffer on a port
        [StructLayout(LayoutKind.Sequential)]
        public struct OCLBufferInfoStruct
        {
            public uint length;
            public uint operation_or_exception_ordinal;
        }
        private OCLBufferInfoStruct[] m_currentBufferInfo;         // One of these per port
        private ComputeBuffer<OCLBufferInfoStruct> m_computeBufferCurrentBufferInfo;
        private GCHandle m_currentBufferInfoHandle;

        // Constructor
        public OCLWorker(string componentDirectory_, string componentBaseName_, OCLContainer container_)
        {
            m_componentDirectory = componentDirectory_;
            m_componentBaseName = componentBaseName_;
            m_container = container_;
            portDictionary = new Dictionary<string, OCLPort>();
//            m_eventListQueue = new Queue<ICollection<ComputeEventBase>>();

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

            ComputeProgram program = new ComputeProgram(m_container.m_context, kernelSource.ToString());
            program.Build(null, "-cl-fast-relaxed-math", null, IntPtr.Zero);
#endif

            // Note, if you were using source code, you would need to do program.Build here.
            // However, we are using binary, so it shouldn't be necessary.

            // All kernels, by design, will have the name "ocpiKernel"
            //            m_kernel = program.CreateKernel("ocpiKernel");
            m_kernel = program.CreateKernel("matrixMul");

            generatePortList();

            setupProperties();

        }

        private void setupProperties()
        {
//            m_propertyDictionary = new Dictionary<string, GCHandle>();
            m_properties = new PropertyStruct[1];
            m_propertyHandle = GCHandle.Alloc(m_properties, GCHandleType.Pinned);
            m_computeBufferProperties = new ComputeBuffer<PropertyStruct>(m_container.m_context, ComputeMemoryFlags.ReadWrite, 1);

//            m_propertyDictionary.Add("WA", GCHandle.Alloc(m_properties.wA, GCHandleType.Pinned));
//            m_propertyDictionary.Add("WB", GCHandle.Alloc(m_properties.wB, GCHandleType.Pinned));
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

            // Create one of these per port, signifying the information pertaining to 
            // the current buffer for that port
            m_currentBufferInfo = new OCLBufferInfoStruct[portDictionary.Count];
            m_currentBufferInfoHandle = GCHandle.Alloc(m_currentBufferInfo, GCHandleType.Pinned);
            m_computeBufferCurrentBufferInfo = new ComputeBuffer<OCLBufferInfoStruct>(m_container.m_context, ComputeMemoryFlags.ReadOnly, portDictionary.Count);

        }

        public void start()
        {
            // Start the worker
        }

        public void run(OCLBuffer buffer_)
        {
            // Call this function when you want to run the kernel
            // Note: buffer_ represents the input buffer that has new data

            ICollection<ComputeEventBase> events = new Collection<ComputeEventBase>();

            // Write the buffer to the device
            buffer_.m_port.write(events, buffer_);
            
            // Write the worker properties structure to the device -- non blocking
            m_container.m_commandQueue.Write<PropertyStruct>(m_computeBufferProperties, false, 0, 1, m_propertyHandle.AddrOfPinnedObject(), events);

            // A list of output buffers from the device
            List<OCLBuffer> outputBufferList = new List<OCLBuffer>();

            int index = 0;
            // Setup all of the kernel arguments

            // The port data goes first
            foreach (KeyValuePair<string, OCLPort> pair in portDictionary)
            {
                OCLPort port = pair.Value;

                // If an output port exists, allocate an output buffer
                if (port.m_type == OCLPort.Type.OUTPUT)
                {
                    OCLBuffer outputBuffer;
                    outputBuffer = port.allocateOutputBuffForSystem();
                    outputBufferList.Add(outputBuffer);
                }
                else
                {
                    // Place the length of the input buffer in the current buffer info array.
                    // This will be written to the kernel side.
                    m_currentBufferInfo[index].length = port.m_currentBufferForKernelInstance.m_size;
                    // Need to set the ordinal here some how.
                }

                m_kernel.SetMemoryArgument(index++, port.m_computeBufferArray[port.m_currentBufferForKernelInstance.m_index]);
            }

            // The current buffer info structure array -- to be passed to the kernel
            m_kernel.SetMemoryArgument(index++, m_computeBufferCurrentBufferInfo);

            // This stuff applies to the application
            // It is cheating.  We need to come back and fix this.
            m_kernel.SetLocalArgument(index++, sizeof(float) * BLOCK_SIZE * BLOCK_SIZE);
            m_kernel.SetLocalArgument(index++, sizeof(float) * BLOCK_SIZE * BLOCK_SIZE);

            // Properties struct
            m_kernel.SetMemoryArgument(index++, m_computeBufferProperties);

            // Now write the buffer info array to the device -- non blocking
            // There should be one of these structures per port
            m_container.m_commandQueue.Write<OCLBufferInfoStruct>(m_computeBufferCurrentBufferInfo, false, 0, m_currentBufferInfo.Length, 
                m_currentBufferInfoHandle.AddrOfPinnedObject(), events);

//            m_kernel.SetValueArgument<uint>(index++, WA);
//            m_kernel.SetValueArgument<uint>(index++, WB);

            long[] globalWorkOffset = { 0, 0 };
            long[] globalWorkSize = { (long)WC, (long)HC };
            long[] localWorkSize = { (long)BLOCK_SIZE, (long)BLOCK_SIZE };

            // Execute the kernel
            m_container.m_commandQueue.Execute(m_kernel, globalWorkOffset, globalWorkSize, localWorkSize, events);

            buffer_.m_port.makeBufferAvailableOnEventsCompletion(buffer_, events);

            foreach (OCLBuffer buffer in outputBufferList)
            {
                // One new event list per port is needed
                // Make sure to copy the original events into this one
                
                IList<ComputeEventBase> precedingEventList = new List<ComputeEventBase>();
                foreach (ComputeEventBase thePrecedingEvent in events)
                    precedingEventList.Add(thePrecedingEvent);

                ICollection<ComputeEventBase> outputBufferEvents = new Collection<ComputeEventBase>(precedingEventList);
 
                buffer.m_port.read(outputBufferEvents, buffer);
                buffer.m_port.makeBufferAvailableOnEventsCompletion(buffer, outputBufferEvents);
            }
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