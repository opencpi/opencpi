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
#include <assert.h>
#include <unistd.h>
#include "ValueReader.h"
#include "ValueWriter.h"
#include "OcpiUtilProtocol.h"
#include "OcpiUtilValue.h"
#include "ocpidds.h"

namespace OU = OCPI::Util;
namespace OA = OCPI::API;

void dataTypeTest(const char *arg) {
  unsigned count;
  OU::Protocol pp, *ppp;
  if (isdigit(*arg)) {
    count = atoi(arg);
    ppp = NULL;
  } else {
    char buf[10000];
    int fd = open(arg, O_RDONLY);
    if (fd < 0) {
      fprintf(stderr, "Can't open %s\n", arg);
      return;
    }
    ssize_t nread = read(fd, buf, sizeof(buf));
    if (nread <= 0 || nread >= (ssize_t)sizeof(buf)) {
      fprintf(stderr, "Can't read file: %s\n", arg);
      return;
    }
    buf[nread] = 0;
    pp.parse(buf);
    ppp = &pp;
    count = 1;
  }
  size_t maxSize = 0;
  std::vector<uint8_t> buf, buf1;
  for (unsigned c = 0; c < count; c++) {
    OU::Protocol genp;
    OU::Protocol &p = ppp ? *ppp : genp;
    printf("=== Test %u of %u ===\n", c+1, count);
    if (0 == (c+1)%10000)
      fprintf(stderr, "=== Test %u of %u ===\n", c+1, count);
    if (!ppp)
      p.generate("test");
    std::string out;
    p.printXML(out);
    fputs(out.c_str(), stdout);
    OU::Value **v;
    uint8_t opcode = 0;
    p.generateOperation(opcode, v);
    p.printOperation(stdout, opcode, v);
    p.testOperation(stdout, opcode, v, false);
    p.testOperation(stdout, opcode, v, true);
    printf("Min Buffer Size: %zu %zu %zu\n", p.m_minBufferSize, p.m_dataValueWidth, p.m_minMessageValues);
    fflush(stdout);
    size_t len;
    {
      OU::ValueReader r((const OU::Value **)v);
      len = p.read(r, NULL, SIZE_MAX, opcode);
    }
    maxSize = std::max(len, maxSize);
    printf("Length is %zu\n", len);
    if (!len)
      continue;
    buf.resize(len);
    OU::ValueReader r((const OU::Value **)v);
    size_t rlen = p.read(r, &buf[0], len, opcode);
    printf("Length was %zu\n", rlen);
    size_t nArgs = p.m_operations[opcode].m_nArgs;
    OU::Value **v1 = new OU::Value *[nArgs];
    OU::ValueWriter w(v1, nArgs);
    p.write(w, &buf[0], len, opcode);
    buf1.resize(rlen, 0);
    OU::ValueReader r1((const OU::Value **)v1);
    size_t rlen1 = p.read(r1, &buf1[0], rlen, opcode);
    assert(rlen == rlen1);
    int dif = memcmp(&buf[0], &buf1[0], rlen);
    if (dif)
      for (unsigned n = 0; n < rlen; n++)
	if (buf[n] != buf1[n]) {
	  printf("Buffer differs at byte %u [%llx %llx]\n",
		 n, (long long unsigned)&buf[n], (long long unsigned)&buf1[n]);
	  assert("buffers different"==0);
	}
    for (unsigned n = 0; n < nArgs; n++) {
      delete v[n];
      delete v1[n];
    }
    delete []v;
    delete []v1;
  }
  fprintf(stderr, "Data type test succeeded with %u randomly generated types and values."
	  " Max buffer was %zu\n", count, maxSize);
}

