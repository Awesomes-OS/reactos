#include <stdint.h>
#include <stdio.h>
#include <float.h>
#define __USE_ISOC9X 1
#define __USE_ISOC99 1
#include <math.h>

#include "windef.h"
#include "winbase.h"

int CDECL __signbit(double x)
{
	union {
		double d;
		uint64_t i;
	} y = { x };
	return y.i>>63;
}

int CDECL __signbitf(float x)
{
	union {
		float f;
		uint32_t i;
	} y = { x };
	return y.i>>31;
}

#if (LDBL_MANT_DIG == 64 || LDBL_MANT_DIG == 113) && LDBL_MAX_EXP == 16384
int CDECL __signbitl(long double x)
{
	union ldshape u = {x};
	return u.i.se >> 15;
}
#elif LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
int CDECL __signbitl(long double x)
{
	return __signbit(x);
}
#else
#error Unsupported architecture
#endif

int CDECL _fdsign(float x)
{
    return __signbitf(x) ? 0x8000 : 0;
}

int CDECL _ldsign(long double x)
{
    return __signbitl(x) ? 0x8000 : 0;
}

int CDECL _dsign(double x)
{
    return __signbit(x) ? 0x8000 : 0;
}

short CDECL _dclass(double x)
{
    switch (_fpclass(x)) {
    case _FPCLASS_QNAN:
    case _FPCLASS_SNAN: /* Never on Windows */
        return FP_NAN;
    case _FPCLASS_NINF:
    case _FPCLASS_PINF:
        return FP_INFINITE;
    case _FPCLASS_ND:
    case _FPCLASS_PD:
        return FP_SUBNORMAL;
    case _FPCLASS_NZ:
    case _FPCLASS_PZ:
        return FP_ZERO;
    case _FPCLASS_NN:
    case _FPCLASS_PN:
    default:
        return FP_NORMAL;
    }
}

short CDECL _fdclass(float x)
{
    return _dclass(x);
}

short CDECL _ldclass(long double x)
{
    return _dclass(x);
}

short CDECL _dtest(double *x)
{
    return _dclass(*x);
}

short CDECL _fdtest(float *x)
{
    return _dclass(*x);
}

short CDECL _ldtest(long double *x)
{
    return _dclass(*x);
}
