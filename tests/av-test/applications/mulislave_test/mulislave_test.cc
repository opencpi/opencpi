#include <iostream>
#include <unistd.h>
#include <cstdio>
#include <cassert>
#include <string>
#include <math.h>
#include "OcpiApi.hh"

double EPSILON = 0.00000001;

inline bool isAlmostEqual(double a, double b)
{
    return fabs(a - b) < EPSILON;
}

std::string checkWkr1Values(OCPI::API::Application* app, std::string comp_name)
{
  std::string ret_val;
  std::string temp_string;

  app->getProperty(comp_name.c_str(), "my_string", temp_string);
  if (temp_string != "test_string"){
    ret_val = comp_name + ".my_string is not set correctly";
    return ret_val;
  }

  app->getProperty(comp_name.c_str(), "my_enum", temp_string);
  if (temp_string != "third_enum"){
    ret_val = comp_name + ".my_enum is not set correctly. Was :" + temp_string +
              " and should be : third_enum" ;
    return ret_val;
  }
  return ret_val;
}

std::string checkValues(OCPI::API::Application* app, std::string comp_name)
{
  std::string ret_val;
  double temp_double;
  unsigned long temp_ulong;
  bool temp_bool;
  char temp_char;
  float temp_float;
  long temp_long;
  //long long temp_longlong;
  short temp_short;
  unsigned char temp_uchar;
  //unsigned long long temp_ulonglong;
  unsigned short temp_ushort;
  std::string temp_string;

  app->getPropertyValue(comp_name, "test_double", temp_double);
  if (!isAlmostEqual(temp_double, 5.0)){
    ret_val = comp_name + ".test_double is not set correctly";
    return ret_val;
  }
  app->getPropertyValue(comp_name, "test_ulong", temp_ulong);
  if (temp_ulong != 10){
    ret_val = comp_name + ".test_ulong is not set correctly";
    return ret_val;
  }

  app->getPropertyValue(comp_name, "test_bool", temp_bool);
  if (temp_bool != true){
    ret_val = comp_name + ".test_bool is not set correctly";
    return ret_val;
  }
  app->getPropertyValue(comp_name, "test_char", temp_char);
  if (temp_char != 'F'){
    ret_val = comp_name + ".test_char is not set correctly";
    return ret_val;
  }
  app->getPropertyValue(comp_name, "test_float", temp_float);
  if (!isAlmostEqual(temp_float, 2.0)){
    ret_val = comp_name + ".test_float is not set correctly";
    return ret_val;
  }
  app->getPropertyValue(comp_name, "test_long", temp_long);
  if (temp_long != 25){
    ret_val = comp_name + ".test_long is not set correctly";
    return ret_val;
  }
  app->getPropertyValue(comp_name, "test_longlong", temp_long);
  if (temp_long != 250){
    ret_val = comp_name + ".test_longlong is not set correctly";
    return ret_val;
  }
  app->getPropertyValue(comp_name, "test_short", temp_short);
  if (temp_short != 6){
    ret_val = comp_name + ".test_short is not set correctly";
    return ret_val;
  }
  app->getPropertyValue(comp_name, "test_uchar", temp_uchar);
  if (temp_uchar != 'G'){
    ret_val = comp_name + ".test_uchar is not set correctly";
    return ret_val;
  }
  app->getPropertyValue(comp_name, "test_ulonglong", temp_ulong);
  if (temp_ulong != 350){
    ret_val = comp_name + ".test_ulonglong is not set correctly";
    return ret_val;
  }
  app->getPropertyValue(comp_name, "test_ushort", temp_ushort);
  printf("Here is %u\n", temp_ushort);
  if (temp_ushort != 16){
    ret_val = comp_name + ".test_ushort is not set correctly.  Was :" + 
              std::to_string(static_cast<unsigned long long>(temp_ushort)) + 
              " and should be : 16" ;
    return ret_val;
  }
}

int main(int argc, char **argv)
{
  // Reference OpenCPI_Application_Development document for an explanation of the ACI
  try
  {
    OCPI::API::Application app("mulislave_test.xml");
    app.initialize(); // all resources have been allocated
    app.start();      // execution is started
    app.wait();       // wait until app is "done"


    std::string err;
    err = checkValues(&app, "comp1");
    if (!err.empty())
    {
      std::cerr << "app failed: " << err << std::endl;
      return 2;
    }
    err = checkWkr1Values(&app, "comp1");
    if (!err.empty())
    {
      std::cerr << "app failed: " << err << std::endl;
      return 3;
    }
    err = checkValues(&app, "comp2");
    if (!err.empty())
    {
      std::cerr << "app failed: " << err << std::endl;
      return 4;
    }
    err = checkWkr1Values(&app, "comp2");
    if (!err.empty())
    {
      std::cerr << "app failed: " << err << std::endl;
      return 5;
    }
    err = checkValues(&app, "comp3");
    if (!err.empty())
    {
      std::cerr << "app failed: " << err << std::endl;
      return 6;
    }
  }
  catch (std::string &e)
  {
    std::cerr << "app failed: " << e << std::endl;
    return 1;
  }
  return 0;
}
