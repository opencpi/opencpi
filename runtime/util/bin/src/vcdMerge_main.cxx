/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <ctime>
#include <cstdio>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <stdint.h>
#include <OcpiUtilCommandLineConfiguration.h>
#include <OcpiOsAssert.h>

class OcpiRccBinderConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  OcpiRccBinderConfigurator ();

public:
  bool          help;
  bool          verbose;
  MultiString   inputFiles;
  std::string   outputFile;

private:
  static CommandLineConfiguration::Option g_options[];
};

// Configuration
static  OcpiRccBinderConfigurator config;

OcpiRccBinderConfigurator::
OcpiRccBinderConfigurator ()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
    verbose (false)
{
}

OCPI::Util::CommandLineConfiguration::Option
OcpiRccBinderConfigurator::g_options[] = {

  { OCPI::Util::CommandLineConfiguration::OptionType::MULTISTRING,
    "inputFiles", "Input Files to merge",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::inputFiles), 0 },

  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "outputFile", "Output File",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::outputFile), 0 },

  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::verbose), 0 },

  { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::help), 0 },

  { OCPI::Util::CommandLineConfiguration::OptionType::END, 0, 0, 0, 0 }
};

static
void
printUsage (OcpiRccBinderConfigurator & a_config,
            const char * argv0)
{
  std::cout << "usage: " << argv0 << " [options]" << std::endl
            << "  options: " << std::endl;
  a_config.printOptions (std::cout);
}


static
void
getNextSym( char * mysym )
{
#define SYMSTART 33
  //#define SYMEND   126
#define SYMEND   47
#define SYMLEN (SYMEND-SYMSTART)
  static int init=false;
  static char syms[SYMLEN];
  static int nextsym=0;
  if ( ! init ) {
    init = true;
    for ( int n=0; n<SYMLEN; n++ ) {
      syms[n] = (char)(SYMSTART + n);
    }
  }

  int rem=nextsym/SYMLEN;
  int mod=nextsym%SYMLEN;
  int idx=0;
  mysym[idx++] = syms[mod];
  while ( rem ) {
    nextsym++;
    mod=nextsym%SYMLEN;
    mysym[idx++] = syms[mod];
    rem--;
  }
  nextsym++;
  mysym[idx]=0;
}

void
testSyms()
{
  for ( int n=0; n<1024; n++ ) {
    char syms[80];
    getNextSym(syms);
    printf("Next sym = %s\n", syms );
  }
}

struct VCDBase {
  VCDBase( std::string& value ):m_value(value){}
  virtual std::string& toString(){return m_value;};
  std::string m_value;
  virtual ~VCDBase(){}
};


enum VCDType { VariableT, TagT, TimeT, Opaque2PartT, JunkT };
struct VCDValue :  public VCDBase {
  VCDValue ( std::string& v )
    :VCDBase(v),type(JunkT){parse();}
  void parse() {
    int c;
    for (unsigned int n=0; n<m_value.size(); n++) {
      if ( isspace( m_value[n] ) ) {
        continue;
      }
      else if ( m_value[n] == '#' ) {
        type = TimeT;
        //        printf("MV = %s\n", m_value.c_str() );
        if ( (c=sscanf( m_value.c_str(),"#%llu", (long long unsigned*)&time)) != 1 ) {
          printf("c = %d\n",c);
          throw std::string("Could not parse value (%s)\n", m_value.c_str() );
        }
        break;
      }
      else if ( m_value[n] == '$' ) {
        type = TagT;
        break;
      }
      else if ( (m_value[n]>=48) && (m_value[n]<=57) ) {
        type = VariableT;
        // Format 01010token
        std::string t;
        long nn;
        for (nn=m_value.length()-1; nn>=0; nn--) {
          if ( (m_value[nn]>=48) && (m_value[nn]<=57) ) {
            memcpy(token,t.c_str(),t.length());
            token[t.length()]=0;
            break;
          }
          if ( ! isspace( m_value[nn] )) {
            t += m_value[nn];
          }
        }
        memcpy(value, m_value.c_str(), nn+1);

        // can fail on $end, ok
        //        sscanf( m_value.c_str(),"%s %s",value, token);
        break;
      }
      else if ( m_value[n] == 'b' ) {
        type = Opaque2PartT;
        sscanf( m_value.c_str(),"%s %s",value, token);
        break;
      }
    }
  }

  virtual std::string& toString()
  {
    if ( type == VariableT) {
      char tmp[1024];
      sprintf(tmp,"%s%s\n",value, token );
      m_value = tmp;
    }
    else if ( type == Opaque2PartT ) {
      char tmp[1024];
      sprintf(tmp,"%s %s\n",value, token );
      m_value = tmp;
    }
    return m_value;
  }

  virtual ~VCDValue(){};

  VCDType type;
  char token[80];
  char value[128];
  uint64_t time;
};


struct VCDValDef : public VCDBase {
  VCDValDef( std::string& v )
    :VCDBase(v),type(TagT){parse();}

  void parse() {

    if ( m_value.find("$var") != std::string::npos ) {
      type = VariableT;
      char n[256];
      sscanf( m_value.c_str(),"$var %s %s %s %s", kind,size,token,n);
      name = n;
      getNextSym( n );
      printf("Replacing token %s with %s\n", token, n );
      old_token = token;
      strcpy( token, n );
    }
  }

  virtual std::string& toString()
  {
    if ( type == VariableT ) {
      char tmp[1024];
      sprintf(tmp,"$var %s %s %s %s $end\n", kind, size, token, name.c_str() );
      m_value = tmp;
    }
    return m_value;
  }

  virtual ~VCDValDef(){};

  VCDType type;
  char  kind[80];
  char  size[80];
  char  token[80];
  std::string  old_token;
  std::string  name;

};


void replaceTokens( std::vector<VCDValue> & values, std::vector<VCDValDef> & defs )
{
  std::vector<VCDValue>::iterator vit;
  std::vector<VCDValDef>::iterator dit;
  for ( vit=values.begin(); vit!=values.end(); vit++) {
    for ( dit=defs.begin(); dit!=defs.end(); dit++) {
      if ( (*vit).token == (*dit).old_token ) {
        strcpy( (*vit).token, (*dit).token );
        break;
      }
    }
  }
}


class VcdParser {
public:

  VcdParser( const char* file )
    :m_filename(file)
  {
    m_file.open( file, std::ios::in );
    if ( ! m_file.is_open() ) {
      std::string err("Unable to open input file: ");
      err += file;
      throw err;
    }
    parse();
  }

  void parse() {
    bool done;
    std::string line;

    // Header
    done = false;
    while ( ! done &&  ! m_file.eof() ) {
      getline(  m_file, line );
      printf("%s\n", line.c_str() );
      if ( line.find("$timescale") != std::string::npos ) {
        printf("Found the timescale !!");
      }
      if ( line.find("$scope") != std::string::npos ) {
        done = true;
      }
      else {
        header += line + "\n";
      }
    }
    printf("Header = %s\n", header.c_str() );

    // Definitions
    val_defs.push_back(VCDValDef(line));
    done = false;
    while ( ! done &&  ! m_file.eof() ) {
      getline( m_file, line );
      printf("%s\n", line.c_str() );
      if ( ( line.find("$enddefinitions" ) != std::string::npos ) ) {
        done = true;
      }
      else {
        val_defs.push_back(VCDValDef(line));
      }
    }


    // initial values (optional)
    done = false;
    bool ivfound=false;
    while ( ! done &&  ! m_file.eof() ) {
      getline( m_file, line );
      printf("%s\n", line.c_str() );
      if ( ( line.find("$dumpvars" ) != std::string::npos ) ) {
        ivfound = true;
        done = true;
      }
    }
    if ( ivfound ) {
      done = false;
      while ( ! done &&  ! m_file.eof() ) {
        getline( m_file, line );
        printf("%s\n", line.c_str() );
        if ( ( line.find("$end" ) != std::string::npos ) ) {
          done = true;
        }
        else {
          init_values.push_back(VCDValue(line) );
        }
      }
    }
    replaceTokens( init_values, val_defs );


    // values
    done = false;
    while ( ! done &&  ! m_file.eof() ) {
      getline( m_file, line );
      printf("%s\n", line.c_str() );
      if ( ( line.find("$dumpoff" ) != std::string::npos ) ) {
        done = true;
      }
      else {
        VCDValue v(line);
        if ( v.type == TimeT ) {
          std::vector<VCDValue> vv;
          vv.push_back(v);
          time.push_back( vv );
        }
        else if ( v.type != JunkT ) {
          time.back().push_back( v );
        }
      }
    }
    for ( int n=0; n<(int)time.size(); n++ ) {
      replaceTokens( time[n], val_defs );
    }

    if ( header.empty() ||
         val_defs.empty() ||
         time.empty() ) {
      std::string err("Invalid VCD file format ");
      err += m_filename;
      throw err;
    }
  }



  void merge( VcdParser* p )
  {
    unsigned int n,m;
    // If there are name collisions, correct them with a post fix
    for( n=0; n<val_defs.size(); n++ ) {
      for(  m=0; m<p->val_defs.size(); m++ ) {
        if ( val_defs[n].type == VariableT ) {
          if ( val_defs[n].name == p->val_defs[m].name ) {
            p->val_defs[m].name += "_Mrg";
          }
        }
      }
    }
    for(  m=0; m<p->val_defs.size(); m++ ) {
      val_defs.push_back( p->val_defs[m] );
    }

    //DEBUG, REMOVE
    for(  m=0; m<val_defs.size(); m++ ) {
      printf( "Var name = %s\n", val_defs[m].name.c_str() );
    }



    for(  m=0; m<p->init_values.size(); m++ ) {
      init_values.push_back( p->init_values[m] );
    }
    std::vector< std::vector<VCDValue> >::iterator t;
    std::vector< std::vector<VCDValue> >::iterator ts;
    for( t=p->time.begin(); t!=p->time.end(); t++ ) {
      bool inserted=false;
      for( ts=time.begin(); ts!=time.end(); ts++ ) {
        if ( (*t).begin()[0].time < (*ts).begin()[0].time ) {
          time.insert( ts, (*t) );
          inserted = true;
          break;
        }
        else if ( (*t).begin()[0].time == (*ts).begin()[0].time ) {
          for (unsigned int u=1; u<(*t).size(); u++ ) {
            (*ts).push_back( (*t)[u] );
          }
          inserted = true;
          break;
        }
      }
      if ( ! inserted ) {
        time.push_back( (*t) );
      }
    }

  }

  std::string  m_filename;
  int          time_scale;
  std::string  header;
  std::vector<VCDValDef>  val_defs;
  std::vector<VCDValue>   init_values;
  std::vector< std::vector<VCDValue> > time;
  std::fstream m_file;
};




static
void
formatOutput(  VcdParser * parser, std::fstream & out )
{
  unsigned int n;

  // Date
  char date[80];
  const char *fmt="%A, %B %d %Y %X";
  struct tm* pmt;
  time_t     raw_time;
  time ( &raw_time );
  pmt = gmtime( &raw_time );
  strftime(date,80,fmt,pmt);
  out << "$date" << std::endl;
  out << "         " << date << std::endl;
  out << "$end" << std::endl;

  // Version
  out << "$version" << std::endl;
  out << "            OCPI VCD Merged File Event Dumper V1.0" << std::endl;
  out << "$end" << std::endl;

  // Timescale
  out << "$timescale" << std::endl;
  out << "          1 us" << std::endl;
  out << "$end" << std::endl;

  // Now the definitions
  out << "$scope module Merge $end" << std::endl;
  for ( n=0; n<parser->val_defs.size(); n++ ) {
    out << parser->val_defs[n].toString() << std::endl;
  }
  out << "$upscope $end" << std::endl;
  out << "$enddefinitions $end" << std::endl;

  // Initial values
  out << "$dumpvars" << std::endl;
  for ( n=0; n<parser->init_values.size(); n++ ) {
    out << parser->init_values[n].toString() << std::endl;
  }
  out << "$end" << std::endl;

  // Time values
  out << std::endl << "$dumpoff" << std::endl;

  std::vector< std::vector<VCDValue> >::iterator t;
  for( t=parser->time.begin(); t!=parser->time.end(); t++ ) {
    for ( unsigned int m=0; m<(*t).size(); m++ ) {
      out << (*t)[m].toString() << std::endl;
    }
  }
  out << "$end" << std::endl;

}



int main( int argc, char ** argv )
{

  try {
    config.configure (argc, argv);
  }
  catch (const std::string & oops) {
    std::cerr << "Error: " << oops << std::endl;
    return -1;
  }
  if (config.help) {
    printUsage (config, argv[0]);
    return -1;
  }
  if ( config.outputFile.length() == 0 ) {
    printf("Output file must be specified\n");
    printUsage (config, argv[0]);
    return -1;
  }
  if ( config.inputFiles.size() < 2 ) {
    printf("At least 2 input files must be specified\n");
    printUsage (config, argv[0]);
    return -1;
  }
  if ( config.verbose ) {
    printf("Processing Input files:\n");
    std::vector<std::string>::iterator it;
    for ( it=config.inputFiles.begin(); it != config.inputFiles.end(); it++ ) {
      printf("   %s\n", (*it).c_str() );
    }
    printf("Output file = %s\n", config.outputFile.c_str() );
  }

  // Open the output file
  std::fstream outfile;
  outfile.open( config.outputFile.c_str(), std::ios::out | std::ios::trunc );
  if ( ! outfile.is_open() ) {
    std::string err("Unable to open Time::Emit dump file ");
    err += config.outputFile.c_str();
    fprintf( stderr, "%s\n", err.c_str());
    return -1;
  }

  // Now the input files
  std::vector<VcdParser*> parsers;
  std::vector<std::string>::iterator it;
  try {
    for ( it=config.inputFiles.begin(); it != config.inputFiles.end(); it++ ) {
      VcdParser* p = new VcdParser( (*it).c_str() );
      parsers.push_back( p );
    }
  }
  catch( std::string & err ) {
    fprintf( stderr, "%s\n", err.c_str());
    return -1;
  }

  // Do the actual merge
  std::vector<VcdParser*>::iterator mpit=parsers.begin();
  std::vector<VcdParser*>::iterator pit = mpit + 1;
  for ( ; pit!=parsers.end(); pit++ ) {
    (*mpit)->merge(*pit);
  }
  formatOutput( (*mpit), outfile );

}
