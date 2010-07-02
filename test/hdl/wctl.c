#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
typedef struct {
  uint32_t 
    CPI,
    Open,
    revision,
    birthday,
    nworkers,
    pci_id,
    attention,
    mbz0,
    scratch20,
    scratch24,
    counter,
    status,
    msi_addr_hi,
    msi_addr_low,
    msi_data,
    mbz1;
} OCCP_Admin;

typedef struct {
  uint32_t
    initialize,
    start,
    stop,
    release,
    test,
    before_query,
    after_config,
    reserved7,
    status,
    control,
    config_addr,
    pad[5];
} OCCP_WorkerControl;

#define OCCP_WORKER_CONTROL_SIZE (1<<16)
#define OCCP_NWORKERS 15

typedef struct {
  uint8_t config[OCCP_WORKER_CONTROL_SIZE - sizeof(OCCP_WorkerControl)];
  OCCP_WorkerControl control;
} OCCP_Worker;

typedef struct {
  OCCP_Admin admin;
  uint8_t pad[OCCP_WORKER_CONTROL_SIZE - sizeof(OCCP_Admin)];
  OCCP_Worker worker[OCCP_NWORKERS];
} OCCP_Space;


typedef int func(OCCP_Space *, char **, OCCP_Worker *);
static func admin, wdump, wread, wwrite, wadmin, wop, wwctl, dtest, dmeta;

typedef struct {
  char *name;
  func *command;
  int   worker;
} OCCP_Command;

static OCCP_Command commands[] = {
  {"admin", admin},	// dump admin
  {"wdump", wdump, 1},  // dump worker controls
  {"wread", wread, 1},  // read worker config
  {"wwrite", wwrite, 1},// write worker config
  {"wadmin", wadmin},   // write admin space
  {"wop", wop, 1},      // do control op
  {"wwctl", wwctl, 1},  // write worker control register
  {"dtest", dtest},     // Perform test on data memory
  {"dmeta", dmeta},     // Dump metadata
  {0}
};

 static int
atoi_any(char *arg, uint8_t *sizep)
{
  int value ;

  if(strncmp(arg,"0x",2) != 0)
    sscanf(arg,"%u",&value) ;
  else
    sscanf(arg,"0x%x",&value) ;
  if (sizep) {
    char *sp;
    if ((sp = strchr(arg, '/')))
      switch(*++sp) {
      default:
	fprintf(stderr, "Bad size specifier: must be 1, 2, or 4");
	abort();
      case '1':
      case '2':
      case '4':
	*sizep = *sp - '0';
      }
    else
      *sizep = 4;
  }
  return value ;
}

  int
 main(int argc, char **argv)
 {
   int fd;
   off_t off;
   OCCP_Space *p;
   OCCP_Command *c;
   char **ap = argv + 1;


   assert(argc >= 3);
   errno = 0;
   off = strtoul(*ap++, NULL, 16);
   assert(errno == 0);
   assert((fd = open("/dev/mem", O_RDWR)) != -1);
   assert ((p = mmap(NULL, sizeof(OCCP_Space), PROT_READ|PROT_WRITE, MAP_SHARED, fd, off)) != (typeof(p))-1);
   for (c = commands; c->name; c++)
     if (strcmp(c->name, *ap) == 0) {
       OCCP_Worker *w = 0;
       if (c->worker) {
	 ap++;
	 unsigned n = atoi(*ap ? *ap : "");

	 if (n < 1 || n > OCCP_NWORKERS) {
	   fprintf(stderr, "Worker number `%s' invalid\n", *ap ? *ap : "<null>");
	   return 1;
	 }
	 w = &p->worker[n  - 1];
       }
       return c->command(p, ++ap, w);
     }
   fprintf(stderr, "wctl: unknown command: %s\n", *ap);
   return 1;
 }

 static int
admin(OCCP_Space *p, char **ap, OCCP_Worker *w)
{
  time_t epochtime;
  struct tm *etime;
  epochtime = (time_t)p->admin.birthday;
  etime = gmtime(&epochtime); 

  printf("OCCP Admin Space\n");
  printf(" CPI:          0x%08x\n", p->admin.CPI);
  printf(" Open:         0x%08x\n", p->admin.Open);
  printf(" revision:     0x%08x\n", p->admin.revision);
  printf(" birthday:     0x%08x  %s", p->admin.birthday, asctime(etime));
  printf(" nworkers:     0x%08x\n", p->admin.nworkers);
  printf(" pci_id:       0x%08x\n", p->admin.pci_id);
  printf(" attention:    0x%08x\n", p->admin.attention);
  printf(" mbz0:         0x%08x\n", p->admin.mbz0);
  printf(" scratch20:    0x%08x\n", p->admin.scratch20);
  printf(" scratch24:    0x%08x\n", p->admin.scratch24);
  printf(" counter:      0x%08x\n", p->admin.counter);
  printf(" status:       0x%08x\n", p->admin.status);
  printf(" msi_addr_hi:  0x%08x\n", p->admin.msi_addr_hi);
  printf(" msi_addr_low: 0x%08x\n", p->admin.msi_addr_low);
  printf(" msi_data:     0x%08x\n", p->admin.msi_data);
  printf(" mbz1:         0x%08x\n", p->admin.mbz1);
  return 0;
}

 static int
wadmin(OCCP_Space *p, char **ap, OCCP_Worker *w)
{
  unsigned off = atoi_any(*ap++, 0);
  unsigned val = atoi_any(*ap, 0);
  uint32_t *pv = (uint32_t *)((uint8_t *)&p->admin + off);

  printf("Admin space, offset 0x%x, writing value: 0x%x\n", off, val);
  *pv = val;
  return 0;
}

static int
wread(OCCP_Space *p, char **ap, OCCP_Worker *w)
{
  uint8_t size;
  unsigned val, off = atoi_any(*ap, &size);
  uint32_t *p32 = (uint32_t *)&w->config[off];
  switch(size) {
  case 1:
    val = *((uint8_t *)p32);
    break;
  case 2:
    val = *((uint16_t *)p32);
    break;
  default:
    val = *p32;
  }
  printf("Worker %ld, offset 0x%x(%d), value: 0x%x\n", (w - p->worker) + 1, off, size, val);
  return 0;
}

static struct {
  char *name;
  unsigned off;
} ops[] = {
  {"initialize", (unsigned)&((OCCP_WorkerControl *)0)->initialize },
  {"start", (unsigned)&((OCCP_WorkerControl *)0)->start },
  {"stop", (unsigned)&((OCCP_WorkerControl *)0)->stop },
  {"release", (unsigned)&((OCCP_WorkerControl *)0)->release },
  {"test", (unsigned)&((OCCP_WorkerControl *)0)->test },
  {"before", (unsigned)&((OCCP_WorkerControl *)0)->before_query },
  {"after", (unsigned)&((OCCP_WorkerControl *)0)->after_config },
  {"reserved7", (unsigned)&((OCCP_WorkerControl *)0)->reserved7 },
  {0}
};

static int
wop(OCCP_Space *p, char **ap, OCCP_Worker *w)
{
  unsigned i;
  for (i = 0; ops[i].name; i++)
    if (*ap && strcmp(*ap, ops[i].name) == 0) {
      unsigned off = ops[i].off;
      uint32_t v, *cp = (uint32_t *)((uint8_t *)&w->control + off);
      printf("Worker %ld control op: %s(%x)\n", (w - p->worker) + 1, *ap, off);
      v = *cp;
      printf("Result: 0x%08x\n", v);
      return 0;
  }
  fprintf(stderr, "Unknown control operation: `%s'\n", *ap);
  return 1;
}

static int
wwrite(OCCP_Space *p, char **ap, OCCP_Worker *w)
{
  uint8_t size;
  unsigned off = atoi_any(*ap++, &size);
  unsigned val = atoi_any(*ap, 0);
  if ((0xffffffffu >> (32 - size*8)) & (val & (-1<<(size*8)))) {
    fprintf(stderr, "Value `%s' too large for size (%d)\n", *ap, size);
    return 1;
  }
  uint32_t *p32 = (uint32_t *)&w->config[off];
  printf("Worker %ld, offset 0x%x, writing value: 0x%x\n", (w - p->worker) + 1, off, val);
  
  switch(size) {
  case 1:
    *((uint8_t *)p32) = val;
    break;
  case 2:
    *((uint16_t *)p32) = val;
    break;
  default:
    *p32 = val;
  }
  return 0;
}

static int
wwctl(OCCP_Space *p, char **ap, OCCP_Worker *w)
{
  unsigned val = atoi_any(*ap, 0);
  uint32_t *p32 = (uint32_t *)&w->control.control;
  printf("Worker %ld, writing control register value: 0x%x\n", (w - p->worker) + 1, val);
  
  *p32 = val;
  return 0;
}

static int
wdump(OCCP_Space *p, char **ap, OCCP_Worker *w)
{
  printf("Worker %ld\n", (w - p->worker) + 1);
  printf(" Status:     0x%08x\n", w->control.status);
  printf(" Control:    0x%08x\n", w->control.control);
  printf(" ConfigAddr: 0x%08x\n", w->control.config_addr);
  return 0;
}

 static int
dtest(OCCP_Space *p, char **ap, OCCP_Worker *w)
{
  unsigned val = atoi_any(*ap, 0);
  uint32_t *pv = (uint32_t *)((uint8_t *)&p->admin);
  unsigned int exp, got, i, j, k;
  int errors = 0;
  
  for (k=0;k<val;k++) {
    for (i=0;i<0x1000;i++) {
      pv[i] = i;
    }
    for (j=0;j<0x1000;j++) {
      exp = j;
      got = pv[j];
    if (got!=exp) {
      printf("Mismatch: Word:%x Exp:%x, Got:%x\n", j, exp, got);
      errors++;
      }
    }
  }

  if (errors==0) printf("Success\n");
  else           printf("%d  Errors\n", errors);
  return 0;
}

 static int
dmeta(OCCP_Space *p, char **ap, OCCP_Worker *w)
{
  unsigned val = atoi_any(*ap, 0);
  uint32_t *pv = (uint32_t *)((uint8_t *)&p->admin);
  unsigned int exp, got, i, j, k, mb, b, dp;
  int errors = 0;
  
  // Metadata offset is 8KB into the BRAM buffer (0x800 DWORDS)
  // Dataplanes each take up 32KB (0x2000 DWORDS)

  for (dp=0;dp<2;dp++)
    for (mb=0;mb<4;mb++)  {
      b = 0x800 + (mb*4) + (dp*0x2000);
      printf("dp:%d metabuf:%x: len:%x, op:%x, tag:%x, deltime:%x \n",
          dp, mb, pv[b+0], pv[b+1], pv[b+2], pv[b+3]);
    }

  return 0;
}


