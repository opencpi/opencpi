#include <CpiUtilUri.h>
#include <CpiUtilHttpFs.h>
#include <CpiUtilTcpConnector.h>
#include <CpiUtilFileFs.h>
#include <CpiUtilUriFs.h>
#include <CpiOsFileSystem.h>
#include <iostream>
#include <string>

int
main (int argc, char *argv[])
{
  if (argc != 2) {
    std::cout << "usage: " << argv[0] << " <URI>" << std::endl;
    return 1;
  }

  std::string currentDir = CPI::OS::FileSystem::cwd ();
  CPI::Util::FileFs::FileFs localFs (currentDir);
  CPI::Util::Http::HttpFs<CPI::Util::Tcp::Connector> remoteFs;
  CPI::Util::Vfs::UriFs ufs;

  ufs.mount (&localFs);
  ufs.mount (&remoteFs);

  CPI::Util::Uri uri (argv[1]);
  std::string localName = uri.getFileName ();

  if (!localName.length()) {
    localName = "index.html";
  }

  std::string localURI = localFs.nameToURI (localName);
  std::string localAbsName = ufs.URIToName (localURI);
  std::string remoteAbsName = ufs.URIToName (uri.get());

  std::cout << "FileFs base URI is " << localFs.baseURI() << std::endl;
  std::cout << "HttpFs base URI is " << remoteFs.baseURI() << std::endl;

  std::cout << "Copying " << remoteAbsName
            << " to " << localAbsName
            << " ... " << std::flush;

  try {
    ufs.copy (remoteAbsName, &ufs, localAbsName);
    std::cout << "done." << std::endl;
  }
  catch (const std::string & err) {
    std::cout << " failed: " << err << std::endl;
  }

  return 0;
}
