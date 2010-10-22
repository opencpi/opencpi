
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <string>
#include <iostream>
#include <ezxml.h>
#include <OcpiOsDebug.h>
#include <OcpiOsFileSystem.h>
#include <OcpiOsProcessManager.h>
#include <OcpiUtilVfs.h>
#include <OcpiUtilVfsUtil.h>
#include <OcpiUtilFileFs.h>
#include <OcpiUtilZipFs.h>
#include <OcpiUtilTest.h>
#include <OcpiUtilEzxml.h>
#include <OcpiUtilCommandLineConfiguration.h>

/*
 * ----------------------------------------------------------------------
 * Command line handling.
 * ----------------------------------------------------------------------
 */

class TestBinderConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  TestBinderConfigurator ();

public:
  bool help;
#if !defined (NDEBUG)
  bool debugBreak;
#endif
  bool verbose;

  std::string rccBinderExecutable;

private:
  static CommandLineConfiguration::Option g_options[];
};

TestBinderConfigurator::
TestBinderConfigurator ()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
#if !defined (NDEBUG)
    debugBreak (false),
#endif
    rccBinderExecutable ("ocpi-rcc-binder")
{
}

OCPI::Util::CommandLineConfiguration::Option
TestBinderConfigurator::g_options[] = {
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "rccBinderExecutable", "OCPI RCC Binder Executable",
    OCPI_CLC_OPT(&TestBinderConfigurator::rccBinderExecutable), 0 },
#if !defined (NDEBUG)
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "break", "Whether to break on startup",
    OCPI_CLC_OPT(&TestBinderConfigurator::debugBreak), 0 },
#endif
  { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    OCPI_CLC_OPT(&TestBinderConfigurator::help), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::END, 0, 0, 0, 0 }
};

static
void
printUsage (TestBinderConfigurator & config,
            const char * argv0)
{
  std::cout << "usage: " << argv0 << " [options]" << std::endl
            << "  options: " << std::endl;
  config.printOptions (std::cout);
}

/*
 * ----------------------------------------------------------------------
 * Helpers.
 * ----------------------------------------------------------------------
 */

void
writeFile (OCPI::Util::Vfs::Vfs & fs,
           const std::string & fileName,
           const char * data)
  throw (std::string)
{
  std::ostream * out = fs.openWriteonly (fileName, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
  *out << data;
  fs.close (out);
}

std::string
readFile (OCPI::Util::Vfs::Vfs & fs,
          const std::string & fileName)
  throw (std::string)
{
  std::istream * in = fs.openReadonly (fileName, std::ios_base::in | std::ios_base::binary);
  std::string res;
  char tmp[1024];

  while (!in->eof()) {
    in->read (tmp, 1024);
    res.append (tmp, in->gcount());
  }

  return res;
}

class TempFileHolder {
public:
  struct Info {
    std::string relName;
    std::string fileName;
    std::string nativeName;
    OCPI::Util::Vfs::EventualEraser eraser;
  };

public:
  TempFileHolder (OCPI::Util::FileFs::FileFs & fs,
                  const std::string & wd);
  ~TempFileHolder ();

  Info & add (const std::string & name);

private:
  OCPI::Util::FileFs::FileFs & m_fs;
  const std::string m_wd;
  std::vector<Info *> m_infos;
};

TempFileHolder::
TempFileHolder (OCPI::Util::FileFs::FileFs & fs,
                const std::string & wd)
  : m_fs (fs),
    m_wd (wd)
{
}

TempFileHolder::
~TempFileHolder ()
{
  for (std::vector<Info *>::iterator ii = m_infos.begin ();
       ii != m_infos.end(); ii++) {
    delete *ii;
  }
}

TempFileHolder::Info &
TempFileHolder::add (const std::string & name)
{
  Info * i = new Info;
  i->relName = name;
  i->fileName = OCPI::Util::Vfs::joinNames (m_wd, name);
  i->nativeName = m_fs.toNativeName (i->fileName);
  i->eraser.eventuallyErase (&m_fs, i->fileName);
  m_infos.push_back (i);
  return *i;
}

/*
 * ----------------------------------------------------------------------
 * Test data.
 * ----------------------------------------------------------------------
 */

/*
 * A simple PRF file.
 */

const char *
simplePRFData = "\
<?xml version=\"1.0\" encoding=\"us-ascii\"?>\n\
<!DOCTYPE properties SYSTEM \"properties.dtd\">\n\
<properties>\n\
    <description />\n\
    <simple id=\"DCE:505483cc-2b56-4156-99dc-1cd2a717aa2d\" type=\"string\" name=\"videoDevice\" mode=\"readwrite\">\n\
        <description>OCPI_MAX_SIZE=64</description>\n\
        <kind kindtype=\"configure\" />\n\
    </simple>\n\
    <simple id=\"DCE:5b008c52-dd59-435e-b3f6-3125df361c2c\" type=\"ushort\" name=\"width\" mode=\"readwrite\">\n\
        <kind kindtype=\"configure\" />\n\
    </simple>\n\
    <simple id=\"DCE:ff7df687-9223-4ddd-89fd-c5fadaa19370\" type=\"ushort\" name=\"height\" mode=\"readwrite\">\n\
        <kind kindtype=\"configure\" />\n\
    </simple>\n\
    <simple id=\"DCE:85cbe2ae-44f7-488d-a9de-4c994dc710e7\" type=\"ushort\" name=\"fps\" mode=\"readwrite\">\n\
        <kind kindtype=\"configure\" />\n\
    </simple>\n\
</properties>\n\
";

/*
 * A simple SCD file.
 */

const char *
simpleSCDData = "\
<?xml version=\"1.0\" encoding=\"us-ascii\"?>\n\
<!DOCTYPE softwarecomponent SYSTEM \"softwarecomponent.dtd\">\n\
<softwarecomponent>\n\
    <corbaversion>2.2</corbaversion>\n\
    <componentrepid repid=\"IDL:CF/Resource:1.0\" />\n\
    <componenttype>resource</componenttype>\n\
    <componentfeatures>\n\
        <supportsinterface repid=\"IDL:CF/Resource:1.0\" supportsname=\"Resource\" />\n\
        <supportsinterface repid=\"IDL:CF/PropertySet:1.0\" supportsname=\"PropertySet\" />\n\
        <supportsinterface repid=\"IDL:CF/PortSupplier:1.0\" supportsname=\"PortSupplier\" />\n\
        <supportsinterface repid=\"IDL:CF/LifeCycle:1.0\" supportsname=\"LifeCycle\" />\n\
        <supportsinterface repid=\"IDL:CF/TestableObject:1.0\" supportsname=\"TestableObject\" />\n\
        <ports>\n\
            <uses repid=\"IDL:CF/Port:1.0\" usesname=\"VideoOut\">\n\
                <porttype type=\"control\" />\n\
            </uses>\n\
        </ports>\n\
    </componentfeatures>\n\
    <interfaces>\n\
        <interface repid=\"IDL:CF/Resource:1.0\" name=\"Resource\">\n\
            <!--[Inherited interface IDL:CF/PropertySet:1.0]-->\n\
            <inheritsinterface repid=\"IDL:CF/PropertySet:1.0\" />\n\
            <!--[Inherited interface IDL:CF/PortSupplier:1.0]-->\n\
            <inheritsinterface repid=\"IDL:CF/PortSupplier:1.0\" />\n\
            <!--[Inherited interface IDL:CF/LifeCycle:1.0]-->\n\
            <inheritsinterface repid=\"IDL:CF/LifeCycle:1.0\" />\n\
            <!--[Inherited interface IDL:CF/TestableObject:1.0]-->\n\
            <inheritsinterface repid=\"IDL:CF/TestableObject:1.0\" />\n\
        </interface>\n\
        <interface repid=\"IDL:CF/Port:1.0\" name=\"Port\" />\n\
    </interfaces>\n\
    <propertyfile>\n\
        <localfile name=\"VideoCaptureComponent.prf.xml\" />\n\
    </propertyfile>\n\
</softwarecomponent>\n\
";

/*
 * ----------------------------------------------------------------------
 * ZipFileTest
 * ----------------------------------------------------------------------
 */

class ZipFileTest : public OCPI::Util::Test::Test {
public:
  ZipFileTest (OCPI::Util::FileFs::FileFs & fs,
               const std::string & workingDirectory,
               const std::string & rccBinderExecutable);
  void run ();

private:
  OCPI::Util::FileFs::FileFs & m_fs;
  std::string m_wd;
  std::string m_rccBinderExecutable;
};

ZipFileTest::
ZipFileTest (OCPI::Util::FileFs::FileFs & fs,
             const std::string & workingDirectory,
             const std::string & rccBinderExecutable)
  : OCPI::Util::Test::Test ("Zip File Test"),
    m_fs (fs),
    m_wd (workingDirectory),
    m_rccBinderExecutable (rccBinderExecutable)
{
}

void
ZipFileTest::run ()
{
  TempFileHolder temp (m_fs, m_wd);
  TempFileHolder::Info & prfFile = temp.add ("VideoCaptureComponent.prf.xml");
  writeFile (m_fs, prfFile.fileName, simplePRFData);

  TempFileHolder::Info & scdFile = temp.add ("VideoCapture.scd.xml");
  writeFile (m_fs, scdFile.fileName, simpleSCDData);

  TempFileHolder::Info & dllFile = temp.add ("test.dll");
  writeFile (m_fs, dllFile.fileName, "dummy");

  TempFileHolder::Info & exeFile = temp.add ("test.zip");

  OCPI::OS::ProcessManager::ParameterList parameters;
  parameters.push_back (std::string ("--workerDll=") + dllFile.nativeName);
  parameters.push_back (std::string ("--worker=dummyEntrypoint=") + scdFile.nativeName);
  parameters.push_back (std::string ("--output=") + exeFile.nativeName);

  OCPI::OS::ProcessManager exe (m_rccBinderExecutable, parameters);
  exe.wait ();

  OCPI::Util::ZipFs::ZipFs zip (&m_fs, exeFile.fileName);
  OCPI::Util::EzXml::Doc doc (zip, "ocpi-meta.inf");
  ezxml_t root = doc.getRootNode ();

  test (root);
  test (ezxml_name (root));
  test (std::strcmp (root->name, "cpx:rcc-executable") == 0);

  ezxml_t fileNode = ezxml_child (root, "file");
  test (fileNode);
  test (ezxml_txt (fileNode));
  test (std::strcmp (ezxml_txt (fileNode), dllFile.relName.c_str()) == 0);

  ezxml_t epNode = ezxml_get (root, "worker", 0, "entrypoint", -1);
  test (epNode);
  test (ezxml_txt (epNode));
  test (std::strcmp (ezxml_txt (epNode), "dummyEntrypoint") == 0);

  ezxml_t propNode = ezxml_get (root, "worker", 0, "properties", -1);
  test (propNode);
  test (ezxml_txt (propNode));
  test (std::strcmp (ezxml_txt (propNode), "1/4/262/4/38/0/0$videoDevice~1/0/0/0/001100110|k256/$width~1/0/256/0/001100110|j0/$height~1/0/258/0/001100110|j0/$fps~1/0/260/0/001100110|j0/$VideoOut~00|") == 0);

  std::string executable = readFile (zip, dllFile.relName);
  test (executable == "dummy");
}

/*
 * ----------------------------------------------------------------------
 * CreateSPDTest
 * ----------------------------------------------------------------------
 */

class CreateSPDTest : public OCPI::Util::Test::Test {
public:
  CreateSPDTest (OCPI::Util::FileFs::FileFs & fs,
                 const std::string & workingDirectory,
                 const std::string & rccBinderExecutable);
  void run ();

private:
  OCPI::Util::FileFs::FileFs & m_fs;
  std::string m_wd;
  std::string m_rccBinderExecutable;
};

CreateSPDTest::
CreateSPDTest (OCPI::Util::FileFs::FileFs & fs,
               const std::string & workingDirectory,
               const std::string & rccBinderExecutable)
  : OCPI::Util::Test::Test ("Create SPD Test"),
    m_fs (fs),
    m_wd (workingDirectory),
    m_rccBinderExecutable (rccBinderExecutable)
{
}

void
CreateSPDTest::run ()
{
  TempFileHolder temp (m_fs, m_wd);
  TempFileHolder::Info & prfFile = temp.add ("VideoCaptureComponent.prf.xml");
  writeFile (m_fs, prfFile.fileName, simplePRFData);

  TempFileHolder::Info & scdFile = temp.add ("VideoCapture.scd.xml");
  writeFile (m_fs, scdFile.fileName, simpleSCDData);

  TempFileHolder::Info & dllFile = temp.add ("test.dll");
  writeFile (m_fs, dllFile.fileName, "dummy");

  TempFileHolder::Info & spdFile = temp.add ("VideoCapture.spd.xml");
  TempFileHolder::Info & exeFile = temp.add ("test.zip");

  OCPI::OS::ProcessManager::ParameterList parameters;
  parameters.push_back (std::string ("--workerDll=") + dllFile.nativeName);
  parameters.push_back (std::string ("--worker=dummyEntrypoint=") + scdFile.nativeName);
  parameters.push_back (std::string ("--output=") + exeFile.nativeName);
  parameters.push_back (std::string ("--updateSPD"));

  OCPI::OS::ProcessManager exe (m_rccBinderExecutable, parameters);
  exe.wait ();

  OCPI::Util::EzXml::Doc doc (m_fs, spdFile.fileName);
  ezxml_t root = doc.getRootNode ();

  test (root);
  test (ezxml_name (root));
  test (std::strcmp (root->name, "softpkg") == 0);

  ezxml_t epNode = ezxml_get (root, "implementation", 0, "code", 0, "entrypoint", -1);
  test (epNode);
  test (ezxml_txt (epNode));
  test (std::strcmp (ezxml_txt (epNode), "dummyEntrypoint") == 0);

  ezxml_t pref1Node = ezxml_get (root, "implementation", 0, "dependency", 0, "propertyref", -1);
  test (pref1Node);
  test (ezxml_attr (pref1Node, "refid"));
  test (ezxml_attr (pref1Node, "value"));
  test (std::strcmp (ezxml_attr (pref1Node, "refid"), "DCE:c4b738d8-fbe6-4893-81cd-1bb7a77bfb43") == 0);
  test (std::strcmp (ezxml_attr (pref1Node, "value"), "RCC") == 0);
}

/*
 * ----------------------------------------------------------------------
 * UpdateSPDNoImplementationTest
 * ----------------------------------------------------------------------
 */

class UpdateSPDNoImplementationTest : public OCPI::Util::Test::Test {
public:
  UpdateSPDNoImplementationTest (OCPI::Util::FileFs::FileFs & fs,
                                 const std::string & workingDirectory,
                                 const std::string & rccBinderExecutable);
  void run ();

private:
  OCPI::Util::FileFs::FileFs & m_fs;
  std::string m_wd;
  std::string m_rccBinderExecutable;
  static const char * s_spdData;
};

/*
 * An SPD file with no implementation.  Note that this file is not
 * compliant as it is mandatory for an SPD file to have an implementation
 * element.
 */

const char *
UpdateSPDNoImplementationTest::
s_spdData = "\
<?xml version=\"1.0\" encoding=\"us-ascii\"?>\
<!DOCTYPE softpkg SYSTEM \"softpkg.dtd\">\
<softpkg id=\"DCE:6ab2e7d1-4f0c-445b-9e6e-5fd22c7d3cf9\" name=\"VideoCapture\" type=\"sca_compliant\" version=\"42\">\
    <title>Video Capture Component</title>\
    <author>\
        <name>Frank Pilhofer</name>\
        <company>Mercury Computer Systems, Inc.</company>\
    </author>\
    <descriptor name=\"\">\
        <localfile name=\"VideoCapture.scd.xml\" />\
    </descriptor>\
</softpkg>\
";

UpdateSPDNoImplementationTest::
UpdateSPDNoImplementationTest (OCPI::Util::FileFs::FileFs & fs,
                               const std::string & workingDirectory,
                               const std::string & rccBinderExecutable)
  : OCPI::Util::Test::Test ("Update SPD No Implementation"),
    m_fs (fs),
    m_wd (workingDirectory),
    m_rccBinderExecutable (rccBinderExecutable)
{
}

void
UpdateSPDNoImplementationTest::run ()
{
  TempFileHolder temp (m_fs, m_wd);
  TempFileHolder::Info & prfFile = temp.add ("VideoCaptureComponent.prf.xml");
  writeFile (m_fs, prfFile.fileName, simplePRFData);

  TempFileHolder::Info & scdFile = temp.add ("VideoCapture.scd.xml");
  writeFile (m_fs, scdFile.fileName, simpleSCDData);

  TempFileHolder::Info & spdFile = temp.add ("VideoCapture.spd.xml");
  writeFile (m_fs, spdFile.fileName, s_spdData);

  TempFileHolder::Info & exeFile = temp.add ("test.zip");
  TempFileHolder::Info & dllFile = temp.add ("test.dll");
  writeFile (m_fs, dllFile.fileName, "dummy");

  OCPI::OS::ProcessManager::ParameterList parameters;
  parameters.push_back (std::string ("--workerDll=") + dllFile.nativeName);
  parameters.push_back (std::string ("--worker=dummyEntrypoint=") + spdFile.nativeName);
  parameters.push_back (std::string ("--output=") + exeFile.nativeName);
  parameters.push_back (std::string ("--updateSPD"));
  parameters.push_back (std::string ("--os=Linux:2.6.19"));
  parameters.push_back (std::string ("--processor=ppc"));
  parameters.push_back (std::string ("--ocpiDeviceId=42"));

  OCPI::OS::ProcessManager exe (m_rccBinderExecutable, parameters);
  exe.wait ();

  OCPI::Util::EzXml::Doc doc (m_fs, spdFile.fileName);
  ezxml_t root = doc.getRootNode ();

  test (root);
  test (ezxml_name (root));
  test (std::strcmp (root->name, "softpkg") == 0);

  /*
   * Check that all the information from the original SPD is still there.
   */

  test (ezxml_attr (root, "id"));
  test (std::strcmp (ezxml_attr (root, "id"), "DCE:6ab2e7d1-4f0c-445b-9e6e-5fd22c7d3cf9") == 0);
  test (ezxml_attr (root, "version"));
  test (std::strcmp (ezxml_attr (root, "version"), "42") == 0);

  ezxml_t titleNode = ezxml_child (root, "title");
  test (titleNode);
  test (ezxml_txt (titleNode));
  test (std::strcmp (ezxml_txt (titleNode), "Video Capture Component") == 0);

  ezxml_t nameNode = ezxml_get (root, "author", 0, "name", -1);
  test (nameNode);
  test (ezxml_txt (nameNode));
  test (std::strcmp (ezxml_txt (nameNode), "Frank Pilhofer") == 0);

  /*
   * Check that the implementation was added with the desired information.
   */

  ezxml_t epNode = ezxml_get (root, "implementation", 0, "code", 0, "entrypoint", -1);
  test (epNode);
  test (ezxml_txt (epNode));
  test (std::strcmp (ezxml_txt (epNode), "dummyEntrypoint") == 0);

  ezxml_t osNode = ezxml_get (root, "implementation", 0, "os", -1);
  test (osNode);
  test (ezxml_attr (osNode, "name"));
  test (ezxml_attr (osNode, "version"));
  test (std::strcmp (ezxml_attr (osNode, "name"), "Linux") == 0);
  test (std::strcmp (ezxml_attr (osNode, "version"), "2.6.19") == 0);

  ezxml_t processorNode = ezxml_get (root, "implementation", 0, "processor", -1);
  test (processorNode);
  test (ezxml_attr (processorNode, "name"));
  test (std::strcmp (ezxml_attr (processorNode, "name"), "ppc") == 0);

  ezxml_t pref1Node = ezxml_get (root, "implementation", 0, "dependency", 0, "propertyref", -1);
  test (pref1Node);
  test (ezxml_attr (pref1Node, "refid"));
  test (ezxml_attr (pref1Node, "value"));
  test (std::strcmp (ezxml_attr (pref1Node, "refid"), "DCE:c4b738d8-fbe6-4893-81cd-1bb7a77bfb43") == 0);
  test (std::strcmp (ezxml_attr (pref1Node, "value"), "RCC") == 0);

  ezxml_t pref2Node = ezxml_get (root, "implementation", 0, "dependency", 1, "propertyref", -1);
  test (pref2Node);
  test (ezxml_attr (pref2Node, "refid"));
  test (ezxml_attr (pref2Node, "value"));
  test (std::strcmp (ezxml_attr (pref2Node, "refid"), "DCE:b59fa5e6-5eb4-44f6-90f6-0548508f2ba2") == 0);
  test (std::strcmp (ezxml_attr (pref2Node, "value"), "42") == 0);
}

/*
 * ----------------------------------------------------------------------
 * UpdateSPDExistingImplementationTest
 * ----------------------------------------------------------------------
 */

class UpdateSPDExistingImplementationTest : public OCPI::Util::Test::Test {
public:
  UpdateSPDExistingImplementationTest (OCPI::Util::FileFs::FileFs & fs,
                                       const std::string & workingDirectory,
                                       const std::string & rccBinderExecutable);
  void run ();

protected:
  void testUpdateAll ();
  void testUpdateNothing ();
  void testUpdateByInfo ();

private:
  OCPI::Util::FileFs::FileFs & m_fs;
  std::string m_wd;
  std::string m_rccBinderExecutable;
  static const char * s_spdData;
  static const char * s_implUUID;
};

/*
 * An SPD file with an implementation that we are updating.
 */

const char *
UpdateSPDExistingImplementationTest::
s_spdData = "\
<?xml version=\"1.0\" encoding=\"us-ascii\"?>\n\
<!DOCTYPE softpkg SYSTEM \"softpkg.dtd\">\n\
<softpkg id=\"DCE:6ab2e7d1-4f0c-445b-9e6e-5fd22c7d3cf9\" name=\"VideoCapture\" type=\"sca_compliant\" version=\"42\">\n\
    <title>Video Capture Component</title>\n\
    <author>\n\
        <name>Frank Pilhofer</name>\n\
        <company>Mercury Computer Systems, Inc.</company>\n\
    </author>\n\
    <descriptor name=\"\">\n\
        <localfile name=\"VideoCapture.scd.xml\" />\n\
    </descriptor>\n\
    <implementation id=\"DCE:70b385a5-d3e5-4146-8f26-4ce4e0c8dd7e\" aepcompliance=\"aep_compliant\">\n\
        <code type=\"SharedLibrary\">\n\
            <localfile name=\"oldexecutable.exe\"></localfile>\n\
            <entrypoint>oldEntrypoint</entrypoint>\n\
        </code>\n\
        <runtime name=\"RCC\"/>\n\
        <os name=\"VxWorks\" version=\"6.6\"/>\n\
        <processor name=\"x86\"/>\n\
        <dependency type=\"OCPI Runtime\">\n\
            <propertyref refid=\"DCE:c4b738d8-fbe6-4893-81cd-1bb7a77bfb43\" value=\"RCC\"></propertyref>\n\
        </dependency>\n\
        <dependency type=\"OCPI Device Id\">\n\
            <propertyref refid=\"DCE:b59fa5e6-5eb4-44f6-90f6-0548508f2ba2\" value=\"1234\"></propertyref>\n\
        </dependency>\n\
    </implementation>\n\
</softpkg>\n\
";

const char *
UpdateSPDExistingImplementationTest::
s_implUUID = "DCE:70b385a5-d3e5-4146-8f26-4ce4e0c8dd7e";

UpdateSPDExistingImplementationTest::
UpdateSPDExistingImplementationTest (OCPI::Util::FileFs::FileFs & fs,
                                     const std::string & workingDirectory,
                                     const std::string & rccBinderExecutable)
  : OCPI::Util::Test::Test ("Update SPD Existing Implementation"),
    m_fs (fs),
    m_wd (workingDirectory),
    m_rccBinderExecutable (rccBinderExecutable)
{
}

void
UpdateSPDExistingImplementationTest::run ()
{
  testUpdateAll ();
  testUpdateNothing ();
  testUpdateByInfo ();
}

void
UpdateSPDExistingImplementationTest::testUpdateAll ()
{
  TempFileHolder temp (m_fs, m_wd);
  TempFileHolder::Info & prfFile = temp.add ("VideoCaptureComponent.prf.xml");
  writeFile (m_fs, prfFile.fileName, simplePRFData);

  TempFileHolder::Info & scdFile = temp.add ("VideoCapture.scd.xml");
  writeFile (m_fs, scdFile.fileName, simpleSCDData);

  TempFileHolder::Info & spdFile = temp.add ("VideoCapture.spd.xml");
  writeFile (m_fs, spdFile.fileName, s_spdData);

  TempFileHolder::Info & exeFile = temp.add ("test.zip");
  TempFileHolder::Info & dllFile = temp.add ("test.dll");
  writeFile (m_fs, dllFile.fileName, "dummy");

  OCPI::OS::ProcessManager::ParameterList parameters;
  parameters.push_back (std::string ("--workerDll=") + dllFile.nativeName);
  parameters.push_back (std::string ("--worker=dummyEntrypoint=") + spdFile.nativeName + std::string(":") + std::string(s_implUUID));
  parameters.push_back (std::string ("--output=") + exeFile.nativeName);
  parameters.push_back (std::string ("--updateSPD"));
  parameters.push_back (std::string ("--os=Linux:2.6.19"));
  parameters.push_back (std::string ("--processor=ppc"));
  parameters.push_back (std::string ("--ocpiDeviceId=42"));

  OCPI::OS::ProcessManager exe (m_rccBinderExecutable, parameters);
  exe.wait ();

  OCPI::Util::EzXml::Doc doc (m_fs, spdFile.fileName);
  ezxml_t root = doc.getRootNode ();

  test (root);
  test (ezxml_name (root));
  test (std::strcmp (root->name, "softpkg") == 0);

  /*
   * Check that the implementation was updated with the desired information.
   */

  ezxml_t epNode = ezxml_get (root, "implementation", 0, "code", 0, "entrypoint", -1);
  test (epNode);
  test (ezxml_txt (epNode));
  test (std::strcmp (ezxml_txt (epNode), "dummyEntrypoint") == 0);

  ezxml_t osNode = ezxml_get (root, "implementation", 0, "os", -1);
  test (osNode);
  test (ezxml_attr (osNode, "name"));
  test (ezxml_attr (osNode, "version"));
  test (std::strcmp (ezxml_attr (osNode, "name"), "Linux") == 0);
  test (std::strcmp (ezxml_attr (osNode, "version"), "2.6.19") == 0);

  ezxml_t processorNode = ezxml_get (root, "implementation", 0, "processor", -1);
  test (processorNode);
  test (ezxml_attr (processorNode, "name"));
  test (std::strcmp (ezxml_attr (processorNode, "name"), "ppc") == 0);

  ezxml_t pref1Node = ezxml_get (root, "implementation", 0, "dependency", 0, "propertyref", -1);
  test (pref1Node);
  test (ezxml_attr (pref1Node, "refid"));
  test (ezxml_attr (pref1Node, "value"));
  test (std::strcmp (ezxml_attr (pref1Node, "refid"), "DCE:c4b738d8-fbe6-4893-81cd-1bb7a77bfb43") == 0);
  test (std::strcmp (ezxml_attr (pref1Node, "value"), "RCC") == 0);

  ezxml_t pref2Node = ezxml_get (root, "implementation", 0, "dependency", 1, "propertyref", -1);
  test (pref2Node);
  test (ezxml_attr (pref2Node, "refid"));
  test (ezxml_attr (pref2Node, "value"));
  test (std::strcmp (ezxml_attr (pref2Node, "refid"), "DCE:b59fa5e6-5eb4-44f6-90f6-0548508f2ba2") == 0);
  test (std::strcmp (ezxml_attr (pref2Node, "value"), "42") == 0);
}

void
UpdateSPDExistingImplementationTest::testUpdateNothing ()
{
  TempFileHolder temp (m_fs, m_wd);
  TempFileHolder::Info & prfFile = temp.add ("VideoCaptureComponent.prf.xml");
  writeFile (m_fs, prfFile.fileName, simplePRFData);

  TempFileHolder::Info & scdFile = temp.add ("VideoCapture.scd.xml");
  writeFile (m_fs, scdFile.fileName, simpleSCDData);

  TempFileHolder::Info & spdFile = temp.add ("VideoCapture.spd.xml");
  writeFile (m_fs, spdFile.fileName, s_spdData);

  TempFileHolder::Info & exeFile = temp.add ("test.zip");
  TempFileHolder::Info & dllFile = temp.add ("test.dll");
  writeFile (m_fs, dllFile.fileName, "dummy");

  OCPI::OS::ProcessManager::ParameterList parameters;
  parameters.push_back (std::string ("--workerDll=") + dllFile.nativeName);
  parameters.push_back (std::string ("--worker=dummyEntrypoint=") + spdFile.nativeName + std::string(":") + std::string(s_implUUID));
  parameters.push_back (std::string ("--output=") + exeFile.nativeName);
  parameters.push_back (std::string ("--updateSPD"));

  OCPI::OS::ProcessManager exe (m_rccBinderExecutable, parameters);
  exe.wait ();

  OCPI::Util::EzXml::Doc doc (m_fs, spdFile.fileName);
  ezxml_t root = doc.getRootNode ();

  test (root);
  test (ezxml_name (root));
  test (std::strcmp (root->name, "softpkg") == 0);

  /*
   * Check that the old implementation information was carried forward.
   * I.e., a new implementation element should be added without impacting
   * the existing one.
   */

  ezxml_t osNode = ezxml_get (root, "implementation", 0, "os", -1);
  test (osNode);
  test (ezxml_attr (osNode, "name"));
  test (ezxml_attr (osNode, "version"));
  test (std::strcmp (ezxml_attr (osNode, "name"), "VxWorks") == 0);
  test (std::strcmp (ezxml_attr (osNode, "version"), "6.6") == 0);

  ezxml_t processorNode = ezxml_get (root, "implementation", 0, "processor", -1);
  test (processorNode);
  test (ezxml_attr (processorNode, "name"));
  test (std::strcmp (ezxml_attr (processorNode, "name"), "x86") == 0);

  ezxml_t pref2Node = ezxml_get (root, "implementation", 0, "dependency", 1, "propertyref", -1);
  test (pref2Node);
  test (ezxml_attr (pref2Node, "refid"));
  test (ezxml_attr (pref2Node, "value"));
  test (std::strcmp (ezxml_attr (pref2Node, "refid"), "DCE:b59fa5e6-5eb4-44f6-90f6-0548508f2ba2") == 0);
  test (std::strcmp (ezxml_attr (pref2Node, "value"), "1234") == 0);
}

void
UpdateSPDExistingImplementationTest::testUpdateByInfo ()
{
  /*
   * When specifying matching OS and CPU types, the existing implementation
   * element should be found and updated rather than adding a new one.
   */

  TempFileHolder temp (m_fs, m_wd);
  TempFileHolder::Info & prfFile = temp.add ("VideoCaptureComponent.prf.xml");
  writeFile (m_fs, prfFile.fileName, simplePRFData);

  TempFileHolder::Info & scdFile = temp.add ("VideoCapture.scd.xml");
  writeFile (m_fs, scdFile.fileName, simpleSCDData);

  TempFileHolder::Info & spdFile = temp.add ("VideoCapture.spd.xml");
  writeFile (m_fs, spdFile.fileName, s_spdData);

  TempFileHolder::Info & exeFile = temp.add ("test.zip");
  TempFileHolder::Info & dllFile = temp.add ("test.dll");
  writeFile (m_fs, dllFile.fileName, "dummy");

  OCPI::OS::ProcessManager::ParameterList parameters;
  parameters.push_back (std::string ("--workerDll=") + dllFile.nativeName);
  parameters.push_back (std::string ("--worker=dummyEntrypoint=") + spdFile.nativeName);
  parameters.push_back (std::string ("--output=") + exeFile.nativeName);
  parameters.push_back (std::string ("--updateSPD"));
  parameters.push_back (std::string ("--os=VxWorks:6.6"));
  parameters.push_back (std::string ("--processor=x86"));
  parameters.push_back (std::string ("--ocpiDeviceId=1234"));

  OCPI::OS::ProcessManager exe (m_rccBinderExecutable, parameters);
  exe.wait ();

  OCPI::Util::EzXml::Doc doc (m_fs, spdFile.fileName);
  ezxml_t root = doc.getRootNode ();

  test (root);
  test (ezxml_name (root));
  test (std::strcmp (root->name, "softpkg") == 0);

  /*
   * Check that the implementation was updated.
   */

  ezxml_t implNode = ezxml_child (root, "implementation");
  test (implNode);
  test (ezxml_attr (implNode, "id"));
  test (std::strcmp (ezxml_attr (implNode, "id"), s_implUUID) == 0);
  test (!ezxml_next (implNode));


  ezxml_t epNode = ezxml_get (root, "implementation", 0, "code", 0, "entrypoint", -1);
  test (epNode);
  test (ezxml_txt (epNode));
  test (std::strcmp (ezxml_txt (epNode), "dummyEntrypoint") == 0);

  ezxml_t osNode = ezxml_get (root, "implementation", 0, "os", -1);
  test (osNode);
  test (ezxml_attr (osNode, "name"));
  test (ezxml_attr (osNode, "version"));
  test (std::strcmp (ezxml_attr (osNode, "name"), "VxWorks") == 0);
  test (std::strcmp (ezxml_attr (osNode, "version"), "6.6") == 0);

  ezxml_t processorNode = ezxml_get (root, "implementation", 0, "processor", -1);
  test (processorNode);
  test (ezxml_attr (processorNode, "name"));
  test (std::strcmp (ezxml_attr (processorNode, "name"), "x86") == 0);

  ezxml_t pref2Node = ezxml_get (root, "implementation", 0, "dependency", 1, "propertyref", -1);
  test (pref2Node);
  test (ezxml_attr (pref2Node, "refid"));
  test (ezxml_attr (pref2Node, "value"));
  test (std::strcmp (ezxml_attr (pref2Node, "refid"), "DCE:b59fa5e6-5eb4-44f6-90f6-0548508f2ba2") == 0);
  test (std::strcmp (ezxml_attr (pref2Node, "value"), "1234") == 0);
}

/*
 * ----------------------------------------------------------------------
 * Main.
 * ----------------------------------------------------------------------
 */

static
int
testRccBinderInt (int argc, char * argv[])
{
  TestBinderConfigurator config;
  int n_failed = 0;

  try {
    config.configure (argc, argv);
  }
  catch (const std::string & oops) {
    std::cerr << "Oops: " << oops << std::endl;
    return false;
  }

  if (config.help || argc != 1) {
    printUsage (config, argv[0]);
    return false;
  }

  try {
    OCPI::Util::Test::Suite tests ("OCPI RCC Binder Tests");
    OCPI::Util::FileFs::FileFs fs ("/");
    std::string cwd = OCPI::OS::FileSystem::cwd ();

    tests.add_test (new ZipFileTest (fs, cwd, config.rccBinderExecutable));
    tests.add_test (new CreateSPDTest (fs, cwd, config.rccBinderExecutable));
    tests.add_test (new UpdateSPDNoImplementationTest (fs, cwd, config.rccBinderExecutable));
    tests.add_test (new UpdateSPDExistingImplementationTest (fs, cwd, config.rccBinderExecutable));
    tests.run ();
    n_failed = tests.report ();
  }
  catch (const OCPI::Util::Test::Exception & e) {
    std::cout << std::endl << "Test Suite exception: " << e.what ( ) << std::endl;
  }
  catch (...) {
    std::cout << std::endl << "Test Suite exception: Unknown exception."<< std::endl;
  }

  return n_failed;
}

/*
 * Entrypoint for the VxWorks command line.
 */

extern "C" {
  int
  testRccBinder (int argc, char * argv[])
  {
    return testRccBinderInt (argc, argv);
  }
}

/*
 * Entrypoint for everybody else.
 */

int
main (int argc, char * argv[])
{
#if !defined (NDEBUG)
  {
    for (int i=1; i<argc; i++) {
      if (std::strcmp (argv[i], "--break") == 0) {
        OCPI::OS::debugBreak ();
        break;
      }
    }
  }
#endif

  return testRccBinderInt (argc, argv);
}
