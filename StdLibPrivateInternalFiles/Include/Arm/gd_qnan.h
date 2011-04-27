/* $NetBSD: gd_qnan.h,v 1.1 2006/01/25 15:33:28 kleink Exp $ */

#include <machine/endian.h>

#define f_QNAN 0x7fc00000
#if BYTE_ORDER == BIG_ENDIAN
#define d_QNAN0 0x7ff80000
#define d_QNAN1 0x0
#else
#define d_QNAN0 0x0
#define d_QNAN1 0x7ff80000
#endif
