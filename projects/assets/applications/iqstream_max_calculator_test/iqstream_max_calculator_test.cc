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

#include <iostream>
#include <unistd.h> // sleep()
#include <cstdio>
#include <cassert>
#include <string>
#include <cstdlib> // EXIT_SUCCESS, EXIT_FAILURE
#include <sstream> // std::ostringstream
#include "OcpiApi.hh"

const int EXIT__TEST_COULD_NOT_COMPLETE=100;

namespace OA = OCPI::API;

OA::Short max_I, max_Q;
OA::Bool max_I_is_valid, max_Q_is_valid;
std::string read_fname_str;

void throw_max_I_is_valid_unexpected() {

  std::ostringstream oss(std::string("FAILED: for file: "));
  oss << read_fname_str << ", ";
  oss << "max_I_is_valid unexpectedly had a value of ";
  oss << (max_I_is_valid ? "true" : "false");
  throw oss.str();
}

void throw_max_Q_is_valid_unexpected() {

  std::ostringstream oss(std::string("FAILED: for file: "));
  oss << read_fname_str << ", ";
  oss << "max_Q_is_valid unexpectedly had a value of ";
  oss << (max_Q_is_valid ? "true" : "false");
  throw oss.str();
}

void test_max_I(OA::Short expected_val) {

  if(not max_I_is_valid) {
    throw_max_I_is_valid_unexpected();
  }
  if(max_I != expected_val) {
    std::ostringstream oss(std::string("FAILED: for file: "));
    oss << "expected value=" << expected_val << ", ";
    oss << "actual value=" << max_I;
    throw oss.str();
  }
}

void test_max_Q(OA::Short expected_val) {

  if(not max_Q_is_valid) {
    throw_max_Q_is_valid_unexpected();
  }
  if(max_Q != expected_val) {
    std::ostringstream oss(std::string("FAILED: for file: "));
    oss << "expected value=" << expected_val << ", ";
    oss << "actual value=" << max_Q;
    throw oss.str();
  }
}

bool test_range_of_I_Q(const std::string& file_str) {
  if(file_str.compare("max_I_0_Q_0.bin") == 0) {
    test_max_I(0);
    test_max_Q(0);
  }
  else if(file_str.compare("max_I_0_Q_1024.bin") == 0) {
    test_max_I(0);
    test_max_Q(1024);
  }
  else if(file_str.compare("max_I_1024_Q_0.bin") == 0) {
    test_max_I(1024);
    test_max_Q(0);
  }
  else if(file_str.compare("max_I_1024_Q_1024.bin") == 0) {
    test_max_I(1024);
    test_max_Q(1024);
  }
  else if(file_str.compare("max_I_is_valid_false_max_Q_is_valid_false.bin") == 0) {
    if(max_I_is_valid) {
      throw_max_I_is_valid_unexpected();
    }
    if(max_Q_is_valid) {
      throw_max_Q_is_valid_unexpected();
    }
  }
  else {
    return false; // file_str did not indicate this type of test
  }
  return true; // don't run any other test
}

void run_app(const std::string& app_xml_str, const std::string& file_str,
    bool no_file_write = false) {

  OA::Application app(app_xml_str);
  app.initialize(); // all resources have been allocated

  read_fname_str.assign("idata/");
  read_fname_str += file_str;
  app.setProperty("file_read", "fileName", read_fname_str.c_str());

  std::string write_fname_str("odata/");
  write_fname_str += file_str;

  if(not no_file_write) {
    app.setProperty("file_write", "fileName", write_fname_str.c_str());
  }

  app.start();

  if(no_file_write or (file_str.compare("10_ZLM_passthrough.bin") == 0)) {
    sleep(1);
    app.stop();
  }
  else {
    app.wait(); // wait for application's done=file_write
  }

  bool ret = true;
  if((app_xml_str.compare("iqstream_max_calculator_test_rcc_rcc.xml") != 0) and
     (app_xml_str.compare("iqstream_max_calculator_test_rcc_hdl_rcc.xml") != 0)) {
    const char* inst = "iqstream_max_calculator";

    // *MUST* read max_I_is_valid before max_I
    max_I_is_valid = app.getPropertyValue<OA::Bool>(inst, "max_I_is_valid");
    // *MUST* read max_Q_is_valid before max_Q
    max_Q_is_valid = app.getPropertyValue<OA::Bool>(inst, "max_Q_is_valid");

    max_I = app.getPropertyValue<OA::Short>(inst, "max_I");
    max_Q = app.getPropertyValue<OA::Short>(inst, "max_Q");

    ret = test_range_of_I_Q(file_str);
  }

  if(app_xml_str.compare("iqstream_max_calculator_test_rcc_rcc.xml") == 0) {
    {
      const char* inst0 = "iqstream_max_calculator0";

      // *MUST* read max_I_is_valid before max_I
      max_I_is_valid = app.getPropertyValue<OA::Bool>(inst0, "max_I_is_valid");
      // *MUST* read max_Q_is_valid before max_Q
      max_Q_is_valid = app.getPropertyValue<OA::Bool>(inst0, "max_Q_is_valid");

      max_I = app.getPropertyValue<OA::Short>(inst0, "max_I");
      max_Q = app.getPropertyValue<OA::Short>(inst0, "max_Q");

      test_max_I(0);
      test_max_Q(1024);
    }

    {
      const char* inst1 = "iqstream_max_calculator1";
      // *MUST* read max_I_is_valid before max_I
      max_I_is_valid = app.getPropertyValue<OA::Bool>(inst1, "max_I_is_valid");
      // *MUST* read max_Q_is_valid before max_Q
      max_Q_is_valid = app.getPropertyValue<OA::Bool>(inst1, "max_Q_is_valid");

      max_I = app.getPropertyValue<OA::Short>(inst1, "max_I");
      max_Q = app.getPropertyValue<OA::Short>(inst1, "max_Q");

      test_max_I(0);
      test_max_Q(1024);
    }
  }
  else if(app_xml_str.compare("iqstream_max_calculator_test_rcc_hdl_rcc.xml") == 0) {

    {
      const char* inst = "iqstream_max_calculator_rcc_start";

      // *MUST* read max_I_is_valid before max_I
      max_I_is_valid = app.getPropertyValue<OA::Bool>(inst, "max_I_is_valid");
      // *MUST* read max_Q_is_valid before max_Q
      max_Q_is_valid = app.getPropertyValue<OA::Bool>(inst, "max_Q_is_valid");

      max_I = app.getPropertyValue<OA::Short>(inst, "max_I");
      max_Q = app.getPropertyValue<OA::Short>(inst, "max_Q");

      test_range_of_I_Q(file_str);
    }

    {
      const char* inst = "iqstream_max_calculator_hdl";
      // *MUST* read max_I_is_valid before max_I
      max_I_is_valid = app.getPropertyValue<OA::Bool>(inst, "max_I_is_valid");
      // *MUST* read max_Q_is_valid before max_Q
      max_Q_is_valid = app.getPropertyValue<OA::Bool>(inst, "max_Q_is_valid");

      max_I = app.getPropertyValue<OA::Short>(inst, "max_I");
      max_Q = app.getPropertyValue<OA::Short>(inst, "max_Q");

      test_range_of_I_Q(file_str);
    }

    {
      const char* inst = "iqstream_max_calculator_rcc_end";
      // *MUST* read max_I_is_valid before max_I
      max_I_is_valid = app.getPropertyValue<OA::Bool>(inst, "max_I_is_valid");
      // *MUST* read max_Q_is_valid before max_Q
      max_Q_is_valid = app.getPropertyValue<OA::Bool>(inst, "max_Q_is_valid");

      max_I = app.getPropertyValue<OA::Short>(inst, "max_I");
      max_Q = app.getPropertyValue<OA::Short>(inst, "max_Q");

      test_range_of_I_Q(file_str);
    }
  }
  else if(file_str.compare("10_ZLM_passthrough.bin") == 0) {
    // test diff read/write files

    std::ostringstream oss;
    oss << "diff " << read_fname_str << " " << write_fname_str;
    std::string temp_str(oss.str());
    const char* cmd = temp_str.c_str();

    int ret = system(cmd);
    if(ret != 0) {
      std::ostringstream oss;
      oss << "ERROR: " << cmd << " returned non-zero value of " << ret;
      throw oss.str();
    }
  }
  else if(not ret) { // bad file_str
    assert(false);
  }

  ///@todo / FIXME - add this test back in
  /*if(not no_file_write) {

    // test diff read/write files

    std::ostringstream oss;
    oss << "diff " << read_fname_str << " " << write_fname_str;
    std::string temp_str(oss.str());
    const char* cmd = temp_str.c_str();

    int ret = system(cmd);
    if(ret != 0) {
      std::ostringstream oss(std::string("ERROR: "));
      oss << cmd << " returned non-zero value of " << ret;
      throw oss.str();
    }

  }*/
  std::cout << "TEST:   " << file_str << ": PASSED\n";
}

int main(int, char **) {
  bool hdl = false;
  unsigned n = 0;
  for (OA::Container *c; (c = OA::ContainerManager::get(n)); n++) {
    if (c->model() == "hdl") {
      hdl = true;
      break;
    }
  }

  try {
    std::cout << "TEST: file_read->RCC worker->file_write\n";
    run_app("iqstream_max_calculator_test_rcc.xml", "max_I_0_Q_0.bin");
    run_app("iqstream_max_calculator_test_rcc.xml", "max_I_0_Q_1024.bin");
    run_app("iqstream_max_calculator_test_rcc.xml", "max_I_1024_Q_0.bin");
    run_app("iqstream_max_calculator_test_rcc.xml", "max_I_1024_Q_1024.bin");
    run_app("iqstream_max_calculator_test_rcc.xml", "max_I_is_valid_false_max_Q_is_valid_false.bin");
    std::cout << "TEST: file_read->RCC worker\n";
    run_app("iqstream_max_calculator_test_no_file_write_rcc.xml", "max_I_0_Q_0.bin", true);
    run_app("iqstream_max_calculator_test_no_file_write_rcc.xml", "max_I_0_Q_1024.bin", true);
    run_app("iqstream_max_calculator_test_no_file_write_rcc.xml", "max_I_1024_Q_0.bin", true);
    run_app("iqstream_max_calculator_test_no_file_write_rcc.xml", "max_I_1024_Q_1024.bin", true);
    run_app("iqstream_max_calculator_test_no_file_write_rcc.xml", "max_I_is_valid_false_max_Q_is_valid_false.bin", true);
    std::cout << "TEST: RCC worker 10 ZLM passthrough\n";
    run_app("iqstream_max_calculator_test_zlm_passthrough_rcc.xml", "10_ZLM_passthrough.bin");

#if 0
    const char *env = getenv("OCPI_TEST_IQSTREAM_MAX_CALCULATOR_RCCONLY");
    if (env && env[0] == '1') {
      std::cout << "DONE: OCPI_TEST_IQSTREAM_MAX_CALCULATOR_RCCONLY explicitly stops application at this point\n";
      return 0;
    }

    if (!hdl) {
      std::cout << "ERROR: test could not be completed because no HDL containers were found\n";
      return EXIT__TEST_COULD_NOT_COMPLETE;
    }
#else
    if (!hdl) {
      std::cerr << "WARNING: some test cases could not be completed because no HDL containers were found\n";
    } else {
#endif
    std::cout << "TEST: file_read->HDL worker->file_write\n";
    run_app("iqstream_max_calculator_test_hdl.xml", "max_I_0_Q_0.bin");
    run_app("iqstream_max_calculator_test_hdl.xml", "max_I_0_Q_1024.bin");
    run_app("iqstream_max_calculator_test_hdl.xml", "max_I_1024_Q_0.bin");
    run_app("iqstream_max_calculator_test_hdl.xml", "max_I_1024_Q_1024.bin");
    run_app("iqstream_max_calculator_test_hdl.xml", "max_I_is_valid_false_max_Q_is_valid_false.bin");
    std::cout << "TEST: file_read->HDL worker\n";
    run_app("iqstream_max_calculator_test_no_file_write_hdl.xml", "max_I_0_Q_0.bin", true);
    run_app("iqstream_max_calculator_test_no_file_write_hdl.xml", "max_I_0_Q_1024.bin", true);
    run_app("iqstream_max_calculator_test_no_file_write_hdl.xml", "max_I_1024_Q_0.bin", true);
    run_app("iqstream_max_calculator_test_no_file_write_hdl.xml", "max_I_1024_Q_1024.bin", true);
    run_app("iqstream_max_calculator_test_no_file_write_hdl.xml", "max_I_is_valid_false_max_Q_is_valid_false.bin", true);
    std::cout << "TEST: HDL worker 10 ZLM passthrough\n";
    run_app("iqstream_max_calculator_test_zlm_passthrough_hdl.xml", "10_ZLM_passthrough.bin");

    ///@todo / FIXME - figure out why this test fails
    /*std::cout << "TEST: file_read->RCC->RCC->file_write\n";
    run_app("iqstream_max_calculator_test_rcc_rcc.xml", "max_I_0_Q_1024.bin");*/

    std::cout << "TEST: file_read->RCC->HDL->RCC->file_write\n";
    run_app("iqstream_max_calculator_test_rcc_hdl_rcc.xml", "max_I_0_Q_1024.bin");
    run_app("iqstream_max_calculator_test_rcc_hdl_rcc.xml", "max_I_0_Q_0.bin");
    run_app("iqstream_max_calculator_test_rcc_hdl_rcc.xml", "max_I_0_Q_1024.bin");
    run_app("iqstream_max_calculator_test_rcc_hdl_rcc.xml", "max_I_1024_Q_0.bin");
    run_app("iqstream_max_calculator_test_rcc_hdl_rcc.xml", "max_I_1024_Q_1024.bin");
    run_app("iqstream_max_calculator_test_rcc_hdl_rcc.xml", "max_I_is_valid_false_max_Q_is_valid_false.bin");
    }
  } catch (std::string &e) {
    std::cerr << "app failed: " << e << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "PASSED\n";
  return EXIT_SUCCESS;
}
