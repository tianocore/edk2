/*      $NetBSD: limits.h,v 1.9 2008/08/29 19:08:29 matt Exp $  */

/*
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      from: @(#)limits.h      7.2 (Berkeley) 6/28/90
 */

#ifndef _ARM32_LIMITS_H_
#define _ARM32_LIMITS_H_

#include <sys/featuretest.h>

#define __CHAR_BIT        8               /* number of bits in a char */
//#define __MB_LEN_MAX      32              /* no multibyte characters */

#define __SCHAR_MIN       (-0x7f-1)       /* max value for a signed char */
#define __SCHAR_MAX       0x7f            /* min value for a signed char */

#define __UCHAR_MAX       0xff            /* max value for an unsigned char */
#define __CHAR_MAX        0xff            /* max value for a char */
#define __CHAR_MIN        0               /* min value for a char */

#define __USHRT_MAX       0xffff          /* max value for an unsigned short */
#define __SHRT_MAX        0x7fff          /* max value for a short */
#define __SHRT_MIN        (-0x7fff-1)     /* min value for a short */

#define __UINT_MAX        0xffffffffU     /* max value for an unsigned int */
#define __INT_MAX         0x7fffffff      /* max value for an int */
#define __INT_MIN         (-0x7fffffff-1) /* min value for an int */

#define __ULONG_MAX       0xffffffffUL    /* max value for an unsigned long */
#define __LONG_MAX        0x7fffffffL     /* max value for a long */
#define __LONG_MIN        (-0x7fffffffL-1)        /* min value for a long */

#if defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE) || \
    defined(_NETBSD_SOURCE)
#define SSIZE_MAX       LONG_MAX        /* max value for a ssize_t */

#if defined(_ISOC99_SOURCE) || (__STDC_VERSION__ - 0) >= 199901L || \
    defined(_NETBSD_SOURCE)
#define __ULLONG_MAX      0xffffffffffffffffULL   /* max unsigned long long */
#define __LLONG_MAX       0x7fffffffffffffffLL    /* max signed long long */
#define __LLONG_MIN       (-0x7fffffffffffffffLL-1) /* min signed long long */
#endif

#if defined(_NETBSD_SOURCE)
#define SIZE_T_MAX      LONG_MAX        /* max value for a size_t */

#define UQUAD_MAX       0xffffffffffffffffLL            /* max unsigned quad */
#define QUAD_MAX        0x7fffffffffffffffLL            /* max signed quad */
#define QUAD_MIN        (-0x7fffffffffffffffLL-1)       /* min signed quad */

#endif /* _NETBSD_SOURCE */
#endif /* _POSIX_C_SOURCE || _XOPEN_SOURCE || _NETBSD_SOURCE */

#if defined(_XOPEN_SOURCE) || defined(_NETBSD_SOURCE)
#define LONG_BIT        32
#define WORD_BIT        32

#define DBL_DIG         15
#define DBL_MAX         1.7976931348623157E+308
#define DBL_MIN         2.2250738585072014E-308

//#define FLT_DIG         6
//#define FLT_MAX         3.40282347E+38F
//#define FLT_MIN         1.17549435E-38F
#endif

#endif  /* _ARM32_LIMITS_H_ */
