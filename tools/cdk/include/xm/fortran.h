
/*
FORTRAN.H: FORTRAN System Library Header File

Copyright 1988-91 PROMULA Development Corporation ALL RIGHTS RESERVED

Author: Fred Goodman

Description:

Whenever any functions in the PROMULA.FORTRAN runtime library are used
or compiled this header MUST be included and one of the following symbols
must must be defined: SPROTOTYPE, LPROTOTYPE, SKANDRCCOM, LKANDRCCOM,
VMSCCOMPIL,COHCCOMPIL, or HXLCCOMPIL.
*/
#ifdef SPROTOTYPE  /* Output expecting short integers and ANSI C prototypes */
#define FPROTOTYPE
typedef int SHORT;
typedef long LONG;
typedef unsigned short USHORT;
typedef unsigned long ULONG;
#define VOID void
#endif

#define NULL 0

#ifdef LPROTOTYPE  /* Output expecting long integers and ANSI C prototypes */
#define FPROTOTYPE
typedef short SHORT;
typedef int LONG;
typedef unsigned short USHORT;
typedef unsigned long ULONG;
#define VOID void
#endif

#ifdef SKANDRCCOM /* Output expecting short integers and K+R C compiler */
#define SHORT   int
#define LONG    long
#define USHORT  unsigned int
#define ULONG   unsigned long
#define VOID void
#endif

#ifdef LKANDRCCOM /* Output expecting long integers and K+R C compiler */
#define SHORT   short
#define LONG    int
#define USHORT  unsigned short
#define ULONG   unsigned int
#define VOID void
#endif

#ifdef COHCCOMPIL /* Output is a compilation for COHERENT */
#define SHORT   short
#define LONG    int
#define USHORT  unsigned short
#define ULONG   unsigned int
#define VOID char
#endif

#ifdef HXLCCOMPIL /* Output is a compilation for an HPXL */
#pragma INTRINSIC ACTIVATE,CALENDAR,CCODE,CLOCK,COMMAND,CREATE,FCLOSE
#pragma INTRINSIC FCONTROL,FMTCALENDAR,FREAD,FSETMODE,FWRITE,FOPEN,GETPRIVMODE
#pragma INTRINSIC GETUSERMODE,KILL,IOWAIT,PAUSE,WHO,FGETINFO
#define SHORT   short
#define LONG    int
#define USHORT  unsigned short
#define ULONG   unsigned int
#define VOID void
#endif

#ifdef VMSCCOMPIL  /* Output is for VMS C compiler */
typedef short SHORT;
typedef int LONG;
typedef unsigned short USHORT;
typedef unsigned int ULONG;
#define VOID void
#endif

#ifdef VMSCCOMPIL
#include <stdio.h>
#include <math.h>
#include <setjmp.h>
#else
#include "stdio.h"
#include "math.h"
#include "setjmp.h"
#endif

#ifdef FPROTOTYPE
#include "stdlib.h"
#include "string.h"
#endif
/*
Define the Virtual File System parameters
*/
typedef struct {
    int vbdel;
    int vblru;
    int vbmru;
    int vbblk;
    int vbhan;
    char vbdat[1024];
} vmsbtyp;
/*
Define the descriptor format.
*/
typedef struct {
    unsigned short Dleng;
    unsigned char Dtype;
    unsigned char Dclass;
    char* Dptr;
} DESCRIPTOR;
/*
Define the namelist structure
*/
typedef struct {
    char* nmname;
    VOID* nmvalu;
    int nmtype;
    int* nmadr;
} namelist;
/*
Define the string type
*/
typedef struct {
    char* a;                 /* Pointer to the character storage */
    int n;                   /* Length of the string */
} string;
/*
Define the single precision complex type
*/
typedef struct {
    float cr;                /* The real part of the value */
    float ci;                /* The imaginary part of the number */
} complex;
/*
Define the double precision complex type
*/
typedef struct {
    double dr;               /* The real part of the value */
    double di;               /* The imaginary part of the number */
} dcomplex;
/*
Define the long to short pointer conversion function
*/
#ifdef LITTLENDER
#define ftnsht2(x) ((short*)(x))
#else
#define ftnsht2(x) ((short*)(x)+1)
#endif
/*
Define the constants used in the translation of the auxilliary I/O statements
for the C and FORTRAN user biases.
*/
#define TRUE          1
#define FALSE         0
#define OPEN          ftnopen
#define PAUSE         ftnpause
#define REWIND        ftnrew
#define ENDFILE       ftnwef
#define BACKSPACE     ftnback
#define CLOSE         ftnclose
#define DELETE        ftndelet
#define LOCK          ftnlock
#define UNLOCK        ftnunlock
#define RNAMELIST     rnmlist
#define WNAMELIST     wnmlist
#define STOP          ftnstop
#define LUN           ftnlun
#define IOSTAT        1
#define FILEN         2
#define STATUS        3
#define ACCESS        4
#define FORM          5
#define RECL          6
#define BLANK         7
#define READONLY      8
#define SHARED        9
#define RECTYPE      10
#define CARRCON      11
#define ASVARIABLE   12
#define RECORDS      13
#define ANYUNIT      14
#define ASBUFFER     15
#define BLOCKSIZE    16
#define OTHER        17
#define INDEXKEY     18
#define ORGANIZATION 19
#define USEROPEN     20
#define DISPOSE      21
#define BUFFERCOUNT  22
#define ERRORLOG     23
#define RECFM        24
#define MAXREC       25
/*
Define the constants used in the READ and WRITE statements.
*/
#define  WRITE        ftnwrit
#define  READ         ftnread
#define  NAMED        -9994
#define  CONSOLE      -9995
#define  PRINTER      -9996
#define  INTERNAL     -9997
#define  INPUT        -9998
#define  OUTPUT       -9999
#define  LISTIO       2
#define  FMT          3
#define  VFMT         4 
#define  REC          5
#define  MORE         9
#define  DO          10
#define  INT2        11
#define  REAL8       12
#define  LOG2        13
#define  BYTE        14
#define  INT4        15
#define  REAL4       16
#define  LOG4        17
#define  LOG1        18
#define  CMPLX       19
#define  STRG        20
#define  CSTR        21
#define  DCMPLX      22
/*
Define the constants needed for trig function conversions
*/
#define F_PI   3.141592654
#define F_HPI  1.570796327
#define F_R2D  57.295779513
#define F_D2R  1.745329252E-02
#define D_PI   3.141592653589793238462643
#define D_HPI  1.570796326794896619231322
#define D_R2D  57.295779513082320876798155
#define D_D2R  1.7453292519943295769237E-02
/*
Define the "float" math functions
*/
#define fifabs(x)       ((float)fabs((double)(x)))
#define fifacos(x)      ((float)acos((double)(x)))
#define dacosd(x)       (acos(x)*D_R2D)
#define fifaint(x)      ((float)fifdint((double)(x)))
#define fiflog10(x)     ((float)log10((double)(x)))

#define fiflog(x)       ((float)log((double)(x)))
#define fifamax1(x,y)   ((float)fifdmax1((double)(x),(double)(y)))
#define fifamin1(x,y)   ((float)fifdmin1((double)(x),(double)(y)))
#define fifamod(x,y)    ((float)fifdmod((double)(x),(double)(y)))
#define fifanint(x)     ((float)fifdnint((double)(x)))
#define fifasin(x)      ((float)asin((double)(x)))
#define dasind(x)       (asin(x)*D_R2D)
#define fifatan(x)      ((float)atan((double)(x)))
#define fifatan2(x,y)   ((float)atan2((double)(x),(double)(y)))
#define datand(x)       (atan(x)*D_R2D)
#define datan2d(x,y)    (atan2(x,y)*D_R2D)
#define atan2d(x,y)     ((float)atan2((float)(x),(float)(y))*F_R2D)
#define fifcos(x)       ((float)cos((double)(x)))
#define fifcosh(x)      ((float)cosh((double)(x)))
#define fifdim(x,y)     ((float)fifddim((double)(x),(double)(y)))
#define fifexp(x)       ((float)exp((double)(x)))
#define fifiaint(x)     fifidint((double)(x))
#define fifi2aint(x)    fifi2dint((double)(x))
#define fifpow(x,y)     ((float)pow((double)(x),(double)(y)))
#define fifsign(x,y)    ((float)fifdsign((double)(x),(double)(y)))
#define fifsin(x)       ((float)sin((double)(x)))
#define fifsinh(x)      ((float)sinh((double)(x)))
#define fifsqrt(x)      ((float)sqrt((double)(x)))
/* 
Fixes for LINUX 
*/
#define fifdsind(x)     sin(x*D_D2R)
#define fifdcosd(x)     cos(x*D_D2R)
#define fifdtand(x)     tan(x*D_D2R)
/*
Define the unaligned memory functions
*/
#ifdef FPROTOTYPE
USHORT get16u(char* unl);
SHORT get16s(char* unl);
ULONG get32u(char* unl);
LONG get32s(char* unl);
float getflt(char* unl);
double getdbl(char* unl);
complex getcpx(char* unl);
dcomplex getdpx(char* unl);
void put16u(char* unl,USHORT val);
void put16s(char* unl,SHORT val);
void put32u(char* unl,ULONG val);
void put32s(char* unl,LONG val);
void putflt(char* unl,float val);
void putdbl(char* unl,double val);
void putcpx(char* unl,complex val);
void putdpx(char* unl,dcomplex val);
#else
USHORT get16u();
SHORT get16s();
ULONG get32u();
LONG get32s();
float getflt();
double getdbl();
complex getcpx();
dcomplex getdpx();
void put16u();
void put16s();
void put32u();
void put32s();
void putflt();
void putdbl();
void putcpx();
void putdpx();
#endif

#ifdef FPROTOTYPE
float fiftan(float x);
#else
float fiftan();
#endif
#define fiftanh(x)      ((float)tanh((double)(x)))
/*
Define the simple integer functions
*/
#define fifichar(x)   (*(unsigned char*)(x))
#define fifmod(x,y)   ((x) % (y))
#define fifi2mod(x,y)   ((x) % (y))
/*
Define the memory functions
*/
#define fifmalloc(x) malloc((int)(x))
#define fifmemc(x,y,z) memcmp(x,y,z)
#define fifcopy(x,y,z) memcpy(y,x,z)
/*
Define the small integer power functions.
*/
#define fifasqr(x)      ((x)*(x))
#define fifacube(x)     ((x)*(x)*(x))
#define fifdsqr(x)      ((x)*(x))
#define fifdcube(x)     ((x)*(x)*(x))
#define fifi2sqr(x)     ((x)*(x))
#define fifi2cube(x)    ((x)*(x)*(x))
#define fifisqr(x)      ((x)*(x))
#define fificube(x)     ((x)*(x)*(x))
#define cpxsqr(x)       cpxmul((x),(x))
#define cpxcube(x)      cpxmul((x),cpxmul((x),(x)))
#define dpxsqr(x)       dpxmul((x),(x))
#define dpxcube(x)      dpxmul((x),dpxmul((x),(x)))
/*
Define the complex functions in the runtime library
*/
#ifdef FPROTOTYPE
complex cpxcos(complex a);
complex cpxexp(complex a);
complex cpxlog(complex a);
complex cpxlog10(complex a);
complex cpxsin(complex a);
complex cpxsroot(complex a);
dcomplex dpxcos(dcomplex a);
dcomplex dpxexp(dcomplex a);
dcomplex dpxlog(dcomplex a);
dcomplex dpxlog10(dcomplex a);
dcomplex dpxsin(dcomplex a);
dcomplex dpxsroot(dcomplex a);
float cpxabs(complex a);
complex cpxadd(complex a, complex b);
complex cpxcjg(complex a);
int cpxcmp(complex a, complex b);
complex cpxcpx(float d1, float d2);
complex cpxdbl(double dbl);
complex cpxdiv(complex a, complex b);
complex cpxdpx(dcomplex dbl);
float cpxima(complex a);
LONG cpxlong(complex a);
complex cpxmul(complex a, complex b);
complex cpxneg(complex a);
complex cpxpow(complex a, complex b);
float cpxreal(complex a);
complex cpxsub(complex a, complex b);
double dpxabs(dcomplex a);
dcomplex dpxadd(dcomplex a, dcomplex b);
dcomplex dpxcjg(dcomplex a);
int dpxcmp(dcomplex a, dcomplex b);
dcomplex dpxcpx(complex sng);
dcomplex dpxdbl(double dbl);
dcomplex dpxdiv(dcomplex a, dcomplex b);
dcomplex dpxdpx(double d1, double d2);
double dpxima(dcomplex a);
LONG dpxlong(dcomplex a);
dcomplex dpxmul(dcomplex a, dcomplex b);
dcomplex dpxneg(dcomplex a);
dcomplex dpxpow(dcomplex a, dcomplex b);
double dpxreal(dcomplex a);
dcomplex dpxsub(dcomplex a, dcomplex b);
#else
float cpxabs();
complex cpxadd();
complex cpxcjg();
int cpxcmp();
complex cpxcos();
complex cpxcpx();
complex cpxdbl();
complex cpxdiv();
complex cpxdpx();
complex cpxexp();
float cpxima();
complex cpxlog();
complex cpxlog10();
LONG cpxlong();
complex cpxmul();
complex cpxneg();
complex cpxpow();
float cpxreal();
complex cpxsin();
complex cpxsroot();
complex cpxsub();
double dpxabs();
dcomplex dpxadd();
dcomplex dpxcjg();
int dpxcmp();
dcomplex dpxcos();
dcomplex dpxcpx();
dcomplex dpxdbl();
dcomplex dpxdiv();
dcomplex dpxdpx();
dcomplex dpxexp();
double dpxima();
dcomplex dpxlog();
dcomplex dpxlog10();
LONG dpxlong();
dcomplex dpxmul();
dcomplex dpxneg();
dcomplex dpxpow();
double dpxreal();
dcomplex dpxsin();
dcomplex dpxsroot();
dcomplex dpxsub();
#endif
/*
Define the type preserving min and max functions
*/
#define fifmax0(a,b)   (((a) > (b)) ? (a) : (b))
#define fifi2max0(a,b) (((a) > (b)) ? (a) : (b))
#define fifmin0(a,b)   (((a) < (b)) ? (a) : (b))
#define fifi2min0(a,b) (((a) < (b)) ? (a) : (b))
/*
Define the remaining functions
*/
#ifdef FPROTOTYPE
float fifacosd(float x);
void fifalloc(int* stat,...);
float fifamax0(LONG a1, LONG a2);
float fifamin0(LONG a1, LONG a2);
void fifasc50(int icnt, VOID* input, VOID* output);
float fifasind(float x);
float fifatand(float x);
int fifbtest(ULONG bits, LONG ibit);
void fifchain(char* prog,int nprog);
char* fifchar(int iv);
void fifdate(char* cl, int ncl);
double fifddim(double a1, double a2);
double fifdint(double a);
double fifdmax1(double a1, double a2);
double fifdmin1(double a1, double a2);
double fifdmod(double num, double dem);
double fifdnint(double a);
double fifdsign(double mag, double sgn);
double fifatn2d(double,double);
int fifeqf(float a, float b);
void fifexit(LONG* numb);
void fiffree(int* stat,...);
void fifgetcl(char* cl, int ncl);
void fifgetarg(LONG k, char* cl, int ncl);
void fifgetenv(char *ename,  int lenin, char *evalue, int lenout);
LONG fifiargc();
SHORT fifi2abs(SHORT a);
void fifi2date(SHORT* mm, SHORT* dd, SHORT* yy);
SHORT fifi2dim(SHORT a1, SHORT a2);
SHORT fifi2dint(double a);
SHORT fifi2nint(float a);
LONG fifi2pow(SHORT a, SHORT b);
SHORT fifi2shf(SHORT a, int n);
SHORT fifi2sign(SHORT mag, SHORT sgn);
LONG fifi4log(LONG a);
LONG fifiabs(LONG a);
void fifibit(unsigned char* bits, int ibit, int ival);
LONG fifidim(LONG a1, LONG a2);
LONG fifidint(double a);
int fifindex(char* s, int ns, char* c, int nc);
LONG fifipow(LONG a, LONG b);
LONG fifishf(LONG a, int n);
LONG fifisign(LONG mag, LONG sgn);
LONG fifmax1(float a1, float a2);
LONG fifmin1(float a1, float a2);
int fifnef(float a, float b);
LONG fifnint(double a);
int fifrad50(int icnt, VOID* input, VOID* output);
float fifran(LONG* seed);
float fifsecnd(float y);
float fifsind(float x);
float fiftand(float x);
float fifcosd(float x);
char* fifpushstr(char* str, int nc);
char* fifstrgv(char* str, int nc);
int fifsystm(char* cl, int ncl);
void fiftime(char* cl, int ncl);
char* fifxcrep(char* chrs);
int fioback(void);
int fioclose(void);
int fioerror(int clear);
void fiofdata(int option, VOID* str, int ns);
int fiofini(char** fmt, int nfmt);
void fiofinqu(int option, char* str, int ns);
LONG fiofvinq(int option);
int fiointu(char* intu, int rsize, int action);
int fiolun(int lun, int action);
int fioname(char* str, int ns, int status);
int fioopen(void);
int fiorbiv(VOID* value, int nvalue);
int fiordb(USHORT* bool, int nval);
int fiordc(char* c, int nval);
int fiordd(double* value, int nval);
int fiordf(float* value, int nval);
int fiordi(SHORT* value, int nval);
int fiordl(LONG* value, int nval);
int fiords(char* str, int nstring, int nval);
int fiordt(ULONG* bool, int nval);
int fiordu(unsigned char* c, int nval);
int fiordx(complex* value, int nval);
int fiordz(dcomplex* value, int nval);
void fiorec(int irec);
int fiorew(void);
int fiorln(void);
int fiornl(namelist* name, int nname, void* nlident);
void fiostatus(VOID* iostat, int error);
int fiostio(int action);
int fiostrdi(int* ftnstruc,char** ftnfdata,VOID** ftnrecrd);
void fiouwl(LONG* recl);
int fiovfini(char** fmt, int nfmt);
void fiovfmt(int nvfmt,...);
int fiowbiv(VOID* value, int nvalue);
int fiowef(void);
int fiowln(void);
int fiownl(namelist* name, int nname, char* nlident);
int fiowrb(USHORT* bool, int nval);
int fiowrc(char* c, int nval);
int fiowrd(double* value, int nval);
int fiowrf(float* value, int nval);
int fiowri(SHORT* value, int nval);
int fiowrl(LONG* value, int nval);
int fiowrs(char* str, int nstring, int nval);
int fiowrt(ULONG* bool, int nval);
int fiowru(unsigned char* c, int nval);
int fiowrx(complex* value, int nval);
int fiowvb(USHORT val);
int fiowvc(char c);
int fiowvd(double val);
int fiowvf(float val);
int fiowvi(SHORT val);
int fiowvl(LONG val);
int fiowvs(char* str, int nstring);
int fiowvt(ULONG val);
int fiowvu(unsigned char c);
int fiowvx(complex val);
int fiowvz(dcomplex val);
char* ftnads(char* s1, int n1, char* s2, int n2);
VOID* ftnalloc(LONG nbyte);
int ftnback(int lun,...);
void ftnblkd(void);
int ftnclose(int lun,...);
int ftncms(char* s1, int n1, char* s2, int n2);
void ftndallo(DESCRIPTOR* str,...);
void ftnfree(VOID* ptr);
void ftnini(int argc, char* argv[], char* defarg);
int ftnlock(int lun,...);
FILE* ftnlun(int lun);
int ftnopen(int lun,...);
void ftnpause(char* message, int nmes);
int ftnread(int lun,...);
int ftnrew(int lun,...);
void ftnsac(char* s1, int n1, char* s2, int n2);
void ftnsallo(string* str,...);
int ftnscomp(char* s1, int ns1,...);
void ftnscopy(char* s1, int ns1,...);
int ftnsleng(char* s1, int ns1,...);
void ftnssubs(string* subs, char* str, int slen, int ipos, int lpos);
void ftnstop(char* message);
int ftnunlock(int lun,...);
int ftnwef(int lun,...);
int ftnwrit(int lun,...);
char* ftnxcons(int base, char* xcons, int xlen, int vlen);
void vmscls(void);
unsigned char* vmsdel(LONG ioiad);
void vmsglob(int iop);
char* vmsload(LONG nbyte,LONG ioiad);
int vmsopn(int argc, char** argv);
int vmsrdb(LONG value, int nval);
int vmsrdc(LONG value, int nval);
int vmsrdd(LONG value, int nval);
int vmsrdf(LONG value, int nval);
int vmsrdi(LONG value, int nval);
int vmsrdl(LONG value, int nval);
int vmsrds(LONG value, int nstring, int nval);
int vmsrdt(LONG value, int nval);
int vmsrdu(LONG value, int nval);
void vmssave(char* svec, LONG nbyte, LONG ioiad);
unsigned char* vmsuse(LONG ioiad);
void vmsvect(int fd, char* vec, LONG nbyte, LONG ioiad, int iop);
int vmswrb(LONG value, int nval);
int vmswrc(LONG value, int nval);
int vmswrd(LONG value, int nval);
int vmswrf(LONG value, int nval);
int vmswri(LONG value, int nval);
int vmswrl(LONG value, int nval);
int vmswrs(LONG value, int nstring, int nval);
int vmswrt(LONG value, int nval);
int vmswru(LONG value, int nval);
#else
float fifacosd();
void fifalloc();
float fifamax0();
float fifamin0();
void fifasc50();
int fifbtest();
void fifchain();
char* fifchar();
void fifdate();
double fifddim();
double fifdint();
double fifdmax1();
double fifdmin1();
double fifdmod();
double fifdnint();
double fifdsign();
double fifatn2d();
int fifeqf();
void fifexit();
void fifgetcl();
void fifgetarg();
void fifgetenv();
void fiffree();
LONG fifiargc();
SHORT fifi2abs();
void fifi2date();
SHORT fifi2dim();
SHORT fifi2dint();
SHORT fifi2nint();
LONG fifi2pow();
SHORT fifi2shf();
SHORT fifi2sign();
LONG fifi4log();
LONG fifiabs();
void fifibit();
LONG fifidim();
LONG fifidint();
int fifindex();
LONG fifipow();
LONG fifishf();
LONG fifisign();
LONG fifmax1();
LONG fifmin1();
int fifnef();
LONG fifnint();
int fifrad50();
float fifran();
float fifsecnd();
float fifatand();
float fifasind();
float fiftand();
float fifsind();
float fifcosd();
char* fifpushstr();
char* fifstrgv();
int fifsystm();
void fiftime();
char* fifxcrep();
int fioback();
int fioclose();
int fioerror();
void fiofdata();
int fiofini();
void fiofinqu();
LONG fiofvinq();
int fiointu();
int fiolun();
int fioname();
int fioopen();
int fiorbiv();
int fiordb();
int fiordc();
int fiordd();
int fiordf();
int fiordi();
int fiordl();
int fiords();
int fiordt();
int fiordu();
int fiordx();
int fiordz();
void fiorec();
int fiorew();
int fiorln();
int fiornl();
void fiostatus();
int fiostio();
int fiostrdi();
void fiouwl();
int fiovfini();
void fiovfmt();
int fiowbiv();
int fiowef();
int fiowln();
int fiownl();
int fiowrb();
int fiowrc();
int fiowrd();
int fiowrf();
int fiowri();
int fiowrl();
int fiowrs();
int fiowrt();
int fiowru();
int fiowrx();
int fiowvb();
int fiowvc();
int fiowvd();
int fiowvf();
int fiowvi();
int fiowvl();
int fiowvs();
int fiowvt();
int fiowvu();
int fiowvx();
int fiowvz();
char* ftnads();
VOID* ftnalloc();
int ftnback();
void ftnblkd();
int ftnclose();
int ftncms();
void ftndallo();
void ftnfree();
void ftnini();
int ftnlock();
FILE* ftnlun();
int ftnopen();
void ftnpause();
int ftnread();
int ftnrew();
void ftnsac();
void ftnsallo();
int ftnscomp();
void ftnscopy();
int ftnsleng();
void ftnssubs();
void ftnstop();
int ftnunlock();
int ftnwef();
int ftnwrit();
char* ftnxcons();
void vmscls();
unsigned char* vmsdel();
void vmsglob();
char* vmsload();
int vmsopn();
int vmsrdb();
int vmsrdc();
int vmsrdd();
int vmsrdf();
int vmsrdi();
int vmsrdl();
int vmsrds();
int vmsrdt();
int vmsrdu();
void vmssave();
unsigned char* vmsuse();
void vmsvect();
int vmswrb();
int vmswrc();
int vmswrd();
int vmswrf();
int vmswri();
int vmswrl();
int vmswrs();
int vmswrt();
int vmswru();
#endif
#ifdef PRIMELIB
int add1_();
void atch__();
void attdev();
int bnsrch();
void break_();
SHORT clos_a();
void cnam__();
int cnva_a();
int cnvb_a();
void comi__();
void como__();
int csub_a();
double date_a();
int delet_();
char* fill_a();
int find_();
int fsub_a();
char* gchr_a();
void gpath_();
int jstr_a();
void kx_cre();
void kx_rfc();
int lock_();
SHORT lt();
int lsub_a();
int lstr_a();
int next_();
char* mchr_a();
int mstr_a();
int msub_a();
int nlen_a();
void ntfym_();
void prwf__();
int queryc();
int quick();
void rdtk__();
SHORT rt();
LONG rt4();
int rstr_a();
void sgdr__();
void sleep_();
void srch__();
int sstr_a();
void subsrt();
void timdat();
void tnoua();
void tnou();
void tonl();
void tooct();
int tree_a();
int trnc_a();
void tsrc__();
void t1in();
int updat_();
#endif
#ifdef HONEYLIB
int appendI();
int appendN();
void atos();
void breakkey();
void bye();
void clerr();
char* clk();
char* dat();
char* gclk();
int gcos();
int getpar();
void getstr();
int hdeledI();
int hdeledN();
int hdeleI();
int hdeleN();
int hgetI();
int hgetN();
int hisamI();
int hisamN();
int hisrtI();
int hisrtN();
int hnextI();
int hnextN();
int hpushI();
int hpushN();
int hsetI();
int hsetN();
int hlwpI();
int hlwpN();
int hnwpI();
int hnwpN();
int hnwplI();
int hnwplN();
int hreplI();
int hreplN();
int hzapbI();
int hzapbN();
int ical();
int idat();
int ierror();
int ifbrk();
int intstr();
int lenc();
int lock();
int mdat();
char* padl();
char* padr();
void sfclos();
char* String();
void trim();
char* triml();
char* trimr();
void trimup();
int unlock();
char* unof();
char* upc();
#endif
#ifdef UNIFTLIB
int bits();
void Lbits();
int fld();
void Lfld();
int trmlen();
char* upperc();
#endif
