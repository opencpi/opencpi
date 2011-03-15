using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.IO;
using System.Xml.Linq;

namespace OCLGen
{
    class OCLProperty
    {
        public string name;
        public string type;
    }

    class OCLPort
    {
        public enum PortType {
            IN,
            OUT
        };

        public PortType type;
    }

    class Program
    {
        static void Main(string[] args)
        {
            string owdFile = Path.GetFullPath(args[0]);
            string path = Path.GetDirectoryName(owdFile);

            if (File.Exists(owdFile))
            {
                XDocument doc = XDocument.Load(owdFile);

                string ocsFileNameBase = doc.Root.Element("xi_include").Attribute("href").Value.ToString();

                // The ocs is one directory up in the tree

                string ocsFile = path + @"\..\" + ocsFileNameBase;

                if (File.Exists(ocsFile))
                {
                    doc = XDocument.Load(ocsFile);
                    
                    string componentName = doc.Root.Attribute("Name").Value.ToString();

                    // The DataInterfaceSpec element defines a port
                    var q = from c in doc.Descendants("DataInterfaceSpec") select c;

                    List<OCLPort> portList = new List<OCLPort>();

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

                        OCLPort port = new OCLPort();
                        if (producer)
                            port.type = OCLPort.PortType.OUT;
                        else
                            port.type = OCLPort.PortType.IN;

                        portList.Add(port);
                    }

                    XElement propertiesElement = doc.Root.Element("Properties");

                    q = from c in propertiesElement.Descendants("Property") select c;

                    List<OCLProperty> propertyList = new List<OCLProperty>();

                    // Loop through each Property
                    foreach (XElement elem in q)
                    {
                        OCLProperty property = new OCLProperty();
                        property.name = elem.Attribute("Name").Value.ToString();
                        property.type = elem.Attribute("Type").Value.ToString();
                        propertyList.Add(property);
                    }

                    string outDir;
                    string dirPrefix;
                    int argumentIndex = -1;
                    for (int i = 0; i < args.Length; i++)
                    {
                        if (args[i] == "-outdir")
                        {
                            argumentIndex = i;
                        }
                    }
                    if (argumentIndex >= 0)
                    {
                        outDir = Path.GetFullPath(args[argumentIndex + 1]);
                        // Create the directory if needed
                        Directory.CreateDirectory(outDir + @"\gen");
                        dirPrefix = outDir + @"\gen\";
                    }
                    else
                    {
                        outDir = null;
                        dirPrefix = "";
                    }

                    // Write to the worker.h file
                    TextWriter tw = new StreamWriter(dirPrefix + componentName + "_Worker.h");

                    // Total number of ports
                    tw.WriteLine("#define " + componentName.ToUpper() + "_N_PORTS (" + portList.Count.ToString() + ")");

                    uint totalPortIndex = 0;
                    uint inputPortIndex = 0;
                    uint outputPortIndex = 0;

                    // Go through the port defines
                    foreach (OCLPort port in portList)
                    {
                        if (port.type == OCLPort.PortType.IN)
                            tw.WriteLine("#define " + componentName.ToUpper() + "_IN" + inputPortIndex++.ToString() + " (" + totalPortIndex++.ToString() + ")");
                        else
                            tw.WriteLine("#define " + componentName.ToUpper() + "_OUT" + outputPortIndex++.ToString() + " (" + totalPortIndex++.ToString() + ")");
                    }

                    // Search argument list for number of number of dynamic local buffers
                    argumentIndex = -1;
                    for (int i = 0; i < args.Length; i++)
                    {
                        if (args[i] == "-max_num_dyn_local_buffs")
                        {
                            argumentIndex = i;
                        }
                    }
                    int numLocalBuffs;

                    if (argumentIndex >= 0)
                        numLocalBuffs = int.Parse(args[argumentIndex + 1]);
                    else
                        numLocalBuffs = 0;

                    tw.WriteLine("#define " + componentName.ToUpper() + "_N_LOCAL_MEMORIES (" + numLocalBuffs + ")");

                    // Now do the properties
                    tw.WriteLine();
                    tw.WriteLine("typedef struct {");

                    foreach (OCLProperty property in propertyList)
                    {
                        string typename = "int";

                        switch (property.name)
                        {
                            case "Long":
                                typename = "int";
                                break;
                            default:
                                break;
                        }

                        tw.WriteLine("  " + typename + " " + property.name + ";");
                    }

                    if (propertyList.Count != 0)
                    {
                        tw.WriteLine("} " + componentName + "Properties;");
                    }

                    // Now write the kernel control 
                    tw.WriteLine();
                    tw.WriteLine("typedef struct {");
                    tw.WriteLine("  uint32_t lMemSize[" + componentName.ToUpper() + "_N_LOCAL_MEMORIES];");
                    tw.WriteLine("} OCLKernelControl;");

                    // Now write the worker struct
                    tw.WriteLine();
                    tw.WriteLine("typedef struct {");
                    tw.WriteLine("  __global void*             properties;");
                    tw.WriteLine("  OCLPort                    ports[" + componentName.ToUpper() + "_N_PORTS];");
                    tw.WriteLine("  __local void*              lMemories[" + componentName.ToUpper() + "_N_LOCAL_MEMORIES];");
                    tw.WriteLine("  __global OCLKernelControl* controlInfo;");
                    tw.WriteLine("} OCLWorker;");
                    
                    // Now write the function prototypes
                    tw.WriteLine();
                    tw.WriteLine("void run (OCLWorker* self);");
                    tw.WriteLine("void start (OCLWorker* self);");

                    tw.Close();

                    // Write to the worker library file
                    tw = new StreamWriter(dirPrefix + componentName + "_library.cl");

                    tw.WriteLine("__kernel void");
                    tw.WriteLine(componentName + "Other(__global void* properties, __global OCLKernelControl* controlInfo, int method)");
                    tw.WriteLine("{");
                    tw.WriteLine("	OCLWorker worker;");
                    tw.WriteLine("	worker.properties = properties;");
                    tw.WriteLine("	worker.controlInfo = controlInfo;");
                    tw.WriteLine();
                    tw.WriteLine("	switch (method)");
                    tw.WriteLine("	{");
                    tw.WriteLine("		case START:");
                    tw.WriteLine("			start(&worker);");
                    tw.WriteLine("			break;");
                    tw.WriteLine("		default:");
                    tw.WriteLine("			break;");
                    tw.WriteLine("	}");
                    tw.WriteLine("}");
                    tw.WriteLine();
                    tw.WriteLine("__kernel void");
                    tw.Write(componentName + "Run(");
                    
                    // Go through the ports again
                    inputPortIndex = 0;
                    outputPortIndex = 0;

                    foreach (OCLPort port in portList)
                    {
                        if (port.type == OCLPort.PortType.IN)
                        {
                            tw.WriteLine("__global void* inputBuffer" + inputPortIndex.ToString() + ", __global OCLBufferInfo* inputBufferInfo" +
                                inputPortIndex.ToString() + ",");
                            inputPortIndex++;
                        }
                        else
                        {
                            tw.WriteLine("__global void* outputBuffer" + outputPortIndex.ToString() + ", __global OCLBufferInfo* outputBufferInfo" +
                                outputPortIndex.ToString() + ",");
                            outputPortIndex++;
                        }
                    }

                    // Go through local memories
                    for (int i = 0; i < numLocalBuffs; i++)
                    {
                        tw.WriteLine("__local void* localMemory" + i.ToString() + ",");
                    }

                    // Write the properties
                    tw.WriteLine("__global " + componentName + "Properties* properties)");

                    tw.WriteLine("{");

                    tw.WriteLine("    OCLWorker worker;");
                    tw.WriteLine("    worker.properties = properties;");

                    // Setup input buffer stuff
                    inputPortIndex = 0;
                    foreach (OCLPort port in portList)
                    {
                        if (port.type == OCLPort.PortType.IN)
                        {
                            string prefix = "    worker.ports[" + componentName.ToUpper() + "_IN" + inputPortIndex.ToString() + "].";
                            tw.WriteLine(prefix + "current.data = inputBuffer" + inputPortIndex.ToString() + ";");
                            tw.WriteLine(prefix + "current.maxLength = inputBufferInfo" + inputPortIndex.ToString() + "->maxLength;");
                            tw.WriteLine(prefix + "input.length = inputBufferInfo" + inputPortIndex.ToString() + "->length;");
                            tw.WriteLine(prefix + "input.u.operation = inputBufferInfo" + inputPortIndex.ToString() + "->operation_or_exception_ordinal;");
                            inputPortIndex++;
                        }
                    }

                    tw.WriteLine();

                    // Now do output ports
                    outputPortIndex = 0;
                    foreach (OCLPort port in portList)
                    {
                        if (port.type == OCLPort.PortType.OUT)
                        {
                            string prefix = "    worker.ports[" + componentName.ToUpper() + "_OUT" + outputPortIndex.ToString() + "].current.";
                            tw.WriteLine(prefix + "data = outputBuffer" + outputPortIndex.ToString() + ";");
                            tw.WriteLine(prefix + "maxLength = outputBufferInfo" + outputPortIndex.ToString() + "->maxLength;");
                            outputPortIndex++;
                        }
                    }

                    tw.WriteLine();

                    // Now setup local memories
                    for (int i = 0; i < numLocalBuffs; i++)
                    {
                        tw.WriteLine("    worker.lMemories[" + i.ToString() + "] = localMemory" + i.ToString() + ";");
                    }

                    tw.WriteLine();

                    tw.WriteLine("    run(&worker);");
                    tw.WriteLine();
                    tw.WriteLine("    // Update values to be sent down to the host");
                    outputPortIndex = 0;
                    foreach (OCLPort port in portList)
                    {
                        if (port.type == OCLPort.PortType.OUT)
                        {
                            tw.WriteLine("    outputBufferInfo" + outputPortIndex.ToString() + "->length = worker.ports[" +
                                componentName.ToUpper() + "_OUT" + outputPortIndex.ToString() + "].output.length;");
                            tw.WriteLine("    outputBufferInfo" + outputPortIndex.ToString() + "->operation_or_exception_ordinal = worker.ports[" +
                                componentName.ToUpper() + "_OUT" + outputPortIndex.ToString() + "].output.u.operation;");
                            outputPortIndex++;
                        }
                    }

                    tw.WriteLine("}");

                    if (numLocalBuffs != 0)
                    {
                        tw.WriteLine("void OCLsetLocalBuffSize (int index, int buffSize, OCLWorker* self)");
                        tw.WriteLine("{");
                        tw.WriteLine("    if (index < " + componentName.ToUpper() + "_N_LOCAL_MEMORIES)");
                        tw.WriteLine("        self->controlInfo->lMemSize[index] = buffSize;");
                        tw.WriteLine("}");
                    }

                    tw.Close();

                }
            }
        }
    }
}
