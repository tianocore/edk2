/*  $NetBSD: fenv.h,v 1.2 2014/01/29 00:22:09 matt Exp $    */
/** @file
*
*  Copyright (c) 2013 - 2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/
 /*
 * Based on ieeefp.h written by J.T. Conklin, Apr 28, 1995
 * Public domain.
 */

#ifndef _ARM_FENV_H_
#define _ARM_FENV_H_

#ifdef __ARM_PCS_AAPCS64
/* AArch64 split FPSCR into two registers FPCR and FPSR */
typedef struct {
    unsigned int __fpcr;
    unsigned int __fpsr;
} fenv_t;
#else
typedef int fenv_t;     /* FPSCR */
#endif
typedef int fexcept_t;

#define FE_INVALID      0x01    /* invalid operation exception */
#define FE_DIVBYZERO    0x02    /* divide-by-zero exception */
#define FE_OVERFLOW     0x04    /* overflow exception */
#define FE_UNDERFLOW    0x08    /* underflow exception */
#define FE_INEXACT      0x10    /* imprecise (loss of precision; "inexact") */

#define FE_ALL_EXCEPT   0x1f

#define FE_TONEAREST    0   /* round to nearest representable number */
#define FE_UPWARD       1   /* round toward positive infinity */
#define FE_DOWNWARD     2   /* round toward negative infinity */
#define FE_TOWARDZERO   3   /* round to zero (truncate) */

__BEGIN_DECLS

/* Default floating-point environment */
extern const fenv_t   __fe_dfl_env;
#define FE_DFL_ENV    (&__fe_dfl_env)

__END_DECLS

#endif /* _ARM_FENV_H_ */
