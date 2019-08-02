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

// This is a prerequisite so that users of it do not depend on the framework being built
#include <string.h>
#include <stdlib.h>
#include <errno.h>

int gethostname(char *name, size_t len) {
  const char *myhostname = getenv("MYHOSTNAME");
  if (!myhostname || len <= (strlen(myhostname)+1))
    return EINVAL;
  strncpy(name, myhostname, len);
  return 0;
}
