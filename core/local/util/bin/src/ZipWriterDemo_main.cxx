#include <CpiUtilZipFs.h>
#include <CpiUtilFileFs.h>
#include <CpiOsFileSystem.h>
#include <iostream>
#include <iomanip>
#include <string>

void
copyFilesRecursively (CPI::Util::Vfs::Vfs * fs1, const std::string & name1,
		      CPI::Util::Vfs::Vfs * fs2, const std::string & name2)
{
  CPI::Util::Vfs::Iterator * it = fs1->list (name1);

  while (!it->end()) {
    std::string targetName =
      CPI::Util::Vfs::joinNames (name2, it->relativeName());

    if (it->isDirectory()) {
      std::cout << "Recursing into "
		<< targetName
		<< "/"
		<< std::endl;

      copyFilesRecursively (fs1, it->absoluteName(),
			    fs2, targetName);

      std::cout << "Done with "
		<< targetName
		<< "/"
		<< std::endl;
    }
    else {
      std::cout << "Copying "
		<< targetName
		<< " (" << it->size() << " bytes) ... "
		<< std::flush;
      fs1->copy (it->absoluteName(), fs2, targetName);
      std::cout << "done." << std::endl;
    }

    it->next ();
  }

  fs1->closeIterator (it);
}

int
main (int argc, char *argv[])
{
  if (argc != 3) {
    std::cout << "usage: " << argv[0] << " <zipfile> <pattern>" << std::endl;
    return 1;
  }

  CPI::Util::FileFs::FileFs localFs ("/");
  std::string zipName = CPI::OS::FileSystem::fromNativeName (argv[1]);
  std::string absZipName = CPI::OS::FileSystem::absoluteName (zipName);
  std::string patName = CPI::OS::FileSystem::fromNativeName (argv[2]);
  std::string absPatName = CPI::OS::FileSystem::absoluteName (patName);
  std::string patDirName = CPI::OS::FileSystem::directoryName (absPatName);
  std::string patRelName = CPI::OS::FileSystem::relativeName (absPatName);
  CPI::Util::ZipFs::ZipFs zipFs;

  std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out;

  if (localFs.exists (absZipName)) {
    std::cout << "Using " << absZipName << std::endl;
  }
  else {
    std::cout << "Creating " << absZipName << std::endl;
    mode |= std::ios_base::trunc;
  }

  try {
    zipFs.openZip (&localFs, absZipName, mode);
  }
  catch (const std::string & oops) {
    std::cout << "error: " << oops << std::endl;
    return 1;
  }

  CPI::Util::Vfs::Iterator * toAdd = localFs.list (patDirName, patRelName);

  try {
    while (!toAdd->end()) {
      if (toAdd->isDirectory()) {
	std::cout << "Recursing into "
		  << toAdd->relativeName()
		  << "/"
		  << std::endl;
	copyFilesRecursively (&localFs,
			      toAdd->absoluteName(),
			      &zipFs,
			      toAdd->relativeName());
	std::cout << "Done with "
		  << toAdd->relativeName()
		  << "/"
		  << std::endl;
      }
      else {
	std::cout << "Copying "
		  << toAdd->relativeName()
		  << " (" << toAdd->size() << " bytes) ... "
		  << std::flush;
	localFs.copy (toAdd->absoluteName(),
		      &zipFs,
		      toAdd->relativeName());
	std::cout << "done." << std::endl;
      }

      toAdd->next ();
    }
  }
  catch (const std::string & oops) {
    std::cout << "oops: " << oops << std::endl;
  }

  localFs.closeIterator (toAdd);
  return 0;
}
