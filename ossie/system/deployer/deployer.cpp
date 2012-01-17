#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include <string.h>
#include <unistd.h>
#include <ossie/debug.h>
#include <ossie/cf.h>
#include <ossie/FileManager_impl.h>
#include <ossie/ossieSupport.h>

namespace bf = boost::filesystem;

std::string g_profilesuffix[] = { "prf.xml", "scd.xml", "spd.xml", "sad.xml", "dcd.xml", "dmd.xml", "DAS.xml" };
//std::vector<const char**> _suffix( profilesuffix, profilesuffix + sizeof(profilesuffix) / sizeof(char) );

void usage()
{
    std::cout << "[DEPLOYER] -src [source dir] -dst [destination dir] <-ORBInitRef NameSercvice=corbaname::<IP>>\n";
    exit(EXIT_SUCCESS);
}

void printMountSequence( CF::FileManager::MountSequence* _ms )
{
    for ( unsigned int i = 0; i < _ms->length(); i++ )
        std::cout << "[DEPLOYER] Mount Point[" << i << "] : " << (*_ms)[i].mountPoint << std::endl;
    exit(EXIT_SUCCESS);
}

bool isProfile( std::string _filename )
{
    for ( unsigned int i = 0; i < g_profilesuffix->length(); i++ )
        if ( _filename.find(g_profilesuffix[i]) != std::string::npos ) return true;
    return false;
}

std::vector<std::string> path2dir( bf::path _path )
{
    std::string _tmp;
    std::vector<std::string> _dirnames;
    bf::path::iterator _it = _path.begin();

    while ( _it != _path.end() ) {
        _tmp = *_it;
        if ( (_tmp != "/") && (_tmp != ".") )
            _dirnames.push_back(_tmp);
        ++_it;
    }

    return _dirnames;
}

int main( int argc, char** argv )
{
    int i = 1;
    unsigned int j = 0;
    int _mountpointidx = 0;
    bool is_single_file_transfer = false;
    bool _pathexists = false;
    std::string _slash = "/";
    std::string _srcdir, _dstdir, _tmp, _dstpathexists;

    std::vector<std::string> _srcfiles, _srcsubdirs, _dstsubdirs, _mprootdirs;

    ossieSupport::ORB* _orb = NULL;

    CORBA::Object_ptr _objptr = CORBA::Object::_nil();
    CosNaming::NamingContext_ptr _rootctx = CosNaming::NamingContext::_nil();
    CF::DomainManager_ptr _dmnMgr = CF::DomainManager::_nil();
    CF::FileManager_ptr _fileMgr = CF::FileManager::_nil();
    CF::File_ptr _filecopy = CF::File::_nil();
    CosNaming::Name _name;

    CF::FileManager::MountSequence* _mountpoints = NULL;

    _name.length(2);
    _name[0].id = (const char*) "DomainName1";
    _name[1].id = (const char*) "DomainManager";

    _orb = new ossieSupport::ORB( argc, argv );

    while ( i < argc ) {
        if ( strcmp( "-src", argv[i] ) == 0 ) _srcdir = argv[i+1];
        if ( strcmp( "-dst", argv[i] ) == 0 ) _dstdir = argv[i+1];
        i++;
    }

    if ( _srcdir.empty() || _dstdir.empty() ) usage();

    bf::path _srcpath( _srcdir );
    if ( !bf::exists(_srcpath) ) {
        std::cout << "[DEPLOYER] The path " << _srcdir << " does not exist\n";
        exit(EXIT_FAILURE);
    }

    bf::path _dstpath( _dstdir );

    _srcsubdirs = path2dir(_srcpath);
    _dstsubdirs = path2dir(_dstpath);

    if ( bf::is_regular(_srcpath) ) {
        _srcfiles.push_back(_srcpath.leaf());
        is_single_file_transfer = true;
    } else {
        bf::directory_iterator _diriterate(_srcpath), _dirend;
        while (_diriterate != _dirend) {
            if ( !bf::is_directory(_diriterate->status()) ) {
                _tmp = _diriterate->leaf();
                if ( isProfile(_tmp) )
                    _srcfiles.push_back(_tmp);
            }
            ++_diriterate;
        }
    }

    if (_srcfiles.empty()) {
        std::cout << "[DEPLOYER] No source files found" << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        _objptr = _orb->orb->resolve_initial_references("NameService");
        _rootctx = CosNaming::NamingContext::_narrow(_objptr);
    } catch ( CORBA::ORB::InvalidName& ) {
        // error message goes here
    } catch ( ... ) {
        // error message goes here
    }

    CORBA::release(_objptr);

    try {
        _objptr = _rootctx->resolve(_name);
        _dmnMgr = CF::DomainManager::_narrow(_objptr);
    } catch ( ... ) {
        // error message goes here
    }

    CORBA::release(_objptr);

    try {
        _fileMgr = _dmnMgr->fileMgr();
        _mountpoints = _fileMgr->getMounts();
    } catch ( ... ) {
        // error message goes here
    }

    std::string _mpfullpath = (const char*)(*_mountpoints)[0].mountPoint;
    for ( j = 1; j < _mountpoints->length(); j++ ) {
        _mpfullpath += _slash;
        _mpfullpath += (*_mountpoints)[j].mountPoint;
    }

    bf::path _mppath(_mpfullpath);
    _mprootdirs = path2dir(_mppath);

    for ( j = 0; j < _mprootdirs.size(); j++ ) {
        if ( _dstsubdirs[0] == _mprootdirs[j] ) {
            _mountpointidx = j;
            break;
        }
    }

    for ( j = 0; j < _dstsubdirs.size(); j++ ) {
        bf::path _reldstpath;
        for ( unsigned int k = j; k < _dstsubdirs.size(); k++ )
            _reldstpath /= _dstsubdirs[k];
        _dstpathexists = _slash + _reldstpath.string();
        try {
            _pathexists = (*_mountpoints)[_mountpointidx].fs->exists(_dstpathexists.c_str());
            if (_pathexists) break;
        } catch ( ... ) {
            // error message goes here
        }
        _dstpathexists.clear();
    }

    if ( !_pathexists ) {
        std::cout << "[DEPLOYER] Destination path " << _dstdir << " not found" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string _copypathexists = _dstpathexists + _slash + _srcsubdirs[_srcsubdirs.size()-1];

    try {
        _pathexists = (*_mountpoints)[_mountpointidx].fs->exists(_copypathexists.c_str());
    } catch ( ... ) {
        // error message goes here
    }

    if ( !_pathexists ) {
        if ( is_single_file_transfer ) {
            std::cout << "[DEPLOYER] Creating file " << _copypathexists << std::endl;
            try {
                _filecopy = (*_mountpoints)[_mountpointidx].fs->create(_copypathexists.c_str());
            } catch ( ... ) {
                // error message goes here
            }

            std::cout << "[DEPLOYER] Copying file " << _srcdir << " to " << _copypathexists << std::endl;
            CF::OctetSequence _buffer;
            _buffer.length(512);
            char _input[512];
            unsigned int _filesize = bf::file_size(_srcpath);
            bf::ifstream _localfile(_srcpath);
            while (_filesize > 0) {
                if (_filesize < 512) _buffer.length(_filesize);
                _localfile.read(_input, _buffer.length());

                for ( j = 0; j < _buffer.length(); j++ ) _buffer[j] = _input[j];

                try {
                    _filecopy->write(_buffer);
                } catch (...) {
                    // error message goes here
                }
                usleep(1000);
                _filesize -= _buffer.length();
            }
            std::cout << "[DEPLOYER] File copy of " << _srcdir << " complete" << std::endl;
            _filecopy->close();
            CORBA::release(_filecopy);
        } else {
            std::cout << "[DEPLOYER] Making destination directory " << _copypathexists << std::endl;
            try {
                (*_mountpoints)[_mountpointidx].fs->mkdir(_copypathexists.c_str());
            } catch ( ... ) {
                // error message goes here
            }
            CF::OctetSequence _buffer;
            char _input[512];
            for ( j = 0; j < _srcfiles.size(); j++ ) {
                _buffer.length(512);
                bf::path _srccopypath(_srcpath / _srcfiles[j]);
                std::string _copypath = _copypathexists + _slash + _srcfiles[j];
                try {
                    _filecopy = (*_mountpoints)[_mountpointidx].fs->create(_copypath.c_str());
                } catch (...) {
                    // error message goes here
                }
                unsigned int _filesize = bf::file_size(_srccopypath);
                bf::ifstream _localfile(_srccopypath);
                while (_filesize > 0) {
                    memset( (char*)_input, 0, 512);
                    if (_filesize < 512) _buffer.length(_filesize);
                    _localfile.read(_input, _buffer.length());
                    for ( unsigned int k = 0; k < _buffer.length(); k++ ) _buffer[k] = _input[k];
                    try {
                        _filecopy->write(_buffer);
                    } catch (...) {
                        // error message goes here
                    }
                    _filesize -= _buffer.length();
                }
                _filecopy->close();
                CORBA::release(_filecopy);
            }
        }
    } else {
        if (is_single_file_transfer) {
        } else {
        }
    }

    delete _mountpoints;

    CORBA::release(_fileMgr);
    CORBA::release(_dmnMgr);

    ossieSupport::ORB::orb->destroy();

    return 0;
}

