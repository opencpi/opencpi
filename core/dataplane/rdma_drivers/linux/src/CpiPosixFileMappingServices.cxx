// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

 /*
 * Abstact:
 *	This file contain the "C" implementation for Posix file mapping functions
 *	required by the system services. It is normally included by targets
 *	that support Posix and thus shared across target OSs.
 *
 * Author: Tony Anzelmo
 *
 * Date: 10/16/03
 *
 */

#ifndef CPI_POSIX_FILEMAPPING_SERVICES_H_
#define CPI_POSIX_FILEMAPPING_SERVICES_H_

#include "CpiHostFileMappingServices.h"
#include <string>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

namespace DataTransfer {

  // CpiPosixFileMappingServices implements basic file mapping support on Posix compliant platforms.
  class CpiPosixFileMappingServices : public CpiFileMappingServices
  {
  public:
    // Create a mapping to a named file.
    //	strFilePath - Path to a file. If null, no backing store.
    //	strMapName	- Name of the mapping. Can be null.
    //	eAccess		- The type of access desired.
    //	iMaxSize	- Maximum size of mapping object.
    // Returns 0 for success or a platform specific error number.
    int CreateMapping (const char*  strFilePath, const char* strMapName, AccessType eAccess, CPI::OS::int64_t iMaxSize)
    {
      // Common call to do shm_open
      int rc = InitMapping (strFilePath, strMapName, eAccess, O_CREAT);
      if (rc == 0)
	{
	  // Set the size of the shared area
	  rc = ftruncate (m_fd, iMaxSize);
	  m_errno = rc;
	  if (rc != 0)
	    {
	      printf("CpiPosixFileMappingServices::CreateMapping: ftruncate failed with errno %d\n", m_errno);
	      TerminateMapping ();
	    }
	}
      return rc;
    }

    // Open an existing mapping to a named file.
    //	strMapName	- Name of the mapping.
    //	eAccess		- The type of access desired.
    // Returns 0 for success or a platform specific error number.
    int OpenMapping (const char* strMapName, AccessType eAccess)
    {
      return InitMapping (NULL, strMapName, eAccess, 0);
    }

    // Close an existing mapping.
    // Returns 0 for success or a platform specific error number.
    int CloseMapping ()
    {
      return TerminateMapping ();
    }

    // Map a segment of the file into the address space.
    //	iOffset		- Byte offset into file for this view.
    //	lLength		- Number of bytes to map.
    // Returns that virtual address or 0 if failure.
    void* MapView (CPI::OS::uint64_t iOffset, CPI::OS::uint64_t lLength, AccessType eAccess)
    {
      // Map access to protection
      int iProtect = MapAccessToProtect (eAccess);

      // Mapping a length of 0 means map the whole mapping.
      void* iRet = 0;
      int fRet = 0;
      if (lLength == 0)
	{
	  // Use the file "size"
	  struct stat statbuf;
	  fRet = fstat (m_fd, &statbuf);
	  lLength = statbuf.st_size;
	}

      // Do the mapping
      if (fRet == 0)
	{
	  iRet = mmap (NULL, lLength, iProtect, MAP_SHARED, m_fd, iOffset);
	}
      m_length = lLength;
      return iRet;
    }

    // Unmap a segment of a file from the address space.
    //	pVA		- Virtual address of view to unmap
    // Returns 0 for success or a platform specific error number.
    int UnMapView (void *pVA)
    {
      int iRet = munmap (pVA, m_length);
      return iRet;
    }

    // Return the last error that occurred for a file mapping operation.
    int GetLastError ()
    {
      return m_errno;
    }

    // Constructor
    CpiPosixFileMappingServices ()
    {
      m_fd = -1;
      m_errno = 0;
      m_length = 0;
    }

    // Destructor
    ~CpiPosixFileMappingServices () {};

  private:
    int	m_fd;			// File descriptor
    int	m_errno;		// Last error.
    int m_length;		// Length of last mapping

  private:

    // Common method to open shared memory
    int InitMapping (const char* strFilePath, std::string strMapName, AccessType eAccess, int iFlags)
    {
      // Terminate any current mapping
      TerminateMapping ();

      // Convert access type to a Posix flag set.
      int iOpenFlags = MapAccessTypeToOpen (eAccess);

      // A leading "/" is required.
      std::basic_string <char>::iterator str_Iter = strMapName.begin ();
      if (*str_Iter != '/')
	{
	  strMapName = "/" + strMapName;
	}

      // Open a shared memory object
      m_fd = shm_open ((const char *)strMapName.c_str (), iOpenFlags | iFlags, 0666);
      m_length = 0;
      if (m_fd == -1)
	{
	  m_errno = errno;
	  printf ("CpiPosixFileMapping::InitMapping: shm_open of %s failed with errno %d\n", strMapName.c_str (), m_errno);
	}
      return (m_fd == -1) ? m_errno : 0;
    }

    // Terminate any existing mapping.
    int TerminateMapping ()
    {
      if ( m_fd != -1 )
	close (m_fd);
      m_fd =  -1;
      return 0;
    }

    // Map an AccessType to a POSIX open flags
    int MapAccessTypeToOpen (AccessType eAccess)
    {
      switch (eAccess)
	{
	case ReadWriteAccess:
	case AllAccess:
	case CopyAccess:
	  return O_RDWR;
	case ReadOnlyAccess:
	  return O_RDONLY;
	default:
	  //	  throw CpiOsException ("Mapping access type is invalid");
	  return -1;
	}
    }

    // Map an AccessType to a POSIX protection flags
    int MapAccessToProtect (AccessType eAccess)
    {
      switch (eAccess)
	{
	case ReadWriteAccess:
	case AllAccess:
	case CopyAccess:
	  return PROT_READ | PROT_WRITE;
	case ReadOnlyAccess:
	  return PROT_READ;
	default:
	  //	  throw CpiOsException ("Mapping access type is invalid");
	  return -1;
	}
    }

  };

  CpiFileMappingServices* CreateFileMappingServices()
  {
    return new CpiPosixFileMappingServices ();
  }

}

#endif
