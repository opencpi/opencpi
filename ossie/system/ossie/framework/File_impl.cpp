/****************************************************************************

Copyright 2004,2007 Virginia Polytechnic Institute and State University

This file is part of the OSSIE Core Framework.

OSSIE Core Framework is free software; you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

OSSIE Core Framework is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Lesser GNU General Public License for more details.

You should have received a copy of the Lesser GNU General Public License
along with OSSIE Core Framework; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

****************************************************************************/

#ifndef USE_OPENCPI_FS // This entire file isn't needed of we are using OpenCPI FS
#include <iostream>
#include <fstream>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem;

#include "ossie/File_impl.h"
#include "ossie/debug.h"

File_impl::File_impl (const char *fileName, fs::path &path, bool readOnly, bool create)
{
    DEBUG(6, File, "In constructor with " << fileName << " and path " << path.string());
    fName = fileName;
    fs::path filePath(path / fileName);

    std::ios_base::openmode mode;

    if (create)
        mode = std::ios::in|std::ios::out|std::ios::trunc;
    else if (readOnly)
        mode = std::ios::in;
    else
        mode = std::ios::in|std::ios::out;

    DEBUG(9, File, "Opening file " << filePath.string());

    f.open(filePath.string().c_str(), mode);

    if (!f.is_open()) {
        DEBUG(3, File, "File " << filePath.string().c_str() << " did not open succesfully.");
    }
    DEBUG(6, File, "Leaving constructor");
}


File_impl::~File_impl ()
{
    DEBUG(6, File, "In destructor.");
}


void File_impl::read (CF::OctetSequence_out data, CORBA::ULong length) throw (CORBA::SystemException, CF::File::IOException)
{
    DEBUG(6, File, "In read with length " << length);
    CORBA::Octet * buf = CF::OctetSequence::allocbuf (length);

    unsigned int count = f.readsome((char *)buf, length);
    DEBUG(10, File, "Read " << f.gcount() << " bytes from file.");

    data = new CF::OctetSequence(length, count, buf, true);

    if (f.fail()) {
        DEBUG(5, File, "Error reading from file, " << fName);
        throw CF::File::IOException (CF::CFEIO, "[File_impl::read] Error reading from file");
    }
}


void
File_impl::write (const CF::OctetSequence & data)
throw (CORBA::SystemException, CF::File::IOException)
{
    DEBUG(6, File, "In write");
    f.write((const char *)data.get_buffer(), data.length());

    if (f.fail()) {
        DEBUG(5, File, "Error writing to file, " << fName);
        throw (CF::File::IOException (CF::CFEIO, "[File_impl::write] Error writing to file"));
    }

}


CORBA::ULong File_impl::sizeOf ()throw (CORBA::SystemException,
                                        CF::FileException)
{
    DEBUG(6, File, "In sizeof.");
    CORBA::ULong fileSize(0), filePos(0);

    filePos = f.tellg();
    f.seekg(0, std::ios::end);
    fileSize = f.tellg();
    f.seekg(filePos);

    DEBUG(6, File, "In sizeOf with size = " << fileSize);

    return fileSize;
}


void
File_impl::close ()
throw (CORBA::SystemException, CF::FileException)
{
    DEBUG(6, File, "In close.");
    f.close();
    DEBUG(6, File, "Leaving close.");
}


void
File_impl::setFilePointer (CORBA::ULong _filePointer)
throw (CORBA::SystemException, CF::File::InvalidFilePointer,
       CF::FileException)
{
    DEBUG(6, File, "In setFilePointer.");
    if (_filePointer > this->sizeOf ()) {
        throw CF::File::InvalidFilePointer ();
    }

    f.seekg(_filePointer);
    f.seekp(_filePointer);
}
#endif /* of ifndef USE_OPENCPI_FS */
