using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Cloo;

namespace OclPrototype2
{
    class OCLDeviceDescription
    {
        public string platformName;
        public string deviceName;
        public ComputePlatform platform;
        public ComputeDevice device;
    }

    class OCLContainer
    {
        private static List<OCLDeviceDescription> m_deviceList = null;
        private ComputePlatform m_platform;
        public ComputeDevice m_device;
        public ComputeContext m_context;
        public ComputeCommandQueue m_commandQueue;

        // Return a list of OCL Device Descriptors
        // Each descriptor contains the name of the platform
        // and the name of the device
        public static List<OCLDeviceDescription> getAvailableDevices()
        {
            if (m_deviceList == null)
            {
                m_deviceList = new List<OCLDeviceDescription>();

                // Go through each platform (AMD, IBM, NVIDIA, etc.)
                for (int i = 0; i < ComputePlatform.Platforms.Count; i++)
                {
                    ComputePlatform platform = ComputePlatform.Platforms[i];
                    // Go through each device on this platform
                    for (int j = 0; j < platform.Devices.Count; j++)
                    {
                        OCLDeviceDescription descrip = new OCLDeviceDescription();
                        descrip.platformName = platform.Name;
                        descrip.deviceName = platform.Devices[j].Name;
                        descrip.platform = platform;
                        descrip.device = platform.Devices[j];
                        m_deviceList.Add(descrip);
                    }
                }
            }

            return m_deviceList;
        }

        // Constructor
        public OCLContainer(int deviceIndex_)
        {
            m_platform = m_deviceList[deviceIndex_].platform;
            m_device = m_deviceList[deviceIndex_].device;

            // Create a device list with only one device
            ComputeDevice[] devices;

            devices = new ComputeDevice[1];
            devices[0] = m_device;

            ComputeContextPropertyList properties = new ComputeContextPropertyList(m_platform);
            m_context = new ComputeContext(devices, properties, null, IntPtr.Zero);
            m_commandQueue = new ComputeCommandQueue(m_context, m_context.Devices[0], ComputeCommandQueueFlags.None);

        }
    }
}