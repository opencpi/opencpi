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

#include "RadioCtrlrNoOSTuneResamp.hh"
#include "RadioCtrlrFMCOMMS3TuneResamp.hh"
#include "OcpiApi.hh" // OCPI::API::

extern "C" {
#include "ad9361.h" // struct ad9361_rf_phy
}

namespace OCPIProjects {

namespace RadioCtrlr {

namespace OA = OCPI::API;

template<class S>
RadioCtrlrFMCOMMS3TuneResamp<S>::RadioCtrlrFMCOMMS3TuneResamp(
    const char* descriptor, S& slave, OA::Application& app) :
    RadioCtrlrNoOSTuneResamp<configurator_t, S>(descriptor,slave, app) {
}

template<class S>
RadioCtrlrFMCOMMS3TuneResamp<S>::RadioCtrlrFMCOMMS3TuneResamp(
    const char* descriptor, S& slave, OA::Application& app,
    struct ad9361_rf_phy*& ad9361_rf_phy) :
    RadioCtrlrNoOSTuneResamp<configurator_t, S>(descriptor, slave, app, ad9361_rf_phy) {
}

} // namespace RadioCtrlr

} // namespace OCPIProjects
