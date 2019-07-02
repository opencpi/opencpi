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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "OcpiApi.hh"
#include <iostream>
#include <stdlib.h>

namespace OA = OCPI::API;
using namespace std;
int programRet = 0; 

unsigned int throwNumber(std::string propname)
{
  int retVal = 0; 
  if (propname == "test_double")
  {
    retVal = 1; 
  }
  else if (propname == "test_ulong")
  {
    retVal = 1;
  } 
  //else if (propname == "test_bool")
  //{
  //  retVal = 0;
  //}
  else if (propname == "test_char")
  {
    retVal = 11;
  }
  //else if (propname == "test_float")
  //{
  //  retVal = 0;
  //}
  else if (propname == "test_long")
  {
    retVal = 1;
  }
  //else if (propname == "test_longlong"
  //{
  //  retVal = 0;
  //}
  else if (propname == "test_short")
  {
    retVal = 11;
  }
  else if (propname == "test_uchar")
  {
    retVal = 2;
  }
  //else if (propname == "test_ulonglong")
  //{
  //  retVal = 0;
  //}
  else if (propname == "test_ushort")
  {
    retVal = 2;
  }
  
  return retVal; 
}

void printProp(OA::Application* app, std::string propname)
{
  double double_prop;
  float float_prop;
  int int_prop;
  unsigned int uint_prop;
  bool bool_prop;
  short short_prop;
  unsigned short ushort_prop;
  char char_prop;
  unsigned char uchar_prop;
  long long_prop;
  unsigned long ulong_prop;
  unsigned int throwNum = 0;   

  try {
  app->setPropertyValue("test_worker", propname, 4.5);
  app->getPropertyValue("test_worker", propname, double_prop);
  cout << "double output for " << propname << " is: " << double_prop << endl;
  throwNum++; 

  app->setPropertyValue("test_worker", propname, 5.5);
  app->getPropertyValue("test_worker", propname, float_prop);
  cout << "float output for " << propname << " is: " << float_prop << endl;
  throwNum++;

  app->setPropertyValue("test_worker", propname, -4);
  app->getPropertyValue("test_worker", propname, int_prop);
  cout << "int output for " << propname << " is: " << int_prop << endl;
  throwNum++;

  app->setPropertyValue("test_worker", propname, 4);
  app->getPropertyValue("test_worker", propname, uint_prop);
  cout << "uint output for " << propname << " is: " << uint_prop << endl;
  throwNum++;

  app->setPropertyValue("test_worker", propname, true);
  app->getPropertyValue("test_worker", propname, bool_prop);
  cout << "bool output for " << propname << " is: " << bool_prop << endl;
  throwNum++;

  app->setPropertyValue("test_worker", propname, -6);
  app->getPropertyValue("test_worker", propname, short_prop);
  cout << "short output for " << propname << " is: " << short_prop << endl;
  throwNum++;

  app->setPropertyValue("test_worker", propname, 6);
  app->getPropertyValue("test_worker", propname, ushort_prop);
  cout << "ushort output for " << propname << " is: " << ushort_prop << endl;
  throwNum++;

  app->setPropertyValue("test_worker", propname, 'A');
  app->getPropertyValue("test_worker", propname, char_prop);
  cout << "char output for " << propname << " is: " << char_prop << endl;
  throwNum++;

  app->setPropertyValue("test_worker", propname, 'B');
  app->getPropertyValue("test_worker", propname, uchar_prop);
  cout << "uchar output for " << propname << " is: " << uchar_prop << endl;
  throwNum++;

  app->setPropertyValue("test_worker", propname, -7);
  app->getPropertyValue("test_worker", propname, long_prop);
  cout << "long output for " << propname << " is: " << long_prop << endl;
  throwNum++;

  app->setPropertyValue("test_worker", propname, 7);
  app->getPropertyValue("test_worker", propname, ulong_prop);
  cout << "ulong output for " << propname << " is: " << ulong_prop << endl << endl;
  throwNum++;
  }
  catch (std::string &e)
  {
    if (throwNumber(propname) != throwNum) 
    { 
      programRet = 1; 
      cout << endl << "FAILURE: caught exception: " << e << endl <<" keep going" << endl << endl;
    }
    else
    {
      cout << endl << "caught exception: " << e << endl <<" keep going" << endl << endl;
    }
  }
}



void printPhase(OA::Application* app)
{
  printProp(app, "test_double");
  printProp(app, "test_ulong");
  printProp(app, "test_bool");
  printProp(app, "test_char");
  printProp(app, "test_float");
  printProp(app, "test_long");
  printProp(app, "test_longlong");
  printProp(app, "test_short");
  printProp(app, "test_uchar");
  printProp(app, "test_ulonglong");
  printProp(app, "test_ushort");
}

int main(/*int argc, char **argv*/) {
  programRet = 0; 

  try {
  OA::Application app("test_app.xml", NULL);

  app.initialize();
  app.dumpProperties();

  cout << "______INITIALIZED______" << endl;
  printPhase(&app);

  app.start();
  cout << "______STARTED______" << endl;
  printPhase(&app);

  app.stop();
  cout << "______STOPPED______" << endl;
  printPhase(&app);

  app.dumpProperties(false);
  }
  catch (std::string &e)
  {
    cout << "caught exception " << e << endl;
    programRet = 2; 
  }

  return programRet;
}
