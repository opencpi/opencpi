#include <CpiUtilZipFs.h>
#include <CpiUtilFileFs.h>
#include <CpiOsFileSystem.h>
#include <iostream>
#include <iomanip>
#include <string>

int
main (int argc, char *argv[])
{
  if (argc != 2 && argc != 3 && argc != 4) {
    std::cout << "usage: " << argv[0] << " <zipfile> [<fileinzip> [<localname>]]" << std::endl;
    return 1;
  }

  CPI::Util::FileFs::FileFs localFs ("/");
  std::string zipName = CPI::OS::FileSystem::fromNativeName (argv[1]);
  std::string absZipName = CPI::OS::FileSystem::absoluteName (zipName);
  CPI::Util::ZipFs::ZipFs zipFs;

  try {
    zipFs.openZip (&localFs, absZipName, std::ios_base::in);
  }
  catch (const std::string & oops) {
    std::cout << "error: " << oops << std::endl;
    return 1;
  }

  std::cout << "Zip File Contents" << std::endl;
  std::cout << "-----------------" << std::endl;

  CPI::Util::Vfs::Iterator * contents = zipFs.list (zipFs.cwd());

  while (!contents->end()) {
    if (contents->isDirectory()) {
      std::cout << "     (dir)";
    }
    else {
      std::cout << std::setw (10) << contents->size();
    }

    std::cout << "    "
	      << contents->relativeName ()
	      << std::endl;

    contents->next ();
  }

  std::cout << std::endl;
  zipFs.closeIterator (contents);
  
  if (argc == 3 || argc == 4) {
    std::string localNativeName;

    if (argc == 3) {
      localNativeName = argv[2];
    }
    else {
      localNativeName = argv[3];
    }

    std::string localName =
      CPI::OS::FileSystem::fromNativeName (localNativeName);
    std::string absLocalName =
      CPI::OS::FileSystem::absoluteName (localName);

    std::cout << "Copying " << argv[2]
	      << " from Zip file to " << absLocalName
	      << " ... " << std::flush;

    try {
      zipFs.copy (argv[2], &localFs, absLocalName);
      std::cout << "done." << std::endl;
    }
    catch (const std::string & err) {
      std::cout << " failed: " << err << std::endl;
    }
  }

  return 0;
}
