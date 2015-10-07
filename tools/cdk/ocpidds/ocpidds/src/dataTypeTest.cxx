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
  for (unsigned n = 0; n < count; n++) {
    OU::Protocol genp;
    OU::Protocol &p = ppp ? *ppp : genp;
    if (!ppp)
      p.generate("test");
    p.printXML(stdout);
    OU::Value **v;
    uint8_t opcode = 0;
    p.generateOperation(opcode, v);
    p.printOperation(stdout, opcode, v);
    p.testOperation(stdout, opcode, v);
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
