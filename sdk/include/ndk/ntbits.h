#pragma once

/*
 * unsigned char _interlockedbittestandset(long volatile *, long)
 * long _InterlockedCompareExchange(long volatile * _Destination, long _Exchange, long _Comparand)
 * unsigned char _interlockedbittestandreset(long volatile *, long)
 */

#include "ntbits-src.h"
#ifndef NTOS_MODE_USER
#include "ntbits-src-kernel.h"
#else
#include "ntbits-src-user.h"
#endif
