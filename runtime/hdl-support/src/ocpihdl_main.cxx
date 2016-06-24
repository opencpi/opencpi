/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <inttypes.h>
#include <errno.h>
#include <signal.h> // for SIGPIPE
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include "lzma.h"
#include "zlib.h"
#include "OcpiOsMisc.h"
#include "OcpiUuid.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#include "OcpiUtilImplementation.h"
#include "DtTransferInternal.h"
#include "HdlSimServer.h"
#include "HdlDriver.h"
#include "HdlContainer.h"
#include "HdlOCDP.h"

#define USE_LZMA 1
namespace OX = OCPI::Util::EzXml;
namespace OE = OCPI::OS::Ether;
namespace HE = OCPI::HDL::Net;
namespace OH = OCPI::HDL;
namespace OU = OCPI::Util;
namespace OA = OCPI::API;
namespace OD = OCPI::DataTransport;
namespace OC = OCPI::Container;
namespace OS = OCPI::OS;
namespace DT = DataTransfer;
/*
  usage message
  verbose
  set log level
  subsume others
  swtcl
  dump all workers: with metadata?
  abbreviated parsers - pci and ether
  tpci - check for PCI access
  ethertest functions - emulator, list
  reset
  jtag load
  flash load
  introduce bitstream to cache
  ocfrp_check: show more stuff like swctl
  perhaps "verb" syntax like git
  use gnu options?
  fix our options?
 */
typedef void Function(const char **ap);
static Function
search, emulate, ethers, probe, testdma, admin, bram, unbram, uuid, reset, set, get, control,
  radmin, wadmin, rmeta, settime, deltatime, wdump, wreset, wunreset, wop, wwctl, wclear, wwpage,
  wread, wwrite, sendData, receiveData, receiveRDMA, sendRDMA, simulate, getxml, load, unload,
  status;
static bool verbose = false, parseable = false, hex = false, isPublic = false;
static int log = -1;
std::string platform, simExec;
static const char
*interface = NULL, *device = NULL, *part = NULL;
static bool simDump = true;
static unsigned
  sleepUsecs = 200000,  // how much time between quota injections
  spinCount = 20,      // how much quote per timeout AND per control message
  simTicks = 10000000;   // how much quota to run
static std::string name, error, endpoint;
static OH::Driver *driver;
static OH::Device *dev;
static OH::Access *cAccess, *dAccess, wAccess, confAccess;
static size_t worker;
static const unsigned
  WORKER = 1,
  DEVICE = 2,
  INTERFACE = 4,
  SUDO = 8,
  DISCOVERY = 16;
struct Command {
  const char *name;
  Function *func;
  unsigned options;
} commands [] = {
  { "admin", admin, DEVICE },
  { "bram", bram, 0 },
  { "control", control, DEVICE},
  { "deltatime", deltatime, DEVICE},
  { "emulate", emulate, SUDO | INTERFACE },
  { "ethers", ethers, INTERFACE},
  { "get", get, DEVICE},
  { "getxml", getxml, DEVICE},
  { "load", load, 0},
  { "unload", unload, DEVICE},
  { "probe", probe, SUDO | DEVICE | DISCOVERY},
  { "radmin", radmin, DEVICE },
  { "receiveData", receiveData, INTERFACE},
  { "receiveRDMA", receiveRDMA, 0}, // might want device depending on args
  { "reset", reset, DEVICE },
  { "rmeta", rmeta, DEVICE },
  { "search", search, SUDO | INTERFACE | DISCOVERY},
  { "sendData", sendData, INTERFACE},
  { "sendRDMA", sendRDMA, 0},  // might want device depending on args
  { "set", set, DEVICE},
  { "settime", settime, DEVICE},
  { "simulate", simulate, 0},
  { "status", status, DEVICE},
  { "testdma", testdma, 0},
  { "unbram", unbram, 0},
  { "uuid", uuid, 0},
  { "wadmin", wadmin, DEVICE},
  { "wclear", wclear, DEVICE|WORKER},
  { "wdump", wdump, DEVICE|WORKER},
  { "wop", wop, DEVICE|WORKER},
  { "wread", wread, DEVICE|WORKER},
  { "wreset", wreset, DEVICE|WORKER},
  { "wunreset", wunreset, DEVICE|WORKER},
  { "wwctl", wwctl, DEVICE|WORKER},
  { "wwpage", wwpage, DEVICE|WORKER},
  { "wwrite", wwrite, DEVICE|WORKER},
  { 0, 0, 0},
};

static int
usage(const char *name) {
  fprintf(stderr,
	  "Usage is: %s <command> [<options>...] [<command args>...]\n"
          "  Major commands/modes:\n"
	  "    search [-i <interface>]      # search for OpenCPI HDL devices, limit ethernet to <interface>\n"
	  "    emulate [-i <interface>]     # emulate an HDL device on ethernet, on first or specified interface\n"
          "    ethers                       # list available ethernet interfaces\n"
          "    probe <hdl-dev>              # see if a specific HDL device is available\n"
          "    get [<instance> [<property]] # get info from bitstream\n"
          "    set <instance> <property> <value>\n"
	  "                                 # set property value for worker instance\n"
          "    control <instance> <operation>\n"
	  "                                 # perform reset, unreset, or control operation\n"
          "    status <instance>            # show status of worker/instance\n"
          "    testdma                      # test for DMA memory setup\n"
          "    admin <hdl-dev>              # dump admin information (reading only) for <hdl-device>\n"
          "    wadmin <hdl-dev> <offset> <value>\n"
	  "                                 # write admin word <value> for <hdl-device> at <offset>\n"
          "    radmin <hdl-dev> <offset>    # read admin word for <hdl-device> at <offset>\n"
	  "    settime <hdl-dev>            # set the GPS time of the device to syste time\n"
	  "    deltatime <hdl-dev>          # measure round trip and difference between host and device\n"
          "    dump <hdl-dev>               # dump all state/status of <platform> including all workers\n"
          "    reset <hdl-dev>              # reset platform\n"
          "    flash <hdl-dev>              # flash load platform\n"
	  "    bram <infile> <outfile>      # create a BRAM file from in input file\n"
	  "    unbram <infile> <outfile>    # recreate the original file from a BRAM file\n"
	  "    uuid -p <platform> -c <part> <outputfilename>\n"
	  "    wclear <hdl-dev> <worker>    # clear worker status errors\n"
	  "    wdump <hdl-dev> <worker>     # dump worker's control plane registers\n"
	  "    wop <hdl-dev> <worker> <op>  # perform control operation on worker\n"
	  "          ops are: initialize, start, stop, release, after, before\n"
	  "    wread <hdl-dev> <worker> <offset>[/size]\n"
	  "                                 # perform config space read of size bytes (default 4) at offset\n"
	  "    wreset <hdl-dev> <worker>    # assert reset for worker\n"
	  "    wunreset <hdl-dev> <worker>  # deassert (enable) worker\n"
	  "    wwctl <hdl-dev> <worker> <val>\n"
	  "                                 # write worker control register\n"
	  "    wwpage <hdl-dev> <worker> <val>\n"
	  "                                 # write worker window register\n"
	  "    wwrite <hdl-dev> <worker> <offset>[/size] <value>"
	  "                                 # perform config space write of size bytes (default 4) at offset\n"
	  "                                 # generate UUID verilog file\n"
	  "    load  <hdl-dev> <file>       # load bitstream from file\n"
	  "    unload  <hdl-dev>            # revert device to unloaded state: no bitstream\n"
          "    getxml <hdl-dev> <file>      # Extract the xml metadata from the device into the file\n"
          "    simulate                     # run simulator inside created sim: device\n"
          "  Options: (values are either directly after the letter or in the next argument)\n"
	  "    -l <level>                   # set log levels\n"
	  "    -i <interface>               # set ethernet interface to use\n"
	  "    -d <hdl-device>              # identify a specific hdl device to use\n"
	  "    -p <hdl-platform>            # specify a particular hdl platform (e.g. ml605)\n"
          "    -c <hdl-part>                # specify a particular part/chip (e.g. xc6vlx240t\n"
	  "    -s <spin-clocks>             # clocks to credit/run the sim between control messages\n"
	  "    -t <sleep-usecs>             # delay time letting sim run between credits\n"
	  "    -T <sim-time>                # total simulation time before terminating\n"
	  "    -D                           # turn off simulation dumping\n"
	  "    -e <sim-executable>          # simulator executable \"bitstream\" file\n"
	  "    -A                           # make the simulator publically available on the LAN\n"
	  "    -v                           # be verbose\n"
	  "    -x                           # print numeric values in hex rather than decimal\n"
	  "  <worker> can be multiple workers such as 1,2,3,4,5.  No ranges.\n"
	  "  <hdl-dev> examples: 3                            # PCI device 3 (i.e. 0000:03:00.0)\n"
	  "                      0000:04:00.0                 # PCI device 0000:04:00.0\n"
          "                      PCI:0001:05:04.2             # fully specified PCI device\n"
	  "                      a0:00:b0:34:55:67            # ethernet MAC address on first up+connected interface\n"
	  "                      eth0/a0:00:b0:34:55:67       # ethernet address on a specific interface\n"
	  "                      Ether:eth1/a0:00:b0:34:55:67 # fully specified Ethernet-based device\n",
	  name);
  return 1;
}

static void exitbad(const char *e) {
  fprintf(stderr, "%s\n", e);
  exit(1);
}

static void
bad(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  std::string e = "Exiting for problem: ";
  OU::formatAddV(e, fmt, ap);
  if (error.size())
    OU::formatAdd(e, ": %s", error.c_str());
  va_end(ap);
  throw e;
}

static void setupDevice(bool discovery) {
  if (!device)
    bad("a device option is required with this command");
  driver = &OCPI::HDL::Driver::getSingleton();
  if (!(dev = driver->open(device, discovery, false, NULL, error)))
    bad("error opening device %s", device);
  cAccess = &dev->cAccess();
  dAccess = &dev->dAccess();
  name = dev->name();
  endpoint = dev->endpointSpecific();
}

static const char *
next(const char **&ap) {
  char letter = ap[0][1];
  const char *next = ap[0][2] ? &ap[0][2] : *++ap;
  if (!next || !next[0])
    bad("Missing option value after -%c flag", letter);
  return next;
}

static void
doFlags(const char **&ap) {
  for (; *ap && ap[0][0] == '-'; ap++)
    switch (ap[0][1]) {
    case 'i':
      //      if (!(exact->options & INTERFACE))
      //	bad("An interface cannot be specified with this command");
      interface = next(ap);
      break;
    case 'd':
      //      if (!(exact->options & DEVICE))
      //	bad("An interface cannot be specified with this command");
      device = next(ap);
      break;
    case 'e':
      simExec = next(ap);
      break;
    case 'p':
      platform = next(ap);
      break;
    case 'A':
      isPublic = true;
      break;
    case 'c':
      part = next(ap);
      break;
    case 'l':
      log = atoi(next(ap));
      break;
    case 's':
      spinCount = atoi(next(ap));
      break;
    case 't':
      sleepUsecs = atoi(next(ap));
      break;
    case 'T':
      simTicks = atoi(next(ap));
      break;
    case 'D':
      simDump = false;
      break;
    case 'v':
      verbose = true;
      break;
    case 'x':
      hex = true;
      break;
    case 'P':
      parseable = true;
      break;
    default:
      bad("unknown flag: %s", *ap);
    }
}

int
main(int argc, const char **argv)
{
  signal(SIGPIPE, SIG_IGN);
  try {
    OCPI::Driver::ManagerManager::suppressDiscovery();
    const char *argv0 = strrchr(argv[0], '/');
    if (argv0)
      argv0++;
    else
      argv0 = argv[0];
    if (argc == 1)
      return usage(argv0);
    argv++;
    doFlags(argv);
    Command *found = NULL, *exact = NULL;
    bool ambiguous = false;
    if (!argv[0])
      bad("Missing command name");
    for (Command *c = commands; c->name; c++)
      if (!strncmp(argv[0], c->name, strlen(argv[0]))) {
	if (!strcmp(argv[0], c->name)) {
	  exact = c;
	  break;
	} else if (found)
	  ambiguous = true;
	else
	  found = c;
      }
    if (!exact) {
      if (!found)
	bad("unknown command: %s", argv[0]);
      else if (ambiguous)
	bad("ambiguous command: %s", argv[0]);
      else
	exact = found;
    }
    argv++;
    doFlags(argv);
    if (log != -1)
      OCPI::OS::logSetLevel(log);
#if 0
    if ((exact->options & SUDO) && geteuid()) {
      int dfd = ::open(OCPI_DRIVER_MEM, O_RDONLY);
      if (dfd >= 0)
	close(dfd);
      else
	bad("This command requires running with \"sudo -E ./%s ...\"", argv0);
    }
#endif
    if (!exact->func)
      bad("command %s not implemented", exact->name);
    if (exact->options & DEVICE) {
      if (!device &&
	  !(device = getenv("OCPI_DEFAULT_HDL_DEVICE")) &&
	  *argv)
	device = *argv++;
      setupDevice((exact->options & DISCOVERY) != 0);
    }
    if (exact->options & WORKER) {
      if (!*argv)
	bad("Missing worker specifier for command");
      const char *cp = *argv++;
      char *ep;
      do {
	worker = strtoul(cp, &ep, 10);
	if (worker > OCCP_MAX_WORKERS)
	  bad("Worker number `%s' invalid", cp);
	if (*ep)
	  ep++;
	cAccess->offsetRegisters(wAccess, (intptr_t)(&((OH::OccpSpace*)0)->worker[worker]));
	cAccess->offsetRegisters(confAccess, (intptr_t)(&((OH::OccpSpace*)0)->config[worker]));
	exact->func(argv);
      } while (*(cp = ep));
    } else
      exact->func(argv);
  } catch (std::string &e) {
    exitbad(e.c_str());
  } catch (const char *e) {
    exitbad(e);
  } catch (...) {
    exitbad("Unexpected exception");
  }
  return 0;
}
static void search(const char **) {
#if 0
  OA::PValue vals[] = {
    OA::PVBool("printOnly", true),
    interface ? OA::PVString("interface", interface) : OA::PVEnd,
    OA::PVEnd};
#else
    OA::PVarray vals(5);
    unsigned n = 0;
    vals[n++] = OA::PVBool("printOnly", true);
    if (interface)
      vals[n++] = OA::PVString("interface", interface);
    vals[n++] = OA::PVEnd;
#endif

    OCPI::HDL::Driver::getSingleton().search(vals, NULL, true);
}
static void probe(const char **) {
  dev->print();
}


static const char *ops[] =
  { "initialize", "start", "stop", "release", "test", "before", "after", "reserved7", 0};

static void emulate(const char **) {
  OE::IfScanner ifs(error);
  if (error.size())
    bad("Error establishing interface scanner");
  OE::Interface eif;
  while (error.empty() && ifs.getNext(eif, error, interface)) {
    if (eif.up && eif.connected) {
      OE::Socket s(eif, ocpi_slave, NULL, 0, error);
      if (error.size())
	bad("Failed to open slave socket");
      printf("Using interface %s with address %s\n", eif.name.c_str(), s.ifAddr().pretty());
      OE::Packet rFrame, sFrame;
      static char cadmin[sizeof(OH::OccpSpace)];
      memset(cadmin, 0, sizeof(cadmin));
      OH::Sim::Server::initAdmin(*(OH::OccpAdminRegisters *)cadmin, "emulator",
				 *(OH::HdlUUID *)(cadmin + offsetof(OH::OccpSpace, config)));
      HE::EtherControlHeader &ech_out =  *(HE::EtherControlHeader *)(sFrame.payload);
      bool haveTag = false;
      OE::Address to;
      do {
	size_t length;
	OE::Address from;
	if (s.receive(rFrame, length, 0, from, error)) {
	  if (from == eif.addr)
	    continue;
	  bool sent;
	  HE::EtherControlHeader &ech_in =  *(HE::EtherControlHeader *)(rFrame.payload);
	  HE::EtherControlMessageType type = OCCP_ETHER_MESSAGE_TYPE(ech_in.typeEtc);
	  unsigned uncache = OCCP_ETHER_UNCACHED(ech_in.typeEtc) ? 1 : 0;
	  OE::Packet &out = uncache ? rFrame : sFrame;
	  HE::EtherControlHeader *echp = (HE::EtherControlHeader *)out.payload;
	  HE::EtherControlRead &ecr =  *(HE::EtherControlRead *)(rFrame.payload);
	  uint32_t offset = ntohl(ecr.address); // for read or write
	  HE::EtherControlWrite &ecw =  *(HE::EtherControlWrite *)(rFrame.payload);
	  ocpiDebug("Received type %u, length 0x%x ntohs 0x%x tag %3d from %s, offset 0x%8" PRIx32
		 ", data 0x%8" PRIx32 "\n",
		 type, ech_in.length, ntohs(ech_in.length), ech_in.tag, from.pretty(), offset,
		 ntohl(ecw.data));
	  switch (type) {
	  case HE::OCCP_READ:
	  case HE::OCCP_WRITE:
	    if (uncache || !haveTag || ech_in.tag != ech_out.tag || from != to) {
	      if (!uncache) {
		to = from;
		haveTag = true;
		echp->tag = ech_in.tag;
	      }
	      if (type == HE::OCCP_READ) {
		ocpiAssert(ntohs(ech_in.length) == sizeof(ecr)-2);
		HE::EtherControlReadResponse &ecrr =  *(HE::EtherControlReadResponse *)(echp);
		ecrr.data = htonl(*(uint32_t *)&cadmin[offset]);
		if (offset > sizeof(cadmin)) {
		  ocpiBad("Read offset out of range: 0x%" PRIx32 ", returning 0\n", offset);
		  ecrr.data = 0;
		} else if (offset >= offsetof(OH::OccpSpace, config)) {
		  size_t
		    worker = (offset - offsetof(OH::OccpSpace, config)) / OCCP_WORKER_CONFIG_SIZE,
		    woffset = (offset - offsetof(OH::OccpSpace, config)) % OCCP_WORKER_CONFIG_SIZE;
		  printf("Worker %2zd config read 0x%zx\n", worker, woffset);
		  if (worker == 13 && woffset == 0x4c)
		    ecrr.data = htonl(0x8000);
		} else if (offset >= offsetof(OH::OccpSpace, worker)) {
		  size_t
		    worker = (offset - offsetof(OH::OccpSpace, worker)) / sizeof(OH::OccpWorker),
		    woffset = (offset - offsetof(OH::OccpSpace, worker)) % sizeof(OH::OccpWorker);
		  if (woffset < offsetof(OH::OccpWorkerRegisters, control)) {
		    printf("Worker %2zd control operation read: %s\n", worker, ops[woffset/sizeof(uint32_t)]);
		    ecrr.data = htonl(OCCP_SUCCESS_RESULT);
		  } else
		    printf("Worker %2zd register read: %s\n", worker, 
			   woffset == offsetof(OH::OccpWorkerRegisters, control) ? "control" :
			   woffset == offsetof(OH::OccpWorkerRegisters, window) ? "window" :
			   woffset == offsetof(OH::OccpWorkerRegisters, clearError) ? "clearError" :
			   woffset == offsetof(OH::OccpWorkerRegisters, lastConfig) ? "lastConfig" :
			   "unknown");
		}
		echp->length = htons((short)(sizeof(ecrr)-2));
	      } else {
		HE::EtherControlWrite &ecw =  *(HE::EtherControlWrite *)(rFrame.payload);
		ocpiAssert(ntohs(ech_in.length) == sizeof(ecw)-2);
		uint32_t data = ntohl(ecw.data);
		if (offset > sizeof(cadmin)) {
		  ocpiDebug("Write offset out of range: 0x%" PRIx32 "\n", offset);
		} else if (offset > offsetof(OH::OccpSpace, config)) {
		  size_t
		    worker = (offset - offsetof(OH::OccpSpace, config)) / OCCP_WORKER_CONFIG_SIZE,
		    woffset = (offset - offsetof(OH::OccpSpace, config)) % OCCP_WORKER_CONFIG_SIZE;
		  printf("Worker %2zd config write 0x%zx: 0x%" PRIx32 "\n", worker, woffset, data);
		  *(uint32_t *)&cadmin[offset] = data;
		} else if (offset > offsetof(OH::OccpSpace, worker)) {
		  size_t
		    worker = (offset - offsetof(OH::OccpSpace, worker)) / sizeof(OH::OccpWorker),
		    woffset = (offset - offsetof(OH::OccpSpace, worker)) % sizeof(OH::OccpWorker);
		  if (woffset < offsetof(OH::OccpWorkerRegisters, control)) {
		    printf("Worker %2zd control operation write???: %s\n",
			   worker, ops[woffset/sizeof(uint32_t)]);
		  } else {
		    printf("Worker %2zd register write of 0x%" PRIx32 " to %s\n", worker, data,
			   woffset == offsetof(OH::OccpWorkerRegisters, control) ? "control" :
			   woffset == offsetof(OH::OccpWorkerRegisters, window) ? "window" :
			   woffset == offsetof(OH::OccpWorkerRegisters, clearError) ? "clearError" :
			   woffset == offsetof(OH::OccpWorkerRegisters, lastConfig) ? "lastConfig" :
			   "unknown");
		    *(uint32_t *)&cadmin[offset] = data;
		  }
		} else
		  *(uint32_t *)&cadmin[offset] = data;
		HE::EtherControlWriteResponse &ecwr =  *(HE::EtherControlWriteResponse *)(echp);
		echp->length = htons((short)(sizeof(ecwr)-2));
	      }
	      // Modify the outgoing packet last since we might be doing it in the
	      // received buffer (for uncached mode).
	      echp->typeEtc = OCCP_ETHER_TYPE_ETC(HE::OCCP_RESPONSE, HE::OK, uncache, 0);
	      ocpiDebug("Sending read/write response packet: length is htons %x, ntohs %u tag %u",
			echp->length, ntohs(echp->length), echp->tag);
	    }
	    sent = s.send(out, ntohs(echp->length) + 2, from, 0, NULL, error);
	    break;
	  case HE::OCCP_NOP:
	    if (OCCP_ETHER_RESERVED(ech_in.typeEtc)) {
	      ocpiInfo("Received reserved command with lenth: %zu",
		       ntohs(ech_in.length) - sizeof(HE::EtherControlHeader));
	      ech_in.length = htons((short)(sizeof(HE::EtherControlHeader)-2));
	      ech_in.typeEtc = OCCP_ETHER_TYPE_ETC(HE::OCCP_RESPONSE, HE::OK, 1, 0);
	    } else {
	      HE::EtherControlNop &ecn =  *(HE::EtherControlNop *)(rFrame.payload);
	      ocpiDebug("Received NOP: sizeof h %zd sizeof nop %zd offset %zd from %s\n",
			sizeof(HE::EtherControlHeader), sizeof(HE::EtherControlNop),
			offsetof(HE::EtherControlNop, mbx80), from.pretty());
	      ocpiAssert(ntohs(ech_in.length) == sizeof(ecn)-2);
	      HE::EtherControlNopResponse &ecnr =  *(HE::EtherControlNopResponse *)(rFrame.payload);
	      // Tag is the same
	      ech_in.length = htons((short)(sizeof(ecnr)-2));
	      ech_in.typeEtc = OCCP_ETHER_TYPE_ETC(HE::OCCP_RESPONSE, HE::OK, uncache, 0);
	      ecnr.mbx40 = 0x40;
	      ecnr.mbz0 = 0;
	      memcpy(ecnr.mac, OU::getSystemAddr().addr(), OS::Ether::Address::s_size);
	      ecnr.pid = getpid();
	      ocpiDebug("Sending nop response packet: length is sizeof %zu, htons %u, ntohs %u",
			sizeof(HE::EtherControlNopResponse), ech_in.length, ntohs(ech_in.length));
	    }
	    sent = s.send(rFrame, ntohs(ech_in.length)+2, from, 0, NULL, error);
	    break;
	  default:
	    sent = false; // warning
	    bad("Invalid control packet type: typeEtc = 0x%x", ech_in.typeEtc);
	  }
	  if (sent)
	    ocpiDebug("response sent ok to %s %x %x %x\n",
		   from.pretty(), echp->length, echp->typeEtc, echp->tag);
	  else
	    ocpiDebug("response send error: %s\n", error.c_str());
	} else
	  bad("Slave Recv failed");
      } while (1);
    } else if (interface)
      OU::format(error, "interface %s is %s and %s", interface,
		 eif.up ? "up" : "down",
		 eif.connected ? "connected" : "not connected");
  }	  
  if (error.size())
    bad("Error getting interface %s", interface ? interface : "that is up and connected");
}

static void
ethers(const char **) {
  OE::IfScanner ifs(error);
  if (error.size())
    bad("Error establishing interface scanner");
  OE::Interface eif;
  while (ifs.getNext(eif, error)) {
    printf("Interface %s: MAC address %s, %s, %s%s",
	   eif.name.c_str(), eif.addr.isEther() ? eif.addr.pretty() : "none",
	   eif.up ? "up" : "down", eif.connected ? "connected" : "disconnected",
	   eif.loopback ? ", loopback" : "");
    if (eif.ipAddr.addrInAddr())
      printf(", IP address: %s", eif.ipAddr.prettyInAddr());
    printf("\n");
  }
  printf("System identity is: %s\n", OU::getSystemId().c_str());
  if (error.size())
    bad("Error scanning for interfaces");
}
static void
testdma(const char **) {
  const char *dmaEnv;
  size_t dmaMeg;
  uint64_t dmaBase, dmaSize;
  int fd;
  volatile uint8_t *cpuBase = 0; // warning

  if (!(dmaEnv = getenv("OCPI_DMA_MEMORY")))
    bad("Warning: You must set the OCPI_DMA_MEMORY environment variable before using any OpenCPI FPGA device.\n"
	"         Use \"sudo -E\" to allow this program to have access to environmant variables");
  if (sscanf(dmaEnv, "%zuM$0x%" SCNx64, &dmaMeg, &dmaBase) != 2)
    bad("The OCPI_DMA_MEMORY environment variable is not formatted correctly");
  dmaSize = (uint64_t)dmaMeg * 1024 * 1024;
  printf("The OCPI_DMA_MEMORY environment variable indicates %zuMB at 0x%" PRIx64 ", ending at 0x%" PRIx64 "\n",
	 dmaMeg, dmaBase, dmaBase + dmaSize);
  int pageSize = getpagesize();
  if (dmaBase & (pageSize - 1))
    bad("DMA memory starting address 0x%" PRIx64 " does not start on a page boundary, %u (0x%x)", 
	dmaBase, pageSize, pageSize);
  if (dmaSize & (pageSize -1))
    bad("DMA memory size %" PRIu64 " (0x%" PRIx64 ") does not start on a page boundary, %u (0x%x)", 
	dmaSize, dmaSize, pageSize, pageSize);
  if ((fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0 ||
	     (cpuBase = (uint8_t*)mmap(NULL, dmaMeg * 1024 * 1024,
				       PROT_READ|PROT_WRITE, MAP_SHARED, fd, (off_t)dmaBase)) ==
	     MAP_FAILED)
    bad("Can't map to local DMA memory defined in OCPI_DMA_MEMORY using /dev/mem (%s/%d). Forgot sudo -E?",
	strerror(errno), errno);
  uint8_t save = *cpuBase; // warning

  if ((*cpuBase = 1, *cpuBase != 1) || (*cpuBase = 2, *cpuBase != 2)) {
    *cpuBase = save; // do this before any sys calls etc.
    bad("Can't write to start of local DMA memory defined in OCPI_DMA_MEMORY using /dev/mem");
  }
  *cpuBase = save; // on the wild chance that we shouldn't actually be touching this?
  cpuBase += dmaSize - 1ull;
  save = *cpuBase;
  if ((*cpuBase = 1, *cpuBase != 1) || (*cpuBase = 2, *cpuBase != 2)) {
    *cpuBase = save; // do this before any sys calls etc.
    bad("Can't write to end of local DMA memory defined in OCPI_DMA_MEMORY using /dev/mem");
  }
  *cpuBase = save; // on the wild chance that we shouldn't actually be touching this?
  printf("Successfully wrote and read back start and end of DMA memory\n");
}
static void
admin(const char **) {
  uint32_t i, k;
  uint64_t j;
  time_t epochtime;
  struct tm *etime, *ntime;
  static union {
    uint64_t uint;
    char c[sizeof(uint64_t) + 1];
  } u;

  epochtime = (time_t)cAccess->get32Register(birthday, OH::OccpAdminRegisters);

  etime = gmtime(&epochtime); 
  //printf("%lld%lld\n", (long long)x, (long long)y);
  printf("OCCP Admin Space\n");
  u.uint = cAccess->get64Register(magic, OH::OccpAdminRegisters);
  printf(" OpenCpi:      0x%016llx \"%s\"\n", (unsigned long long)u.uint, u.c);
  printf(" revision:     0x%08x\n", cAccess->get32Register(revision, OH::OccpAdminRegisters));
  printf(" birthday:     0x%08x %s", (uint32_t)epochtime, asctime(etime));
  printf(" workerMask:   0x%08" PRIx64 " workers", j = cAccess->get64Register(present, OH::OccpAdminRegisters));
  for (i = 0; i < sizeof(uint64_t) * 8; i++)
    if (j & (1 << i))
      printf(" %d", i);
  printf(" exist\n");
  printf(" pci_dev_id:   0x%08x\n", cAccess->get32Register(pciDevice, OH::OccpAdminRegisters));
  printf(" attention:    0x%08" PRIx64 "\n", cAccess->get64Register(attention, OH::OccpAdminRegisters));
  printf(" cpStatus:     0x%08x\n", cAccess->get32Register(status, OH::OccpAdminRegisters));
  printf(" scratch20:    0x%08x\n", cAccess->get32Register(scratch20, OH::OccpAdminRegisters));
  printf(" scratch24:    0x%08x\n", cAccess->get32Register(scratch24, OH::OccpAdminRegisters));
  printf(" cpControl:    0x%08x\n", cAccess->get32Register(control, OH::OccpAdminRegisters));

  //  nowtime = (time_t)(cAccess.get32Register(time, OH::OccpAdminRegisters)); // FIXME WRONG ENDIAN IN FPGA
  i = cAccess->get32Register(timeStatus, OH::OccpAdminRegisters);
  printf(" timeStatus:   0x%08x ", i);
  if(i & 0x80000000) printf("ppsLostSticky ");
  if(i & 0x40000000) printf("gpsInSticky ");
  if(i & 0x20000000) printf("ppsInSticky ");
  if(i & 0x10000000) printf("timeSetSticky ");
  if(i & 0x08000000) printf("ppsOK ");
  if(i & 0x04000000) printf("ppsLost ");
  printf("\n");
  printf(" timeControl:  0x%08x\n", cAccess->get32Register(timeControl, OH::OccpAdminRegisters));
  uint64_t gpsTime = cAccess->get64Register(time, OH::OccpAdminRegisters);
  //  cAccess.set64Register(timeDelta, OH::OccpAdminRegisters, gpsTime);
  uint32_t gpsTimeLS = (uint32_t)(gpsTime >> 32);
  uint32_t gpsTimeMS = (uint32_t)(gpsTime & 0xffffffffll);
  time_t gpsNow = gpsTimeMS;
  ntime = gmtime(&gpsNow); 
  uint64_t deltaTime = cAccess->get64Register(timeDelta, OH::OccpAdminRegisters);
  uint32_t deltaTimeLS = (uint32_t)(deltaTime >> 32);
  uint32_t deltaTimeMS = (uint32_t)(deltaTime & 0xffffffffll);
  uint32_t deltaTimeNS = (uint32_t)(((deltaTimeLS * 1000000000ull) + (1ull << 31)) / (1ull << 32));
  printf(" gpsTime:      0x%16llx (%llu)\n", (unsigned long long)gpsTime, (unsigned long long)gpsTime);
  printf(" gpsTimeMS:    0x%08x (%u) %s", gpsTimeMS,  gpsTimeMS, asctime(ntime));
  printf(" gpsTimeLS:    0x%08x (%u)\n",  gpsTimeLS,  gpsTimeLS);
  printf(" deltaTimeMS:  0x%08x (%u)\n", deltaTimeMS, deltaTimeMS);
  printf(" deltaTimeLS:  0x%08x (%u) (%uns)\n", deltaTimeLS, deltaTimeLS, deltaTimeNS);
  i = cAccess->get32Register(timeClksPerPps, OH::OccpAdminRegisters);
  printf(" refPerPPS:    0x%08x (%u)\n", i, i);
  i = cAccess->get32Register(readCounter, OH::OccpAdminRegisters);
  printf(" readCounter:  0x%08x (%u)\n", i, i);
  i = cAccess->get32Register(numRegions, OH::OccpAdminRegisters);
  printf(" numDPMemReg:  0x%08x (%u)\n", i, i);
  uint32_t regions[OCCP_MAX_REGIONS];
  cAccess->getRegisterBytes(regions, regions, OH::OccpAdminRegisters, 8, false);
  if (i < 16) 
    for (k=0; k<i; k++)
      printf("    DP%2d:      0x%08x\n", k, regions[k]);

  // Print out the 64B 16DW UUID in little-endian looking format...
  uint32_t uuid[16];
  cAccess->getRegisterBytes(uuid, uuid, OH::OccpAdminRegisters, 8, false);
  for (k=0;k<16;k+=4)
    printf(" UUID[%2d:%2d]:  0x%08x 0x%08x 0x%08x 0x%08x\n",
	   k+3, k, uuid[k+3], uuid[k+2], uuid[k+1], uuid[k]);
}
#ifndef USE_LZMA
static voidpf zalloc(voidpf , uInt items, uInt size) {
  return malloc(items * size);
}
static void zfree(voidpf , voidpf data) {
  free(data);
}
#endif
static void
bram(const char **ap) {
  if (!ap[0])
    bad("No input filename specified for bram");
  if (!ap[1])
    bad("No output filename specified for bram");
  unsigned char *in = NULL, *out = NULL;
  int fd = open(*ap, O_RDONLY);
  if (fd < 0)
    bad("Cannot open file: \"%s\"", *ap);
  FILE *ofp = fopen(ap[1], "w");
  if (!ofp)
    bad("Cannot open output file '%s'", ap[1]);
  off_t length = 0; // for warning
  if (fd < 0 ||
      (length = lseek(fd, 0, SEEK_END)) == -1 ||
      lseek(fd, 0, SEEK_SET) != 0 ||
      (in = (unsigned char*)malloc(length)) == NULL ||
      read(fd, in, length) != length ||
      (out = (unsigned char*)malloc(length)) == NULL)
    bad("Could not open or read the input file");
  size_t total, check;
#if USE_LZMA
  lzma_stream strm = LZMA_STREAM_INIT;
  lzma_ret lr;
  if ((lr = lzma_easy_encoder(&strm, 6, LZMA_CHECK_CRC64)) != LZMA_OK)
    bad("lzma_easy_encoder failed: %u", lr);
  strm.avail_in = strm.avail_out = length;
  strm.next_in = in;
  strm.next_out = out;
  if ((lr = lzma_code(&strm, LZMA_FINISH)) != LZMA_STREAM_END)
    bad("lzma didn't finish: %u", lr);
  total = OCPI_UTRUNCATE(size_t, strm.total_out);
  check = 0;
#else
  z_stream zs;
  int zr;
  zs.zalloc = zalloc;
  zs.zfree = zfree;
  zs.next_in = in;
  zs.avail_in = (uInt)length;
  zs.next_out = out;
  zs.avail_out = (uInt)length;
  zs.data_type = Z_TEXT;
  if ((zr = deflateInit(&zs, 9)) != Z_OK ||
      (zr = deflate(&zs, Z_FINISH)) != Z_STREAM_END ||
      (zr = deflateEnd(&zs)) != Z_OK)
    bad("zip failed: %u", zr);
  total = zs.total_out;
  check = zs.adler;
#endif
  size_t oWords = (total + OH::ROM_WIDTH_BYTES-1)/OH::ROM_WIDTH_BYTES;
  if (oWords + OH::ROM_HEADER_WORDS > OH::ROM_NWORDS)
    bad("Compressfile is too large for BRAM. Input is %zu, output is %zu",
	length, total + OH::ROM_HEADER_BYTES);
  OH::RomWord *u32p = (OH::RomWord *)out;
  for (unsigned n = 0; n < OH::ROM_NWORDS; n++) {
    OH::RomWord i;
    switch(n) {
    case 0: i = 1; break;
    case 1: i = OCPI_UTRUNCATE(OH::RomWord, total); break;
    case 2: i = OCPI_UTRUNCATE(OH::RomWord, length); break;
    case 3: i = OCPI_UTRUNCATE(OH::RomWord, check); break;
    default:
      i = n < oWords+OH::ROM_HEADER_WORDS ? *u32p++ : 0;
    }
    fprintf(ofp, "%08x\n", i);
  }
  if (fclose(ofp)) {
    unlink(ap[1]);
    bad("Error writing output file '%s'", ap[1]);
  }
  printf("Wrote bram file '%s' (%zu bytes) from file '%s' (%zu bytes)\n",
	 ap[1], total + OH::ROM_HEADER_BYTES, *ap, (size_t)length);
  if (in) free(in);
  if (out) free(out);
}

static void
unbram(const char **ap) {
  if (!*ap)
    bad("No input bram filename specified for unbram");
  if (!ap[1])
    bad("No output filename specified for unbram");
  FILE *ifp = fopen(ap[0], "r");
  if (!ifp)
    bad("Cannot open input file '%s'", ap[0]);
  FILE *ofp = fopen(ap[1], "wb");
  if (!ofp)
    bad("Cannot open output file: \"%s\"", ap[1]);
  size_t
    adler,
    olength = 16*OH::ROM_NBYTES, // worst case output size
    inWords = OH::ROM_NWORDS,
    inBytes = inWords * sizeof(OH::RomWord);
  OH::RomWord *in, *u32p;
  unsigned char *out;
  for (unsigned n = 0; n < inWords; n++) {
    char line[34];
    line[32] = 0;
    if (!fgets(line, 34, ifp))
      bad("error reading input");
    size_t len = strlen(line);
    if (line[len - 1] != '\n')
      bad("error in data in input file lines");
    line[len-1] = '\0';
    errno = 0;
    char *end;
    unsigned long l = strtoul(line, &end, 16);
    if (errno != 0 || *end != 0)
      bad("error in data in input file data");
    OH::RomWord i = OCPI_UTRUNCATE(OH::RomWord, l);
    switch(n) {
    case 0:
      if (i != 1)
	bad("format magic");
      break;
    case 1:
      inBytes = i;
      if (!(u32p = in = (OH::RomWord *)malloc(inBytes)))
	bad("Error allocating %zu bytes for input file", inBytes);
      break;
    case 2:
      olength = i;
      if (olength > 100*OH::ROM_NBYTES ||
	  !(out = (unsigned char *)malloc(olength)))
	bad("Error allocating %zu bytes for output file", olength);
      break;
    case 3:
      adler = i;
      break;
    default:
      *u32p++ = i;
    }
  }
  printf("Expanding from %zu bytes in bram to %zu bytes in text\n",
	 inBytes, olength);
  size_t total, check;
#ifdef USE_LZMA
  lzma_ret lr;
  uint64_t memlimit = UINT64_MAX;
  size_t in_pos = 0, out_pos = 0;
  if ((lr = lzma_stream_buffer_decode(&memlimit, // ptr to max memory to use during decode
				      0,         // flags
				      NULL,      // allocator if not malloc/free
				      (uint8_t *)in,        // input buffer
				      &in_pos,   // updated input index
				      inBytes,   // size of input
				      out,       // output buffer
				      &out_pos,  // updated output index
				      olength)) != LZMA_OK)
    bad("Decompression failure: %u", lr);
  total = out_pos;
  check = 0;
#else
  int zr;
  z_stream zs;  
  zs.zalloc = zalloc;
  zs.zfree = zfree;
  zs.data_type = Z_TEXT;
  zs.next_in = (unsigned char *)in;
  zs.avail_in = (uInt)inBytes;
  zs.next_out = out;
  zs.avail_out = (uInt)olength;
  if ((zr = inflateInit(&zs)) != Z_OK ||
      (zr = inflate(&zs, Z_FINISH)) != Z_STREAM_END ||
      (zr = inflateEnd(&zs)) != Z_OK)
    bad("zlib decompression failed: %u", zr);
  total = zs.total_out;
  check = zs.adler;
#endif
  if (check != adler || total != olength)
    bad("bad checksum on decompressed data: is %lx, should be %zx", check, adler);
  if (fwrite(out, 1, total, ofp) != total || fclose(ofp)) {
    unlink(ap[1]);
    bad("Error writing output file '%s'", ap[1]);
  }
  printf("Wrote unbram file '%s' (%zu bytes) from file '%s' (%zu bytes)\n",
	 ap[1], total, ap[0], inBytes);
}

static void
uuid(const char **ap) {
  OH::HdlUUID uuidRegs;
  if (platform.empty() || !part)
    bad("both platform and part/chip must be specified for the uuid command");
  if (!*ap)
    bad("No output uuid verilog specified for uuid command");
  FILE *ofp = fopen(ap[0], "wb");
  if (!ofp)
    bad("Cannot open output file: \"%s\"", ap[1]);
  if (platform.length() > sizeof(uuidRegs.platform) - 1)
    bad("Platform: '%s' is too long.  Must be < %u long",
	platform.c_str(), sizeof(uuidRegs.platform) - 1);
  if (strlen(part) > sizeof(uuidRegs.device) - 1)
    bad("Part/chip: '%s' is too long.  Must be < %u long",
	part, sizeof(uuidRegs.device) - 1);
  OU::Uuid uuid;
  OU::generateUuid(uuid);
  memcpy(uuidRegs.uuid, uuid, sizeof(uuidRegs.uuid));
  uuidRegs.birthday = (uint32_t)time(0);
  strncpy(uuidRegs.platform, platform.c_str(), sizeof(uuidRegs.platform));
  strncpy(uuidRegs.device, part, sizeof(uuidRegs.device));
  strncpy(uuidRegs.load, "", sizeof(uuidRegs.load));
  assert(sizeof(uuidRegs) * 8 == 512);
  OU::UuidString textUUID;
  OU::uuid2string(uuid, textUUID);
  fprintf(ofp,
	  "// UUID generated for platform '%s', device '%s', uuid '%s'\n"
	  "module mkUUID(uuid);\n"
	  "output [511 : 0] uuid;\nwire [511 : 0] uuid = 512'h",
	  platform.c_str(), part, textUUID);
  for (unsigned n = 0; n < sizeof(uuidRegs); n++)
    fprintf(ofp, "%02x", ((char*)&uuidRegs)[n]&0xff);
  fprintf(ofp, ";\nendmodule // mkUUID\n");
  if (fclose(ofp))
    bad("Could close output file '%s'. No space?", ap[0]);
}
  
static void
reset(const char **) {
  // FIXME:  need to copy PCI config.
  cAccess->set32Register(reset, OH::OccpAdminRegisters, 0xc0deffff);
}
static uint64_t
atoi_any(const char *arg, unsigned *sizep)
{
  uint64_t value ;

  if (!arg)
    bad("Missing numeric argument");
  int n = sscanf(arg, strncmp(arg, "0x", 2) != 0 ? "%" SCNu64 : "0x%" SCNx64, &value);
  if (n != 1)
    bad("Bad numeric value: '%s'", arg);
  if (sizep) {
    const char *sp;
    if ((sp = strchr(arg, '/')))
      switch(*++sp) {
      default:
	bad("Bad size specifier in %s: must be 1, 2, or 4", arg);
	abort();
      case '1':
      case '2':
      case '4':
      case '8':
	*sizep = *sp - '0';
      }
    else
      *sizep = 4;
  }
  return value ;
}

static void
radmin(const char **ap) {
  unsigned size;
  unsigned off = (unsigned)atoi_any(*ap, &size);
  if (size == 4) {
    uint32_t x = cAccess->get32RegisterOffset(off);
    if (parseable)
      printf("0x%" PRIx32 "\n", x);
    else
      printf("Admin for hdl-device '%s' at offset 0x%x is 0x%x (%u)\n",
	     device, off, x, x);
  } else if (size == 8) {
    uint64_t x = cAccess->get64RegisterOffset(off);
    if (parseable)
      printf("0x%" PRIx64 "\n", x);
    else
      printf("Admin for hdl-device '%s' at offset 0x%x is 0x%" PRIx64 " (%" PRIi64 ")\n",
	     device, off, x, x);
  } else
    bad("bad size for radmin");
}

static void
rmeta(const char **ap) {
  unsigned size;
  unsigned off = (unsigned)atoi_any(*ap, &size);
  if (size == 4) {
    uint32_t x = cAccess->get32RegisterOffset(off + offsetof(OH::OccpSpace,configRam));
    if (parseable)
      printf("0x%" PRIx32 "\n", x);
    else
      printf("Metadata for hdl-device '%s' at offset 0x%x is 0x%x (%u)\n",
	     device, off, x, x);
  } else if (size == 8) {
    uint64_t x = cAccess->get64RegisterOffset(off + offsetof(OH::OccpSpace,configRam));
    if (parseable)
      printf("0x%" PRIx64 "\n", x);
    else
      printf("Metadata for hdl-device '%s' at offset 0x%x is 0x%" PRIx64 " (%" PRIi64 ")\n",
	     device, off, x, x);
  } else
    bad("bad size for rmeta");
}

static void
wadmin(const char **ap) {
  unsigned size;
  unsigned off = (unsigned)atoi_any(*ap++, &size);
  uint64_t val = atoi_any(*ap, 0);
  switch (size) {
  case 1:
    cAccess->set8RegisterOffset(off, (uint8_t)val);
    break;
  case 2:
    cAccess->set16RegisterOffset(off, (uint16_t)val);
    break;
  case 4:
    cAccess->set32RegisterOffset(off, (uint32_t)val);
    break;
  case 8:
    cAccess->set64RegisterOffset(off, val);
    break;
  default:
    bad("bad size for wadmin");
  }
}
static void
settime(const char **) {
  struct timeval tv;
  gettimeofday(&tv, NULL); 
  uint32_t fraction = (uint32_t)
    ((((uint64_t)tv.tv_usec * 1000 * (1ull << 32) ) + 500000000ull) / 1000000000ull);
  // Write 64 bit value
  // When it goes on the PCIe wire, it will be "endianized".
  // On intel, first DW will be LSB.  On PPC, first DW will be MSB.
  
#define FPGA_IS_OPPOSITE_ENDIAN_FROM_CPU() 1

  cAccess->set64Register(time, OH::OccpAdminRegisters, 
#if FPGA_IS_OPPOSITE_ENDIAN_FROM_CPU()
			((uint64_t)fraction << 32) | tv.tv_sec
#else
			((uint64_t)tv.tv_sec << 32) | fraction
#endif
			);
}
typedef unsigned long long ull; 
static inline int64_t ticks2ns(uint64_t ticks) {
  return ((ticks) * 1000000000ull + (1ull << 31))/ (1ull << 32);
}
static inline int64_t dticks2ns(int64_t ticks) {
  return ((ticks) * 1000000000ll + (1ll << 31))/ (1ll << 32);
}
#if 0
static inline ull ns2ticks(uint32_t sec, uint32_t nsec) {
  return (((uint64_t)(sec)) << 32ull) + ((nsec) + 500000000ull) * (1ull<<32) /1000000000;
}
#endif
static int compu32(const void *a, const void *b) { return *(int32_t*)a - *(int32_t*)b; }
static inline uint64_t swap32(uint64_t x) {return (x <<32) | (x >> 32); }
static void
deltatime(const char **) {
  unsigned n;
  int32_t delta[100];
  int64_t sum = 0;
  
  for (n = 0; n < 100; n++) {
    uint64_t time = cAccess->get64Register(time, OH::OccpAdminRegisters);
    cAccess->set64Register(timeDelta, OH::OccpAdminRegisters, time);
    delta[n] = (int32_t)swap32(cAccess->get64Register(timeDelta, OH::OccpAdminRegisters));
  }
  qsort(delta, 100, sizeof(int32_t), compu32);
  
  for (n = 0; n < 90; n++)
    sum += delta[n];
  sum = (sum + 45) / 90;
  // we have average delay
  printf("Delta ns min %" PRIi64 " max %" PRIi64 ". average: (of best 90 out of 100) %" PRIi64 "\n",
	 ticks2ns(delta[0]), ticks2ns(delta[99]), ticks2ns(sum));
  uint64_t time = swap32(cAccess->get64Register(time, OH::OccpAdminRegisters));
  cAccess->set64Register(timeDelta, OH::OccpAdminRegisters, swap32(time + sum));
  int32_t deltatime = (int32_t)swap32(cAccess->get64Register(timeDelta, OH::OccpAdminRegisters));
  printf("Now after correction, delta is: %" PRIi64 "ns\n", dticks2ns(deltatime));
}
static void
wdump(const char **) {
  printf("Worker %zu on device %s\n", worker, device);
  uint32_t i = wAccess.get32Register(status, OH::OccpWorkerRegisters);
  printf(" Status:     0x%08x", i);
  if (i & OCCP_STATUS_CONTROL_ERROR)
    printf(" ctlError");
  if (i & OCCP_STATUS_READ_ERROR)
    printf(" rdError");
  if (i & OCCP_STATUS_READ_FAIL)
    printf(" rdBusy");
  if (i & OCCP_STATUS_WRITE_ERROR)
    printf(" wrtError");
  if (i & OCCP_STATUS_WRITE_FAIL)
    printf(" wrtBusy");
  if (i & OCCP_STATUS_CONTROL_TIMEOUT)
    printf(" ctlTimeout");
  if (i & OCCP_STATUS_CONTROL_FAIL)
    printf(" ctlBusy");
  if (i & OCCP_STATUS_READ_TIMEOUT)
    printf(" rdTimeout");
  if (i & OCCP_STATUS_ACCESS_ERROR)
    printf(" accError");
  if (i & OCCP_STATUS_ATTENTION)
    printf(" attention");
  if (i & OCCP_STATUS_FINISHED)
    printf(" finished");
  if (i & OCCP_STATUS_CONFIG_ADDR_VALID)
    printf(" addrValid");
  if (i & OCCP_STATUS_CONFIG_BE_VALID)
    printf(" beValid:0x%x", (i & OCCP_STATUS_CONFIG_BE) >> 20);
  static const char *ops[] = { "init", "start", "stop", "release", "test", "bQuery", "aConfig", "rsvd7"};
  if (i & OCCP_STATUS_CONFIG_OP_VALID)
    printf(" opValid:0x%x:%s", (i & OCCP_STATUS_CONFIG_OP) >> 24, ops[ (i & OCCP_STATUS_CONFIG_OP) >> 24]);
  if (i & OCCP_STATUS_CONFIG_WRITE_VALID)
    printf(" wrtValid:%d", (i & OCCP_STATUS_CONFIG_WRITE) ? 1 : 0);
  printf("\n");
  i = wAccess.get32Register(control, OH::OccpWorkerRegisters);
  printf(" Control:    0x%08x %s;  timeout value is %u\n", i,
	 i & OCCP_WORKER_CONTROL_ENABLE ?
	 "enabled (reset not asserted)" : "not enabled (reset asserted)",
	 1 << OCCP_WORKER_CONTROL_TIMEOUT(i)); 
  printf(" ConfigAddr: 0x%08x\n", wAccess.get32Register(lastConfig, OH::OccpWorkerRegisters));
  printf(" PageWindow: 0x%08x\n", wAccess.get32Register(window, OH::OccpWorkerRegisters));
}

static int
wwreset(OH::Access &w) {
  uint32_t i = w.get32Register(control, OH::OccpWorkerRegisters);
  w.set32Register(control, OH::OccpWorkerRegisters, i & ~OCCP_WORKER_CONTROL_ENABLE);
  return i;
}
static void
wreset(const char **) {
  uint32_t i = wwreset(wAccess);
  printf("Worker %zu on device %s: reset asserted, was %s\n", 
	 worker, device, (i & OCCP_WORKER_CONTROL_ENABLE) ? "deasserted" : "already asserted");
}
static int
wwunreset(OH::Access &w) {
  uint32_t i = w.get32Register(control, OH::OccpWorkerRegisters);
  w.set32Register(control, OH::OccpWorkerRegisters, i  | OCCP_WORKER_CONTROL_ENABLE);
  return i;
}
static void
wunreset(const char **) {
  uint32_t i = wwunreset(wAccess);
  printf("Worker %zu on device %s: reset deasserted, was %s\n", 
	 worker, device, (i & OCCP_WORKER_CONTROL_ENABLE) ? "already deasserted" : "asserted");
}

#define WOP(name) offsetof(OH::OccpWorkerRegisters, name)
static uint32_t
wwop(OH::Access &w, const char *op) {
  for (unsigned i = 0; ops[i]; i++) 
    if (!strcasecmp(ops[i], op)) {
      ocpiDebug("Worker control op: %s, %u offset %zx", op, i,
		offsetof(OH::OccpWorkerRegisters, initialize) + i * sizeof(uint32_t));
     return w.get32RegisterOffset(offsetof(OH::OccpWorkerRegisters, initialize) +
				   i * sizeof(uint32_t));
    }
  bad("Unknown control operation: `%s'", op);
  return 0;
}      

static void
wop(const char **ap) {
  if (!*ap)
    bad("Missing control operation name for this command");
  uint32_t r = wwop(wAccess, *ap);
  if (parseable)
    printf("0x%" PRIx32 "\n", r);
  else
    printf("Worker %zu on device %s: the '%s' control operation was performed with result '%s' (0x%x)\n",
	   worker, device, *ap,
	   r == OCCP_ERROR_RESULT ? "error" :
	   r == OCCP_TIMEOUT_RESULT ? "timeout" :
	   r == OCCP_RESET_RESULT ? "reset" :
	   r == OCCP_SUCCESS_RESULT ? "success" : 
	   r == OCCP_BUSY_RESULT ? "busy" : 
	   r == OCCP_FATAL_RESULT  ? "fatal" : "unknown",
	   r);
}

static void
wwctl(const char **ap) {
  uint32_t i = (uint32_t)atoi_any(*ap++, 0);
  wAccess.set32Register(control, OH::OccpWorkerRegisters, i);
  printf("Worker %zu on device %s: writing control register value: 0x%x\n",
	 worker, device, i);
}
static void
wclear(const char **) {
  uint32_t i = wAccess.get32Register(control, OH::OccpWorkerRegisters);
  wAccess.set32Register(control, OH::OccpWorkerRegisters,
			i | OCCP_CONTROL_CLEAR_ATTENTION | OCCP_CONTROL_CLEAR_ERRORS);
  printf("Worker %zu on device %s: clearing errors from status register\n",
	 worker, device);
}
static void
wwpage(const char **ap) {
  uint32_t i = (uint32_t)atoi_any(*ap++, 0);
  wAccess.set32Register(window, OH::OccpWorkerRegisters, i);
  printf("Worker %zu on device %s: setting window register to 0x%x (%u)\n",
	 worker, device, i, i);
}
static void
wread(const char **ap) {
  unsigned size;
  unsigned off = (unsigned)atoi_any(*ap++, &size);
  unsigned n = *ap ? (unsigned)atoi_any(*ap, 0) : 1;
  if (!parseable)
    printf("Worker %zu on device %s: read config offset 0x%x size %u count %u\n",
	   worker, device, off, size, n);

  for (unsigned n = *ap ? (unsigned)atoi_any(*ap, 0) : 1; n--; off += size) {
    uint64_t i = 0;
    switch (size) {
    case 1:
      i = confAccess.get8RegisterOffset(off); break;
    case 2:
      i = confAccess.get16RegisterOffset(off); break;
    case 4:
      i = confAccess.get32RegisterOffset(off); break;
    case 8:
      i = confAccess.get64RegisterOffset(off); break;
    }
    if (parseable)
      printf("0x%08" PRIx64"\n", i);
    else
      printf("  offset 0x%08x (%8u), value 0x%08" PRIx64 " (%10" PRIu64 ")\n",
	     off, off, i, i);
  }
}
static void
wwrite(const char **ap) {
  unsigned size;
  unsigned off = (unsigned)atoi_any(*ap++, &size);
  uint64_t val = atoi_any(*ap, 0);

  switch (size) {
  case 1:
    confAccess.set8RegisterOffset(off, (uint8_t)val); break;
  case 2:
    confAccess.set16RegisterOffset(off, (uint16_t)val); break;
  case 4:
    confAccess.set32RegisterOffset(off, (uint32_t)val); break;
  case 8:
    confAccess.set64RegisterOffset(off, val); break;
  }
  printf("Worker %zu on device %s: wrote config offset 0x%x size %u: value is 0x%" PRIx64 "(%" PRIu64 ")\n",
	 worker, device, off, size, val, val);
}
static OE::Socket *getSock() {
  OE::Interface i(interface, error);
  if (error.size())
    bad("Opening interface");
  OE::Socket *s = new OE::Socket(i, ocpi_data, NULL, 123, error);
  if (error.size()) {
    delete s;
    bad("opening ethernet socket for data");
  }
  return s;
}

static void
sendData(const char **ap) {
  OE::Address a(ap[0]);
  if (a.hasError())
    bad("Establishing remote address as %s", ap[0]);
  OE::Socket *s = getSock();
  OE::Packet p;
  ((uint16_t *)p.payload)[1] = 123;
  for (uint8_t n = 0; n < 10; n++) {
    p.payload[4] = n;
    if (!s->send(p, 5, a, 0, NULL, error))
      bad("sending");
  }
}
static void
receiveData(const char **/*ap*/) {
  OE::Socket *s = getSock();
  OE::Packet p;
  OE::Address from;
  for (unsigned n = 0; n < 10; n++) {
    size_t len;
    if (!s->receive(p, len, 0, from, error))
      bad("receiving");
    if (p.payload[2] != 123 || p.payload[4] != n)
      bad("received %d/%d, wanted %d/%d", p.payload[2], p.payload[4], 123, n);
  }
  printf("Got 10 good packets\n");
}
// Receive an RDMA stream using the ethernet datagram transport
// If there is a file name argument, we are exchanging descriptors between software endpoints
// otherwise we are dealing directly with an HDL hardware endpoint
#define EDP_WORKER 13
#define SMA_WORKER 6
#define GBE_WORKER 9
#define GEN_WORKER 5
static void
receiveRDMA(const char **ap) {
  if (!*ap)
    setupDevice(false);
  std::string file;
  OD::TransportGlobal &global(OC::Manager::getTransportGlobal());
  OD::Transport transport(&global, false);
  if (endpoint.empty())
    endpoint = "ocpi-ether-rdma";
  OU::PValue params[2] = { 
    OA::PVString("remote", endpoint.c_str()),
    OA::PVEnd
  };
  OD::Descriptor myInputDesc;
  // Initialize out input descriptor before it is processed and
  // completed by the transport system.
  myInputDesc.type = OD::ConsumerDescT;
  myInputDesc.role = OD::ActiveFlowControl;
  myInputDesc.options = 1 << OD::MandatedRole;
  myInputDesc.desc.nBuffers = 10;
  myInputDesc.desc.dataBufferBaseAddr = 0;
  myInputDesc.desc.dataBufferPitch = 0;
  myInputDesc.desc.dataBufferSize = 4096;
  myInputDesc.desc.metaDataBaseAddr = 0;
  myInputDesc.desc.metaDataPitch = 0;
  myInputDesc.desc.fullFlagBaseAddr =  0;
  myInputDesc.desc.fullFlagSize = 0;
  myInputDesc.desc.fullFlagPitch = 0;
  myInputDesc.desc.fullFlagValue = 0;
  myInputDesc.desc.emptyFlagBaseAddr = 0;
  myInputDesc.desc.emptyFlagSize = 0;
  myInputDesc.desc.emptyFlagPitch = 0;
  myInputDesc.desc.emptyFlagValue = 0;
  myInputDesc.desc.oob.port_id = 0;
  memset( myInputDesc.desc.oob.oep, 0, sizeof(myInputDesc.desc.oob.oep));
  myInputDesc.desc.oob.cookie = 0;
  OD::Port &port = *transport.createInputPort(myInputDesc, params);
  ocpiDebug("Our input descriptor: %s", myInputDesc.desc.oob.oep);
  // Now the normal transport/transfer driver has initialized (not finalized) the input port.
  OD::Descriptor theOutputDesc;
  OH::Access
    edpAccess, edpConfAccess, smaAccess, smaConfAccess, gbeAccess, gbeConfAccess,
    genAccess, genConfAccess;
  if (*ap) {
    // If we are exchanging descriptors, write it out to the given file with ".in" suffix
    // Pause (getchar), and then read in the output descriptor from the other side.
    file = *ap;
    file += ".in";
    // Strip out our ether interface.
    OD::Descriptor sendDesc = myInputDesc;
    const char *sp = strchr(myInputDesc.desc.oob.oep,'/');
    std::string clean("ocpi-ether-rdma:");
    clean.append(sp+1);
    strcpy(sendDesc.desc.oob.oep, clean.c_str());
    int fd = creat(file.c_str(), 0666);
    if (fd < 0 ||
	write(fd, &sendDesc, sizeof(sendDesc)) != sizeof(sendDesc) ||
	close(fd) < 0)
      bad("Creating temp file for RDMA descriptor");
    printf("Input Descriptor written to %s.\nWaiting for a keystroke...", file.c_str());
    fflush(stdout);
    getchar();
    file = *ap;
    file += ".out";
    fd = open(file.c_str(), O_RDONLY);
    if (fd < 0 ||
	read(fd, &theOutputDesc, sizeof(theOutputDesc)) != sizeof(theOutputDesc) ||
	close(fd) < 0)
      bad("Reading file for RDMA output descriptor");
    // Now we have the output descriptor for the remote output port
  } else {
    // The output port is a hardware port.  We need to program the hardware 
    // as well as produce/synthesize the output descriptor for the remote hardware port.
    // 1. Get acccessors to all workers with control and configuration accessors for each.
    cAccess->offsetRegisters(edpAccess, (intptr_t)(&((OH::OccpSpace*)0)->worker[EDP_WORKER]));
    cAccess->offsetRegisters(edpConfAccess, (intptr_t)(&((OH::OccpSpace*)0)->config[EDP_WORKER]));
    cAccess->offsetRegisters(smaAccess, (intptr_t)(&((OH::OccpSpace*)0)->worker[SMA_WORKER]));
    cAccess->offsetRegisters(smaConfAccess, (intptr_t)(&((OH::OccpSpace*)0)->config[SMA_WORKER]));
    cAccess->offsetRegisters(gbeAccess, (intptr_t)(&((OH::OccpSpace*)0)->worker[GBE_WORKER]));
    cAccess->offsetRegisters(gbeConfAccess, (intptr_t)(&((OH::OccpSpace*)0)->config[GBE_WORKER]));
    cAccess->offsetRegisters(genAccess, (intptr_t)(&((OH::OccpSpace*)0)->worker[GEN_WORKER]));
    cAccess->offsetRegisters(genConfAccess, (intptr_t)(&((OH::OccpSpace*)0)->config[GEN_WORKER]));
    wwreset(edpAccess);
    wwreset(smaAccess);
    wwreset(gbeAccess);
    wwreset(genAccess);
    wwunreset(edpAccess);
    wwunreset(smaAccess);
    wwunreset(gbeAccess);
    wwunreset(genAccess);
    ocpiCheck(wwop(edpAccess, "initialize") == OCCP_SUCCESS_RESULT);
    ocpiCheck(wwop(smaAccess, "initialize") == OCCP_SUCCESS_RESULT);
    ocpiCheck(wwop(gbeAccess, "initialize") == OCCP_SUCCESS_RESULT);
    ocpiCheck(wwop(genAccess, "initialize") == OCCP_SUCCESS_RESULT);
    // 2. Synthesize the output descriptor
    theOutputDesc.type = OD::ProducerDescT;
    theOutputDesc.role = OD::ActiveMessage;
    theOutputDesc.options = 1 << OD::MandatedRole;
    theOutputDesc.desc.nBuffers = 3;
    theOutputDesc.desc.dataBufferBaseAddr = 0;
    theOutputDesc.desc.dataBufferPitch = 4096;
    theOutputDesc.desc.dataBufferSize = 4096;
    theOutputDesc.desc.metaDataBaseAddr = 4096 * theOutputDesc.desc.nBuffers;
    theOutputDesc.desc.metaDataPitch = sizeof(OH::OcdpMetadata);
    theOutputDesc.desc.fullFlagBaseAddr = 
      theOutputDesc.desc.metaDataBaseAddr + theOutputDesc.desc.metaDataPitch * theOutputDesc.desc.nBuffers;
    theOutputDesc.desc.fullFlagSize = 4;
    theOutputDesc.desc.fullFlagPitch = 4;
    theOutputDesc.desc.fullFlagValue = 1;
    theOutputDesc.desc.emptyFlagBaseAddr = edpConfAccess.physOffset(offsetof(OH::OcdpProperties, nRemoteDone));
    theOutputDesc.desc.emptyFlagSize = 4;
    theOutputDesc.desc.emptyFlagPitch = 0;
    theOutputDesc.desc.emptyFlagValue = 1;
    theOutputDesc.desc.oob.port_id = 0;
    theOutputDesc.desc.oob.cookie = 0;
    uint32_t outputEndPointSize = edpConfAccess.get32Register(memoryBytes, OH::OcdpProperties);
    DT::EndPoint &outputEndPoint =
      DT::getManager().allocateProxyEndPoint(endpoint.c_str(), outputEndPointSize);
    OD::Transport::fillDescriptorFromEndPoint(outputEndPoint, theOutputDesc);
  }
  // Finalizing the input port takes: role, type flow, emptyflagbase, size, pitch, value
  // This makes the input port ready to data from the output port
  port.finalize(theOutputDesc, myInputDesc);
  ocpiDebug("Our output descriptor is: %s", theOutputDesc.desc.oob.oep);
  if (!*ap) {
    // If the other side is software (sendRDMA below), it will start when told.
    // If it is hardware we setup and start the workers now
    // We aren't passing descriptors, but programming the hardware and
    // synthesizing the output descriptor

    // Configure the 4 workers, upstream to downstream:
    // 1. The message generator set up to send 10 messages of 16 bytes each.
    struct Generator {
      uint32_t
        enable,         // 0x00
	wrapCount,      // 0x04
	messagesSent,   // 0x08
	dataSent,       // 0x0c
	messagesToSend, // 0x10
	status;         // 0x14
    };
    genAccess.set32Register(window, OH::OccpWorkerRegisters, 0);     // for normal config
    //    genConfAccess.set32Register(enable, Generator, 1);
    genConfAccess.set32Register(wrapCount, Generator, 1);
    genConfAccess.set32Register(messagesToSend, Generator, 10);
    genAccess.set32Register(window, OH::OccpWorkerRegisters, 0x400); // setup for metadata
    genConfAccess.set32Register(length, OH::OcdpMetadata, 16);
    genConfAccess.set32Register(opCode, OH::OcdpMetadata, 1);
    genAccess.set32Register(window, OH::OccpWorkerRegisters, 0x800); // setup for data
    genConfAccess.set32RegisterOffset(0, 1);
    genConfAccess.set32RegisterOffset(4, 2);
    genConfAccess.set32RegisterOffset(8, 3);
    genConfAccess.set32RegisterOffset(0xc, 4);
    genAccess.set32Register(window, OH::OccpWorkerRegisters, 0);     // for normal config
    // 2. The SMA adapter converting from WSI to WMI
    smaConfAccess.set32RegisterOffset(0, 2); // WSI to WMI
    // 3. The EDP, set up to perform datagram RDMA
    // Tell the output side about the input side, as determined by createInputPort
    edpConfAccess.set32Register(control, OH::OcdpProperties,
				OCDP_CONTROL(OCDP_CONTROL_PRODUCER, OH::OCDP_ACTIVE_MESSAGE));
    edpConfAccess.set32Register(nRemoteBuffers, OH::OcdpProperties, myInputDesc.desc.nBuffers);
    edpConfAccess.set32Register(remoteBufferBase, OH::OcdpProperties,
				(uint32_t)myInputDesc.desc.dataBufferBaseAddr);
    edpConfAccess.set32Register(remoteMetadataBase, OH::OcdpProperties,
				(uint32_t)myInputDesc.desc.metaDataBaseAddr);
    edpConfAccess.set32Register(remoteBufferSize, OH::OcdpProperties, myInputDesc.desc.dataBufferPitch);
    edpConfAccess.set32Register(remoteMetadataSize, OH::OcdpProperties, myInputDesc.desc.metaDataPitch);
    edpConfAccess.set32Register(remoteFlagBase, OH::OcdpProperties,
				(uint32_t)myInputDesc.desc.fullFlagBaseAddr);
    edpConfAccess.set32Register(remoteFlagPitch, OH::OcdpProperties, myInputDesc.desc.fullFlagPitch);
    // Program the source/destination ids for remote DMA
    uint32_t val = myInputDesc.desc.oob.mailBox | (theOutputDesc.desc.oob.mailBox << 16);
    ocpiDebug("DW with DestId in LSB and SourceID in MSB is 0x%x", val);
    edpConfAccess.set32Register(remoteBufferHi, OH::OcdpProperties, val);

    // Program the local part of the output side - how data is placed in local buffers
    edpConfAccess.set32Register(nLocalBuffers, OH::OcdpProperties, theOutputDesc.desc.nBuffers);
    edpConfAccess.set32Register(localBufferSize, OH::OcdpProperties, theOutputDesc.desc.dataBufferPitch);
    edpConfAccess.set32Register(localBufferBase, OH::OcdpProperties, 
				(uint32_t)theOutputDesc.desc.dataBufferBaseAddr);
    edpConfAccess.set32Register(localMetadataBase, OH::OcdpProperties,
				(uint32_t)theOutputDesc.desc.metaDataBaseAddr);
    edpConfAccess.set32Register(localMetadataSize, OH::OcdpProperties, theOutputDesc.desc.metaDataPitch);
    // 4. The GBE worker that inserts the MAC header fields.
    OE::Address addr(strchr(myInputDesc.desc.oob.oep, '/') + 1);
    union {
      struct {
	uint8_t addr[6];
	uint16_t ethertype;
      };
      uint64_t value;
    } value;
    memcpy(value.addr, addr.addr(), addr.s_size);
    value.ethertype = OCDP_ETHER_TYPE;
    ocpiDebug("Telling device to send to %s with ethertype %x",
	      addr.pretty(), OCDP_ETHER_TYPE);
    gbeConfAccess.set64RegisterOffset(0x10, value.value);
    // Now that they are all configured, start from downstream to upstream.
    ocpiCheck(wwop(gbeAccess, "start") == OCCP_SUCCESS_RESULT);
    ocpiCheck(wwop(edpAccess, "start") == OCCP_SUCCESS_RESULT);
    ocpiCheck(wwop(smaAccess, "start") == OCCP_SUCCESS_RESULT);
    ocpiCheck(wwop(genAccess, "start") == OCCP_SUCCESS_RESULT);
    genConfAccess.set32Register(enable, Generator, 1);
  }
  for (unsigned n = 0; n < 10; n++) {
    uint8_t opCode;
    size_t length;
    void *vdata;
    OD::BufferUserFacet *buf;
    unsigned t;
    for (t = 0; t < 1000000 && !(buf = port.getNextFullInputBuffer(vdata, length, opCode)); t++)
      OS::sleep(1);
    if (t == 1000000)
      ocpiBad("Receive DMA timeout\n");
    else {
      ocpiDebug("Received RDMA buffer: first DW 0x%" PRIx32 ", length %zu, op %u\n",
		*(uint32_t *)vdata, length, opCode);
      if (opCode != n)
	ocpiBad("Bad opcode: wanted %u, got %u", n, opCode);
      port.releaseInputBuffer(buf);
    }
  }
  printf("Received 10 RDMA buffers\n");
  port.reset();
}
// Send an RDMA stream using a datagram transport
static void
sendRDMA(const char **ap) {
  if (!*ap)
    setupDevice(false);
  std::string file;
  OD::TransportGlobal &global(OC::Manager::getTransportGlobal());
  OD::Transport transport(&global, false);
  OD::Descriptor myOutputDesc;
  myOutputDesc.type = OD::ProducerDescT;
  myOutputDesc.role = OD::ActiveMessage;
  myOutputDesc.options = 1 << OD::MandatedRole;
  myOutputDesc.desc.nBuffers = 10;
  myOutputDesc.desc.dataBufferBaseAddr = 0;
  myOutputDesc.desc.dataBufferPitch = 0;
  myOutputDesc.desc.dataBufferSize = 4096;
  myOutputDesc.desc.metaDataBaseAddr = 0;
  myOutputDesc.desc.metaDataPitch = 0;
  myOutputDesc.desc.fullFlagBaseAddr =  0;
  myOutputDesc.desc.fullFlagSize = 0;
  myOutputDesc.desc.fullFlagPitch = 0;
  myOutputDesc.desc.fullFlagValue = 0;
  myOutputDesc.desc.emptyFlagBaseAddr = 0;
  myOutputDesc.desc.emptyFlagSize = 0;
  myOutputDesc.desc.emptyFlagPitch = 0;
  myOutputDesc.desc.emptyFlagValue = 0;
  myOutputDesc.desc.oob.port_id = 0;
  memset( myOutputDesc.desc.oob.oep, 0, sizeof(myOutputDesc.desc.oob.oep));
  myOutputDesc.desc.oob.cookie = 0;

  OD::Descriptor theInputDesc;
  if (*ap) {
    file = *ap;
    file += ".in";
    int fd = open(file.c_str(), O_RDONLY);
    if (fd < 0 ||
	read(fd, &theInputDesc, sizeof(theInputDesc)) != sizeof(theInputDesc) ||
	close(fd) < 0)
      bad("Reading file for RDMA input descriptor");
    ocpiDebug("Received descriptor for: %s", myOutputDesc.desc.oob.oep);
  } else {
    theInputDesc.type = OD::ProducerDescT;
    theInputDesc.role = OD::ActiveMessage;
    theInputDesc.options = 1 << OD::MandatedRole;
    theInputDesc.desc.nBuffers = 10;
    theInputDesc.desc.dataBufferBaseAddr = 0;
    theInputDesc.desc.dataBufferPitch = 0;
    theInputDesc.desc.dataBufferSize = 0;
    theInputDesc.desc.metaDataBaseAddr = 0;
    theInputDesc.desc.metaDataPitch = 0;
    theInputDesc.desc.fullFlagBaseAddr =  0;
    theInputDesc.desc.fullFlagSize = 0;
    theInputDesc.desc.fullFlagPitch = 0;
    theInputDesc.desc.fullFlagValue = 0;
    theInputDesc.desc.emptyFlagBaseAddr = 0;
    theInputDesc.desc.emptyFlagSize = 0;
    theInputDesc.desc.emptyFlagPitch = 0;
    theInputDesc.desc.emptyFlagValue = 0;
    theInputDesc.desc.oob.port_id = 0;
    memset( theInputDesc.desc.oob.oep, 0, sizeof(theInputDesc.desc.oob.oep));
    theInputDesc.desc.oob.cookie = 0;
  }
  OD::Port &port = *transport.createOutputPort(myOutputDesc, theInputDesc);
  OD::Descriptor localShadowPort, feedback;
  const OD::Descriptor &outDesc = *port.finalize(theInputDesc, myOutputDesc, &localShadowPort);
  if (*ap) {
    file = *ap;
    file += ".out";
    OD::Descriptor sendDesc = outDesc;
    const char *sp = strchr(outDesc.desc.oob.oep, '/');
    std::string clean("ocpi-ether-rdma:");
    clean.append(sp+1);
    strcpy(sendDesc.desc.oob.oep, clean.c_str());
    int fd = creat(file.c_str(), 0666);
    if (fd < 0 ||
	write(fd, &sendDesc, sizeof(sendDesc)) != sizeof(sendDesc) ||
	close(fd) < 0)
      bad("Writing output RDMA descriptor");
    printf("Flow Control Descriptor in %s.\nWaiting for a keystroke...", file.c_str());
    fflush(stdout);
    getchar();
  }
  for (unsigned n = 0; n < 10; n++) {
    void *vdata;
    size_t length;
    OD::BufferUserFacet *buf;
    unsigned t;
    for (t = 0; t < 1000000 && !(buf = port.getNextEmptyOutputBuffer(vdata, length)); t++)
      OS::sleep(1);
    if (t == 1000000)
      bad("Receive DMA timeout\n");
    else {
      for (unsigned d = 0; d < length/sizeof(uint32_t); d++)
	((uint32_t *)vdata)[d] = n + d;
      port.sendOutputBuffer(buf, 4096, (uint8_t)n);
      ocpiDebug("Sent RDMA buffer: first DW 0x%" PRIx32 ", length %zu, op %u\n",
		*(uint32_t *)vdata, length, n);
    }
  }
  printf("Sent 10 buffers, sleeping to allow frames to get through and be ack'd...\n");
  fflush(stdout);
  OS::sleep(10000);
  printf("Exiting\n");
  port.reset();
}

static void
simulate(const char **ap) {
  OH::Sim::Server server(*ap, platform, OCPI_UTRUNCATE(uint8_t,spinCount), sleepUsecs,
			 simTicks, verbose, simDump, isPublic, error);
  if (error.length())
    bad("Simulator server creation error");
  if (server.run(simExec, error))
    bad("Simulator server execution error");
  ocpiDebug("Simulation server finished: '%s'", error.c_str());
}

static void
getxml(const char **ap) {
  if (!ap[0])
    bad("No output filename specified for getxml");
  FILE *ofp = fopen(ap[0], "w");
  if (!ofp)
    bad("Cannot open output file: \"%s\"", ap[0]);
  std::vector<char> xml;
  std::string err;
  if (dev->getMetadata(xml, err))
    bad(err.c_str());
  if (fwrite(&xml[0], xml.size(), 1, ofp) != 1 ||
      fclose(ofp))
    bad("error writing xml file '%s' size %zu", ap[0], xml.size());
}

// We create a worker class that does not have to be part of anything else.
class Worker : public OC::Worker, public OH::WciControl {
  std::string m_name, m_wName;
public:
  Worker(ezxml_t impl, ezxml_t inst, const char *idx) 
    : OC::Worker(NULL, impl, inst, NULL, false),
      OH::WciControl(*dev, impl, inst, properties(), false),
      m_name(ezxml_cattr(inst, "name")),
      m_wName(ezxml_cattr(impl, "name"))
  {
    // We need to initialize the status of the worker since the OC::Worker class
    // object is being created without knowledge of previous state.
    // The worker's status register tells us the last control operation
    // that was performed.  It also has a sticky indication of
    // errors from the worker itself, but it doesn't remember whether the
    // previous control operation failed for other reasons (FIXME: the OCCP should
    // capture this information).  We do our best here by first bypassing the software.
    worker = atoi(idx);
    cAccess->offsetRegisters(wAccess, (intptr_t)(&((OH::OccpSpace*)0)->worker[worker]));
    uint32_t
      control = wAccess.get32Register(control, OH::OccpWorkerRegisters),
      status =  wAccess.get32Register(status, OH::OccpWorkerRegisters);
    OU::Worker::ControlState cs;
    OU::Worker::ControlOperation lastOp =
      (OU::Worker::ControlOperation)OCCP_STATUS_LAST_OP(status);
    if (!(control & OCCP_WORKER_CONTROL_ENABLE))
      cs = OU::Worker::EXISTS; // there is no specific reset state since it isn't hetero
    else if (!(status & OCCP_STATUS_CONFIG_OP_VALID) || lastOp == 4)
      cs = OU::Worker::EXISTS; // no control op since reset
    else if (status & OCCP_STATUS_CONTROL_ERRORS)
      cs = OU::Worker::UNUSABLE;
    else if (lastOp == OU::Worker::OpRelease)
      cs = OU::Worker::UNUSABLE;
    else if (status & OCCP_STATUS_FINISHED)
      cs = OU::Worker::FINISHED;
    else
      switch(lastOp) {
      case OU::Worker::OpInitialize: cs = OU::Worker::INITIALIZED; break;
      case OU::Worker::OpStart: cs = OU::Worker::OPERATING; break;
      case OU::Worker::OpStop: cs = OU::Worker::SUSPENDED; break;
      default:
	cs = OU::Worker::OPERATING;
	// FIXME:  the beforeQuery, and AfterConfig and test ops screw us up here.
      }
    setControlState(cs);
  }
  void control(const char *op) {
    bool ignored = false;
    uint32_t c =  wAccess.get32Register(control, OH::OccpWorkerRegisters);
    if (!strcasecmp(op, "reset")) {
      if (OCCP_WORKER_CONTROL_ENABLE & c) {
	// Force the last control op to be bad
	c &= ~OCCP_WORKER_CONTROL_ENABLE;
	wAccess.set32Register(control, OH::OccpWorkerRegisters,
			      c | OCCP_CONTROL_CLEAR_ATTENTION | OCCP_CONTROL_CLEAR_ERRORS);
      } else
	ignored = true;
    } else if (!strcasecmp(op, "unreset")) {
      if (OCCP_WORKER_CONTROL_ENABLE & c)
	ignored = true;
      else {
	wAccess.set32Register(control, OH::OccpWorkerRegisters, c |= OCCP_WORKER_CONTROL_ENABLE);
	wAccess.get32Register(test, OH::OccpWorkerRegisters);
	wAccess.set32Register(control, OH::OccpWorkerRegisters,
			      c | OCCP_CONTROL_CLEAR_ATTENTION | OCCP_CONTROL_CLEAR_ERRORS);
      }
    } else {
      unsigned i;
      for (i = 0; ops[i]; i++) 
	if (!strcasecmp(ops[i], op)) {
	  ignored = controlOp((OU::Worker::ControlOperation)i);
	  break;
	}
      if (!ops[i])
	bad("Unknown control operation: %s", op);
    }
    if (ignored)
      printf("The '%s' operation was IGNORED on instance '%s' of worker '%s'.\n",
	     op, name().c_str(), implTag().c_str());
    else
      printf("The '%s' operation was performed on instance '%s' of worker '%s'.\n",
	     op, name().c_str(), implTag().c_str());
  }
  void status() {
    printf("Status of instance '%s' of worker '%s' is '%s'\n",
	   name().c_str(), implTag().c_str(),
	   wAccess.get32Register(control, OH::OccpWorkerRegisters) & OCCP_WORKER_CONTROL_ENABLE ?
	   OU::Worker::s_controlStateNames[getState()] : "RESET");
    wdump(NULL);
  }
  OC::Port *findPort(const char *) { return NULL; }
  const std::string &name() const { return m_name; }
  void prepareProperty(OU::Property &, volatile void *&, const volatile void *&) {}
  OC::Port &createPort(const OU::Port &, const OU::PValue *) {
    return *(OC::Port*)NULL;
  }
  OC::Port &createOutputPort(OU::PortOrdinal, size_t, size_t, 
			     const OU::PValue*)
    throw (OU::EmbeddedException)
  {
    return *(OC::Port*)NULL;
  }
  OC::Port & createInputPort(OU::PortOrdinal, size_t, size_t, const OU::PValue*)
    throw (OU::EmbeddedException)
  {
    return *(OC::Port*)NULL;
  }
  OC::Application *application() { return NULL;}
  OC::Worker *nextWorker() { return NULL; }
  void read(size_t, size_t, void *) {}
  void write(size_t, size_t, const void *) {}
};

static ezxml_t
getmeta() {
  static std::vector<char> xml;
  std::string err;
  if (dev->getMetadata(xml, err))
    bad(err.c_str());
  ezxml_t x = ezxml_parse_str(&xml[0], xml.size());
  if (x && ezxml_error(x)[0])
    bad("XML Parsing error: %s", ezxml_error(x));
  if (!x || !x->name)
    bad("XML configuration in bitstream could not be parsed as XML");
  return x;
}

struct Arg {
  ezxml_t top;
  size_t index;
  char   letter;
  const char  *name, *prop, *value;
  bool control, status;
  const char **args;
  unsigned count;
  Arg(const char **args)
    : index(999), letter(0), name(NULL), control(false), status(false), args(args), count(0) {
    top = getmeta();
    prop = args[0] ? args[1] : NULL;
    value = args[0] && args[1] ? args[2] : NULL;
  }
  void
  setup() {
    if (isdigit(**args))
      index = atoi(*args);
    else
      name = *args;
  }

  void
  doWorker(ezxml_t inst) {
    count++;
    const char
      *name = ezxml_attr(inst, "name"),
      *idx = ezxml_attr(inst, "occpIndex"),
      *wkr = ezxml_attr(inst, "worker");
    ezxml_t impl = OX::findChildWithAttr(top, "worker", "name", wkr);
    if (!impl)
      bad("Can't find worker '%s' for instance '%s'", name, wkr);
    Worker *w = NULL;
    if (!idx) {
      if (control || status || value || prop)
	bad("Can't to control/status/get/put on workers with no control interface");
    } else {
      w = new Worker(impl, inst, idx);
      if (control)
	w->control(prop);
      else if (status)
	w->status();
      else if (value) {
	// Setting a property
	w->setProperty(prop, value);
	printf("Setting the %s property to '%s' on instance '%s'\n",
	       prop, value, name);
	fflush(stdout);
      }
    }
    printf("  Instance %s of %s worker %s (spec %s)",
	   name, strcasecmp(inst->name, "instance") ? inst->name :
	   idx && !strcmp(idx, "0") ? "platform" : "normal",
	   wkr, wkr ? ezxml_cattr(impl, "specname") : "<none>");
    if (idx)
      printf(" with index %s", idx);
    printf("\n");
    fflush(stdout);
    std::string pname, value;
    bool unreadable;
    if (prop) {
      unsigned i = w->whichProperty(prop);
      w->getProperty(i, pname, value, &unreadable, hex);
      printf("%3u %20s: %s\n", i, pname.c_str(),
	     unreadable ? "<unreadable>" : value.c_str());
    } else if (verbose && w) {
      for (unsigned i = 0; w->getProperty(i, pname, value, &unreadable, hex); i++)
	printf("%3u %20s: %s\n", i, pname.c_str(),
	       unreadable ? "<unreadable>" : value.c_str());
    }
    fflush(stdout);
  }

  const char *
  tryWorker(ezxml_t x) {
    const char *err;
    if ((!strcasecmp(x->name, "instance") ||
	 !strcasecmp(x->name, "adapter") ||
	 !strcasecmp(x->name, "io") ||
	 !strcasecmp(x->name, "interconnect"))) {
      size_t iindex;
      bool ifound = false;
      if ((err = OX::getNumber(x, "occpIndex", &iindex, &ifound)))
	return err;
      const char *iname = ezxml_cattr(x, "name");
      if ((letter && iname && iname[0] == letter) ||
	  (name && iname && !strcasecmp(name, iname)) ||
	  (!letter && !name && ifound && index == iindex))
	doWorker(x);
      else if (name && iname && !strcasecmp(name, ezxml_cattr(x, "worker"))) {
	// Check for a duplicate, among all children
	for (ezxml_t c = top->child; c; c = c->ordered)
	  if (x != c) {
	    const char *w = ezxml_cattr(c, "worker");
	    const char *n = ezxml_cattr(c, "name");
	    if (w && !strcasecmp(w, name))
	      bad("More than one instance (%s and %s) matches worker '%s'",
		  iname, n, name);
	  }
	doWorker(x);
      }
    }
    return NULL;
  }
  static const char *
  doWorkerArg(ezxml_t x, void *arg) {
    return ((Arg *)arg)->tryWorker(x);
  }
  const char *
  doWorkers() {
    count = 0;
    const char *err = OX::ezxml_children(top, doWorkerArg, (void*)this);
    if (err && err[0])
      bad("Error: %s", err);
    if (!count && !letter)
      bad("No workers found matching worker/instance argument: %s", args[0]);
    return NULL;
  }
};

static void
get(const char **ap) {
  Arg arg(ap);
  if (*ap) {
    if (ap[1] && ap[2])
      bad("The 'get' command takes 0, 1 or 2 args: [instance-or-worker [property-name]]");
    arg.setup();
    arg.doWorkers();
  } else {
    printf("HDL Device: '%s' is platform '%s' part '%s' and UUID '%s'\n",
	   dev->name().c_str(), ezxml_cattr(arg.top, "platform"), ezxml_cattr(arg.top, "device"),
	   ezxml_cattr(arg.top, "uuid"));
    printf("Platform configuration workers are:\n");
    arg.letter = 'p';
    arg.doWorkers();
    printf("Container workers are:\n");
    arg.letter = 'c';
    arg.doWorkers();
    printf("Application workers are:\n");
    arg.letter = 'a';
    arg.doWorkers();
  }
}

// Set a property value
static void
set(const char **ap) {
  Arg arg(ap);
  if (!ap[0] || !ap[1] || !ap[2])
    bad("The 'set' command takes three arguments: instance-or-worker property-name property-value");
  arg.setup();
  arg.doWorkers();
}

// Perform a control operation
static void
control(const char **ap) {
  Arg arg(ap);
  if (!ap[0] || !ap[1] || ap[2])
    bad("The 'control' command takes two arguments: instance-or-worker [reset|unreset|start|stop]");
  arg.setup();
  arg.control = true;
  arg.doWorkers();
}

// Display a worker's status
static void
status(const char **ap) {
  Arg arg(ap);
  if (!ap[0] || ap[1])
    bad("The 'status' command takes one argument: instance-or-worker");
  arg.setup();
  arg.status = true;
  arg.doWorkers();
}

static void
load(const char **ap) {
  // Some devices can be loaded even when they don't probe, so we open the device
  // specially/softly here...
  if (!device &&
      !(device = getenv("OCPI_DEFAULT_HDL_DEVICE")))
    bad("a device option is required with this command");
  if (!ap[0])
    bad("No input filename specified for loading");
  
  driver = &OCPI::HDL::Driver::getSingleton();
  std::string error;
  if (!(dev = driver->open(device, false, true, NULL, error)))
    bad("error opening device %s", device);
  dev->load(ap[0]);
}
static void
unload(const char **) {
  dev->unload();
}
