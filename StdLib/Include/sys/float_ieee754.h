/** @file
    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 * Copyright (c) 1992, 1993
 *  The Regents of the University of California.  All rights reserved.
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
 *  @(#)float.h 8.1 (Berkeley) 6/10/93
    NetBSD: float_ieee754.h,v 1.8 2005/12/11 12:25:20 christos Exp

 * NOTICE: This is not a standalone file.  To use it, #include it in
 * your port's float.h header.
**/

#ifndef _SYS_FLOAT_IEEE754_H_
#define _SYS_FLOAT_IEEE754_H_

#include <sys/EfiCdefs.h>
#include <sys/featuretest.h>

#ifndef FLT_ROUNDS
__BEGIN_DECLS
extern int __flt_rounds(void);
__END_DECLS
#define FLT_ROUNDS  __flt_rounds()
#endif

#ifndef FLT_EVAL_METHOD
#if __GNUC_PREREQ__(3, 3)
#define FLT_EVAL_METHOD __FLT_EVAL_METHOD__
#endif /* GCC >= 3.3 */
#endif /* defined(FLT_EVAL_METHOD) */

#define FLT_RADIX 2   /* b */

#define FLT_MANT_DIG  24    /* p */
#define FLT_EPSILON 1.19209290E-7F  /* b**(1-p) */
#define FLT_DIG   6   /* floor((p-1)*log10(b))+(b == 10) */
#define FLT_MIN_EXP (-125)    /* emin */
#define FLT_MIN   1.17549435E-38F /* b**(emin-1) */
#define FLT_MIN_10_EXP  (-37)   /* ceil(log10(b**(emin-1))) */
#define FLT_MAX_EXP 128   /* emax */
//#define FLT_MAX   3.40282347e+38F /* (1-b**(-p))*b**emax */
#define FLT_MAX   3.402823e+38F /* (1-b**(-p))*b**emax */
#define FLT_MAX_10_EXP  38    /* floor(log10((1-b**(-p))*b**emax)) */

#define DBL_MANT_DIG  53
#define DBL_EPSILON 2.2204460492503131E-16
#define DBL_DIG   15
#define DBL_MIN_EXP (-1021)
#define DBL_MIN   2.2250738585072014E-308
#define DBL_MIN_10_EXP  (-307)
#define DBL_MAX_EXP 1024
#define DBL_MAX   1.7976931348623157E+308
#define DBL_MAX_10_EXP  308

/*
 * If no extended-precision type is defined by the machine-dependent
 * header including this, default to `long double' being double-precision.
 */
#ifndef LDBL_MANT_DIG
  #define LDBL_MANT_DIG     DBL_MANT_DIG
  #define LDBL_EPSILON      DBL_EPSILON
  #define LDBL_DIG          DBL_DIG
  #define LDBL_MIN_EXP      DBL_MIN_EXP
  #define LDBL_MIN          DBL_MIN
  #define LDBL_MIN_10_EXP   DBL_MIN_10_EXP
  #define LDBL_MAX_EXP      DBL_MAX_EXP
  #define LDBL_MAX          DBL_MAX
  #define LDBL_MAX_10_EXP   DBL_MAX_10_EXP

  #define DECIMAL_DIG       17    /* ceil((1+p*log10(b))-(b==10) */
#endif /* LDBL_MANT_DIG */

#endif  /* _SYS_FLOAT_IEEE754_H_ */
