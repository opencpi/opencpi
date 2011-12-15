using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using Cloo;

namespace OCLcc
{
    class Program
    {
        static void Main(string[] args)
        {
            // oclcc <source_file_list> -o <base_output_file_name>
            // [-device_name <name>] [-platform_vendor <name>] [-build_options <option_list>]

            List<string> sourceFileList = new List<string>();

            // Go through the arg list
            // First collect the source files, which should be listed at the beginning
            for (int i = 0; i < args.Length; i++)
            {
                if (args[i][0] != '-')
                {
                    sourceFileList.Add(args[i]);
                }
                else
                {
                    break;
                }
            }

            int argumentIndex = -1;
            // Now find the -o which represents the base output file name
            for (int i = 0; i < args.Length; i++)
            {
                if (args[i] == "-o")
                {
                    argumentIndex = i;
                }
            }
            string outputFileName = null;

            if (argumentIndex >= 0)
            {
                outputFileName = args[argumentIndex + 1];
            }
            else
            {
                Console.WriteLine("Missing the -o argument to describe the base output file name");
                System.Environment.Exit(-1);
            }

            string deviceName;
            argumentIndex = Array.BinarySearch(args, "-device_name");
            if (argumentIndex >= 0)
                deviceName = args[argumentIndex + 1];
            else
                deviceName = null;

            string platformVendorName;
            argumentIndex = Array.BinarySearch(args, "platform_vendor");
            if (argumentIndex >= 0)
                platformVendorName = args[argumentIndex + 1];
            else
                platformVendorName = null;

            string buildOptions;
            argumentIndex = Array.BinarySearch(args, "-build_options");
            if (argumentIndex >= 0)
                buildOptions = args[argumentIndex + 1];
            else
                buildOptions = null;

            int platformIndex;
            if (platformVendorName == null)
                platformIndex = 0;
            else
            {
                platformIndex = -1;
                // Go through each platform (AMD, IBM, NVIDIA, etc.)
                for (int i = 0; i < ComputePlatform.Platforms.Count; i++)
                {
                    if (ComputePlatform.Platforms[i].Name == platformVendorName)
                    {
                        platformIndex = i;
                        break;
                    }
                }
                if (platformIndex == -1)
                {
                    Console.WriteLine("Unable to find platform: " + platformVendorName);
                    System.Environment.Exit(-1);
                }
            }

            ComputePlatform platform = ComputePlatform.Platforms[platformIndex];

            int deviceIndex;
            if (deviceName == null)
                deviceIndex = 0;
            else
            {
                deviceIndex = -1;
                // Go through each device on the platform
                for (int i = 0; i < platform.Devices.Count; i++)
                {
                    if (platform.Devices[i].Name == deviceName)
                    {
                        deviceIndex = i;
                        break;
                    }
                }
                if (deviceIndex == -1)
                {
                    Console.WriteLine("Unable to find device: " + deviceName);
                    System.Environment.Exit(-1);
                }
            }

            ComputeDevice[] devices = new ComputeDevice[1];
            devices[0] = platform.Devices[deviceIndex];
            ComputeContextPropertyList properties = new ComputeContextPropertyList(platform);
            ComputeContext context = new ComputeContext(devices, properties, null, IntPtr.Zero);

            StringBuilder kernelSource = new StringBuilder();
            foreach (string filename in sourceFileList)
            {
                StreamReader file = new StreamReader(filename);
                kernelSource.Append(file.ReadToEnd());
            }

            ComputeProgram program = new ComputeProgram(context, kernelSource.ToString());
            try
            {
                program.Build(null, buildOptions, null, IntPtr.Zero);
            }
            catch
            {
               Console.WriteLine("Build not successful");
               Console.WriteLine(program.GetBuildLog(devices[0]));
               System.Environment.Exit(-1);
            }

            ICollection<byte[]> binaries = program.Binaries;
                    
            FileStream stream = new FileStream(outputFileName, FileMode.Create);
            BinaryWriter writer = new BinaryWriter(stream);

            // Since we only picked one device, there should only be one binary in the collection
            foreach (byte[] binary in binaries)
            {
                writer.Write(binary);
            }

            writer.Close();
            stream.Close();
        }
    }
}
