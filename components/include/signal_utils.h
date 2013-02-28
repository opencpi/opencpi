
#include <math.h>


#define byteLen2Complex(x) ((x)/4)
#define byteLen2Real(x) ((x)/2)
#define Complex2bytes(x) ((x)*4)
#define real2bytes(x) ((x)*2)
#define ComplexLen2Real(x) ((x)/2)

#define min(x,y) (((x)<(y))?(x):(y))
#define mems(t,m) sizeof(((t*)0)->m)

#define QMASK 0xefff
#define Uscale(x)  (double)((double)(x) / pow(2,15))
#define Scale(x)   (int16_t)((double)(x) * pow(2,15))
#define Gain(x) (double)((double)(x) * pow(2,15))

// Qs15 complex absolute value returned as a double
#define scabs( r, i ) sqrt(pow(Uscale(r),2)+pow(Uscale(i),2))

#define PI 3.14159265358979




