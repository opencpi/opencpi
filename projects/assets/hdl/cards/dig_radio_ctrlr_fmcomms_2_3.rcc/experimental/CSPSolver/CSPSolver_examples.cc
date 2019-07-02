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
#include "Math.hh" // OCPIProjects::Math
using namespace OCPIProjects::Math;

#define PRINT_VAR(x) std::cout << #x"=" << x << "\n"

enum class data_stream_type_t {rx, tx};
enum class gain_mode_t {agc, manual};

void test_add_vars(CSPSolver& solver) {

  const double dfp_comparison_tol = 1e-12;

  solver.add_var<double>("Rx_RFPLL_LO_freq_MHz", dfp_comparison_tol);
  solver.add_var<double>("Tx_RFPLL_LO_freq_MHz", dfp_comparison_tol);
  solver.add_var<double>("RX_SAMPL_FREQ_MHz", dfp_comparison_tol);
  solver.add_var<double>("TX_SAMPL_FREQ_MHz", dfp_comparison_tol);
  solver.add_var<double>("rx_rf_bandwidth_MHz", dfp_comparison_tol);
  solver.add_var<double>("tx_rf_bandwidth_MHz", dfp_comparison_tol);
  solver.add_var<int32_t>("DAC_Clk_divider");

  solver.add_var<int32_t>("data_stream_type_RX1");
  solver.add_var<int32_t>("data_stream_type_RX2");
  solver.add_var<int32_t>("data_stream_type_TX1");
  solver.add_var<int32_t>("data_stream_type_TX2");
  solver.add_var<double>("tuning_freq_MHz_RX1", dfp_comparison_tol);
  solver.add_var<double>("tuning_freq_MHz_RX2", dfp_comparison_tol);
  solver.add_var<double>("tuning_freq_MHz_TX1", dfp_comparison_tol);
  solver.add_var<double>("tuning_freq_MHz_TX2", dfp_comparison_tol);
  solver.add_var<double>("bandwidth_3dB_MHz_RX1", dfp_comparison_tol);
  solver.add_var<double>("bandwidth_3dB_MHz_RX2", dfp_comparison_tol);
  solver.add_var<double>("bandwidth_3dB_MHz_TX1", dfp_comparison_tol);
  solver.add_var<double>("bandwidth_3dB_MHz_TX2", dfp_comparison_tol);
  solver.add_var<double>("sampling_rate_Msps_RX1", dfp_comparison_tol);
  solver.add_var<double>("sampling_rate_Msps_RX2", dfp_comparison_tol);
  solver.add_var<double>("sampling_rate_Msps_TX1", dfp_comparison_tol);
  solver.add_var<double>("sampling_rate_Msps_TX2", dfp_comparison_tol);
  solver.add_var<int32_t>("samples_are_complex_RX1");
  solver.add_var<int32_t>("samples_are_complex_RX2");
  solver.add_var<int32_t>("samples_are_complex_TX1");
  solver.add_var<int32_t>("samples_are_complex_TX2");
  solver.add_var<int32_t>("gain_mode_RX1");
  solver.add_var<int32_t>("gain_mode_RX2");
  solver.add_var<int32_t>("gain_mode_TX1");
  solver.add_var<int32_t>("gain_mode_TX2");
  solver.add_var<int32_t>("gain_dB_RX1");
  solver.add_var<int32_t>("gain_dB_RX2");
  solver.add_var<double>("gain_dB_TX1", dfp_comparison_tol);
  solver.add_var<double>("gain_dB_TX2", dfp_comparison_tol);
}

void test_add_constrs(CSPSolver& solver) {

  solver.add_constr("Rx_RFPLL_LO_freq_MHz", ">=", 70.);
  solver.add_constr("Rx_RFPLL_LO_freq_MHz", "<=", 6000.);
  solver.add_constr("Tx_RFPLL_LO_freq_MHz", ">=", 70.);
  solver.add_constr("Tx_RFPLL_LO_freq_MHz", "<=", 6000.);
  solver.add_constr("rx_rf_bandwidth_MHz", ">=", 0.4);
  solver.add_constr("rx_rf_bandwidth_MHz", "<=", 56.);
  solver.add_constr("tx_rf_bandwidth_MHz", ">=", 1.25);
  solver.add_constr("tx_rf_bandwidth_MHz", "<=", 40.);
  /// @todo / FIXME - assumes FIR disabled, OpenCPI rounding error, etc
  solver.add_constr("RX_SAMPL_FREQ_MHz", ">=", 2.083334);
  solver.add_constr("RX_SAMPL_FREQ_MHz", "<=", 61.44);
  /// @todo / FIXME - assumes FIR disabled, OpenCPI rounding error, etc
  solver.add_constr("TX_SAMPL_FREQ_MHz", ">=", 2.083334);
  solver.add_constr("TX_SAMPL_FREQ_MHz", "<=", 61.44);
  solver.add_constr("DAC_Clk_divider", ">=", (int32_t)1);
  solver.add_constr("DAC_Clk_divider", "<=", (int32_t)2);
  //solver.add_constr("RX_SAMPL_FREQ_MHz", "=", "TX_SAMPL_FREQ_MHz", "x", "DAC_Clk_divider");

  solver.add_constr("data_stream_type_RX1", "=", (int32_t)data_stream_type_t::rx);
  solver.add_constr("data_stream_type_RX2", "=", (int32_t)data_stream_type_t::rx);
  solver.add_constr("data_stream_type_TX1", "=", (int32_t)data_stream_type_t::tx);
  solver.add_constr("data_stream_type_TX2", "=", (int32_t)data_stream_type_t::tx);
  solver.add_constr("tuning_freq_MHz_RX1", "=", "Rx_RFPLL_LO_freq_MHz");
  solver.add_constr("tuning_freq_MHz_RX2", "=", "Rx_RFPLL_LO_freq_MHz");
  solver.add_constr("tuning_freq_MHz_TX1", "=", "Tx_RFPLL_LO_freq_MHz");
  solver.add_constr("tuning_freq_MHz_TX2", "=", "Tx_RFPLL_LO_freq_MHz");
  solver.add_constr("bandwidth_3dB_MHz_RX1", "=", "rx_rf_bandwidth_MHz");
  solver.add_constr("bandwidth_3dB_MHz_RX2", "=", "rx_rf_bandwidth_MHz");
  solver.add_constr("bandwidth_3dB_MHz_TX1", "=", "tx_rf_bandwidth_MHz");
  solver.add_constr("bandwidth_3dB_MHz_TX2", "=", "tx_rf_bandwidth_MHz");
  solver.add_constr("sampling_rate_Msps_RX1", "=", "RX_SAMPL_FREQ_MHz");
  solver.add_constr("sampling_rate_Msps_RX2", "=", "RX_SAMPL_FREQ_MHz");
  solver.add_constr("sampling_rate_Msps_TX1", "=", "TX_SAMPL_FREQ_MHz");
  solver.add_constr("sampling_rate_Msps_TX2", "=", "TX_SAMPL_FREQ_MHz");
  solver.add_constr("samples_are_complex_RX1", "=", (int32_t)1);
  solver.add_constr("samples_are_complex_RX2", "=", (int32_t)1);
  solver.add_constr("samples_are_complex_TX1", "=", (int32_t)1);
  solver.add_constr("samples_are_complex_TX2", "=", (int32_t)1);
  solver.add_constr("gain_mode_RX1", ">=", (int32_t)gain_mode_t::agc);
  solver.add_constr("gain_mode_RX1", "<=", (int32_t)gain_mode_t::manual);
  solver.add_constr("gain_mode_RX2", ">=", (int32_t)gain_mode_t::agc);
  solver.add_constr("gain_mode_RX2", "<=", (int32_t)gain_mode_t::manual);
  solver.add_constr("gain_mode_TX1", "=", (int32_t)gain_mode_t::manual);
  solver.add_constr("gain_mode_TX2", "=", (int32_t)gain_mode_t::manual);
  CSPSolver::Constr::Cond if_freq_le_1300("Rx_RFPLL_LO_freq_MHz", "<=", 1300.);
  //solver.add_constr("gain_dB_RX1", ">=", 1, "if", cond_le1300);
  solver.add_constr("gain_dB_RX1", "<=", 77, &if_freq_le_1300);
  //solver.add_constr("gain_dB_RX2", ">=", 1, "if", cond_le1300);
  //solver.add_constr("gain_dB_RX2", "<=", 77, "if", cond_le1300);
  //auto cond_le4000 = solver.get_condition("Rx_RFPLL_LO_freq_MHz", "<=", 1400.);
  //solver.add_constr("gain_dB_RX1", ">=", -3, "if", cond_le4000);
  //solver.add_constr("gain_dB_RX1", "<=", 71, "if", cond_le4000);
  //solver.add_constr("gain_dB_RX2", ">=", -3, "if", cond_le4000);
  //solver.add_constr("gain_dB_RX2", "<=", 71, "if", cond_le4000);
  //auto cond_gt4000 = solver.get_condition("Rx_RFPLL_LO_freq_MHz", ">", 1400.);
  //solver.add_constr("gain_dB_RX1", ">=", -10, "if", cond_gt4000);
  //solver.add_constr("gain_dB_RX2", "<=", 62, "if", cond_gt4000);
  //solver.add_constr("gain_dB_RX2", ">=", -10, "if", cond_gt4000);
  solver.add_constr("gain_dB_TX1", ">=", -89.75);
  solver.add_constr("gain_dB_TX1", "<=", 0.);
  solver.add_constr("gain_dB_TX2", ">=", -89.75);
  solver.add_constr("gain_dB_TX2", "<=", 0.);
  solver.add_constr("bandwidth_3dB_MHz_RX1", "<=", "sampling_rate_Msps_RX1");
  solver.add_constr("bandwidth_3dB_MHz_RX2", "<=", "sampling_rate_Msps_RX2");
  solver.add_constr("bandwidth_3dB_MHz_TX1", "<=", "sampling_rate_Msps_TX1");
  solver.add_constr("bandwidth_3dB_MHz_TX2", "<=", "sampling_rate_Msps_TX2");
}

void test() {

  CSPSolver solver;

  test_add_vars(solver);
  test_add_constrs(solver);

  std::cout << "\n";
  PRINT_VAR(solver);
  std::cout << "\n";
  PRINT_VAR(solver.get_feasible_region_limits());
}

int main() {
  test();
  return 0;

  const double dfp_comparison_tol = 1e-12;
  size_t max_num_constr_prop_loop_iter = 2;

  CSPSolver solver(max_num_constr_prop_loop_iter);
  solver.add_var<double>("x1", dfp_comparison_tol);
  solver.add_var<double>("x2", dfp_comparison_tol);
  solver.add_var<double>("x3", dfp_comparison_tol);
  solver.add_var<double>("x4", dfp_comparison_tol);
  solver.add_var<double>("x5", dfp_comparison_tol);
  solver.add_var<double>("x6", dfp_comparison_tol);
  solver.add_var<double>("x7", dfp_comparison_tol);
  solver.add_constr("x2", ">=", 5.);
  solver.add_constr("x3", "<=", 21983740.);
  solver.add_constr("x4", "=", 999.);
  solver.add_constr("x5", ">=", "x2");
  solver.add_constr("x6", "<=", "x4");
  solver.add_constr("x7", "=", "x3");
  //solver.remove_constraint(
  //solver.add_constr("x1", "lt_or_equal_to_var", "x2");
  //PRINT_VAR(solver.get_feasible_region_limits());
  //solver.add_constr("x1", "equal_to_const", 10);

  PRINT_VAR(solver);
  PRINT_VAR(solver.get_feasible_region_limits());
}
