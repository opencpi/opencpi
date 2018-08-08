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

#ifndef _AD9361_COMMON_H
#define _AD9361_COMMON_H

enum class AD9361_duplex_mode_t {FDD, TDD};
enum class AD9361_RX_port_t {RX1A, RX2A, RX1B, RX2B, RX1C, RX2C};
enum class AD9361_rx_rf_port_input_t {A_BALANCED, B_BALANCED, C_BALANCED, A_N, A_P, B_N, B_P, C_N, C_P};
enum class AD9361_RX_timing_diagram_channel_t {R1, R2};
enum class AD9361_TX_timing_diagram_channel_t {T1, T2};
typedef AD9361_RX_timing_diagram_channel_t AD9361_one_rx_one_tx_mode_use_rx_num_t;

#endif // _AD9361_COMMON_H
