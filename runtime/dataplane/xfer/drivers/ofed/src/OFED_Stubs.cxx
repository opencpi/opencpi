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

// This file is a "stubs" file that becomes a .so that OpenCPI executables are linked against.
// At runtime, it is not present, but other libraries in the OFED infrastructure should
// handle the calls.

#include <stdexcept>
#include "verbs.h"

void ofed_stub_throw() {
    throw std::logic_error("Called stub function that should not be linked in!");
};

BEGIN_C_DECLS

int ibv_dereg_mr(struct ibv_mr *) { ofed_stub_throw(); return -1; };
struct ibv_pd *ibv_alloc_pd(struct ibv_context *) { ofed_stub_throw(); return NULL; };
int ibv_close_device(struct ibv_context *) { ofed_stub_throw(); return -1; };
int ibv_dealloc_pd(struct ibv_pd *) { ofed_stub_throw(); return -1; };
int ibv_destroy_qp(struct ibv_qp *) { ofed_stub_throw(); return -1; };
int ibv_modify_qp(struct ibv_qp *, struct ibv_qp_attr *, int) { ofed_stub_throw(); return -1; };
struct ibv_cq *ibv_create_cq(struct ibv_context *, int, void *, struct ibv_comp_channel *, int) { ofed_stub_throw(); return NULL; };
struct ibv_context *ibv_open_device(struct ibv_device *) { ofed_stub_throw(); return NULL; };
struct ibv_qp *ibv_create_qp(struct ibv_pd *, struct ibv_qp_init_attr *) { ofed_stub_throw(); return NULL; };
struct ibv_mr *ibv_reg_mr(struct ibv_pd *, void *, size_t, int) { ofed_stub_throw(); return NULL; };
struct ibv_device **ibv_get_device_list(int *) { ofed_stub_throw(); return NULL; };
const char *ibv_get_device_name(struct ibv_device *) { ofed_stub_throw(); return NULL; };
int ibv_query_gid(struct ibv_context *, uint8_t, int, union ibv_gid *) { ofed_stub_throw(); return -1; };
int ibv_destroy_cq(struct ibv_cq *) { ofed_stub_throw(); return -1; };

END_C_DECLS

namespace DataTransfer {
    namespace OFED {
        int * __errno_location() { ofed_stub_throw(); return NULL; };
    }
}
