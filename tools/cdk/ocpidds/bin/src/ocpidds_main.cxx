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
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>
#include <vector>
#include "cdkutils.h"
#include "ocpidds.h"
#include "OcpiUtilProtocol.h"
#include "OcpiUtilMisc.h"
#include "ValueReader.h"

#define OCPI_OPTIONS_HELP \
  "Usage is: ocpidds [options] <input-files>\n"

//          name      abbrev  type    value description
#define OCPI_OPTIONS \
 CMD_OPTION(protocol, p, Bool,   NULL, "Generate the protocol XML file from  DDS IDL files") \
 CMD_OPTION(idl,      d, Bool,   NULL, "Generate the DDS IDL file from an XML protocol file") \
 CMD_OPTION(structname, s, String, NULL,  "IDL struct name for DDS topic for -p. Default is IDL file name") \
 CMD_OPTION(test,       t, String,  0,     "Internal data type test iteration count") \
 CMD_OPTION(output,     O, String, NULL,  "the output directory for generated files") \
 CMD_OPTION_S(define,   D, String, NULL,  "preprocessor definition for IDL") \
 CMD_OPTION_S(undefine, U, String, NULL, "preprocessor undefinition for IDL") \
 CMD_OPTION_S(include,  I, String, NULL, "an include search directory for IDL/XML processing") \
 CMD_OPTION(depend,     M, String, NULL, "file to write makefile dependencies to") \
 CMD_OPTION(in,         i, String, NULL, "input file to read protocol data from") \
 CMD_OPTION(out,        o, String, NULL, "output file to write protocol data to") \

#include "CmdOption.h"

namespace OU = OCPI::Util;
namespace OA = OCPI::API;

static const char *
parseAndWrite(OU::Protocol &p, OU::Operation &op, uint8_t opcode, const char *text, int out) {
  struct {
    uint32_t length;
    uint32_t opcode;
  } msg;
  msg.opcode = opcode;
  msg.length = 0;
  struct iovec iov[2];
  iov[0].iov_base = &msg;
  iov[0].iov_len = sizeof(msg);
  std::vector<uint8_t> data;
  const char *err;

  if (op.nArgs()) {
    std::vector<OU::Value *> values(op.nArgs(), NULL);
    const OU::Value **v;
    if (op.nArgs() == 1) {
      values[0] = new OU::Value(*op.args());
      values[0]->parse(text);
      v = (const OU::Value **)&values[0];
    } else {
      size_t maxAlign = 1, minSize = 0, myOffset = 0;
      bool diverseSizes, unBounded, variable, isSub32; // we are precluding unbounded in any case

      // FIXME:  elide structures and operations
      OU::Member m(op.cname(), NULL, NULL, OA::OCPI_Struct, false, NULL);
      m.m_members = op.args();
      m.m_nMembers = op.nArgs();
      if ((err = m.offset(maxAlign, myOffset, minSize, diverseSizes, isSub32, unBounded, 
			  variable))) {
	m.m_members = 0;
	m.m_nMembers = 0;
	return err;
      }
      OU::Value sv(m);
      err = sv.parse(text);
      m.m_members = 0;
      m.m_nMembers = 0;
      if (err)
	return err;
      v = (const OU::Value **)sv.m_struct;
      size_t dataLen;
      {
	OU::ValueReader r(v);
	dataLen = p.read(r, NULL, SIZE_MAX, opcode);
      }      
      data.resize(dataLen);
      OU::ValueReader r(v);
      ocpiCheck(dataLen == p.read(r, &data[0], dataLen, opcode));
      iov[1].iov_base = &data[0];
      iov[1].iov_len = dataLen;
      msg.length = (uint32_t)dataLen;
    }
  }
  if (writev(out, iov, msg.length ? 2 : 1) != (ssize_t)(iov[0].iov_len + iov[1].iov_len))
    return OU::esprintf("error writing output file: %s", strerror(errno));
  return NULL;
}

static const char *
emitData(const char *protoFile, const char *input, const char *output) {
  OU::Protocol p;
  ezxml_t x;
  std::string dummy;
  const char *err = NULL;
  if ((err = parseFile(protoFile, dummy, "protocol", &x, dummy, false)) ||
      (err = p.parse(x, NULL, NULL, NULL, NULL)))
    return err;
  FILE *in;
  int  out;
  if (!strcmp(input, "-"))
    in = stdin;
  else if (!(in = fopen(input, "r")))
    return OU::esprintf("input file \"%s\" cannot be opened", input);
  if ((out = open(output, O_WRONLY|O_TRUNC|O_CREAT, 0666)) < 0)
    return OU::esprintf("output file \"%s\" cannot be opened (%s)", output, strerror(errno));
  size_t len = 0;
  char *line = NULL;
  ssize_t n;
  for (size_t lines = 1; (n = getline(&line, &len, in)) > 0 && line[n-1] == '\n'; lines++) {
    line[n-1] = '\0';
    char *text = line;
    OU::Operation *op;
    if (n >= 4 && !strncmp("\\:", line, 2)) {
      char *end = strchr(line + 2, ':');
      if (end) {
	err = OU::esprintf("invalid opcode at start of input line %zu", lines);
	break;
      }
      text = end + 1;
      *end = '\0';
      op = p.findOperation(line+2);
      if (!op) {
	err = OU::esprintf("invalid or unknown opcode \"%s\" at start of input line %zu",
			   line + 2, lines);
	break;
      }
    } else
      op = p.operations();
    if ((err = parseAndWrite(p, *op, (uint8_t)(op - p.operations()), text, out)))
      break;
  }
  if (!err && n > 0)
    err = OU::esprintf("missing newline character at end of input");
  fclose(in);
  if (close(out))
    err = OU::esprintf("error closing output file \"%s\": %s", output, strerror(errno));
  if (line)
    free(line);
  return err;
}

/*
 * Generate things related to DDS.
 * In particular, generate the OpenCPI protocol XML from DDS IDL
 */
static int mymain(const char **argv) {
  const char *err = 0;
  if (options.protocol()) {
    size_t n;
    std::vector<const char *> args;
    for (const char **ap = options.define(n); *ap; ++ap)
      args.push_back(*ap);
    for (const char **ap = options.undefine(n); *ap; ++ap)
      args.push_back(*ap);
    for (const char **ap = options.include(n); *ap; ++ap)
      args.push_back(*ap);
    args.push_back(NULL);
    if ((err = emitProtocol(&args[0], options.output(), *argv, options.structname())))
      fprintf(stderr, "Error generating OpenCPI protocol file from IDL: %s\n", err);
  } else if (options.idl() && (err = emitIDL(options.output(), *argv)))
    fprintf(stderr, "Error generating IDL file from OpenCPI protocol file \"%s\": %s\n", *argv,
	    err);
  else if (options.test())
    dataTypeTest(options.test());
  else if (options.in() && (err = emitData(*argv, options.in(), options.out())))
    fprintf(stderr, "Error generating data file from OpenCPI protocol file \"%s\": %s\n", *argv,
	    err);
  return err ? 1 : 0;
}
