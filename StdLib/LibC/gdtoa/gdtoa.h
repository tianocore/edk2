/** @file

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

The author of this software is David M. Gay.

Copyright (C) 1998 by Lucent Technologies
All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the name of Lucent or any of its entities
not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

LUCENT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL LUCENT OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.

  $NetBSD: gdtoa.h,v 1.6.4.1.4.1 2008/04/08 21:10:55 jdc Exp

****************************************************************/

/* Please send bug reports to David M. Gay (dmg at acm dot org,
 * with " at " changed at "@" and " dot " changed to ".").  */

#ifndef GDTOA_H_INCLUDED
#define GDTOA_H_INCLUDED
#include  <LibConfig.h>

#include "arith.h"

#ifndef Long
#define Long int32_t
#endif
#ifndef ULong
#define ULong uint32_t
#endif
#ifndef UShort
#define UShort uint16_t
#endif

#ifndef ANSI
#define ANSI(x) x
#define Void void
#endif /* ANSI */

#ifndef CONST
#define CONST const
#endif /* CONST */

enum {  /* return values from strtodg */
  STRTOG_Zero = 0,
  STRTOG_Normal = 1,
  STRTOG_Denormal = 2,
  STRTOG_Infinite = 3,
  STRTOG_NaN  = 4,
  STRTOG_NaNbits  = 5,
  STRTOG_NoNumber = 6,
  STRTOG_Retmask  = 7,

  /* The following may be or-ed into one of the above values. */

  STRTOG_Neg  = 0x08,
  STRTOG_Inexlo = 0x10,
  STRTOG_Inexhi = 0x20,
  STRTOG_Inexact  = 0x30,
  STRTOG_Underflow= 0x40,
  STRTOG_Overflow = 0x80,
  STRTOG_NoMemory = 0x100
};

 typedef struct
FPI {
  int nbits;
  int emin;
  int emax;
  int rounding;
  int sudden_underflow;
} FPI;

enum {  /* FPI.rounding values: same as FLT_ROUNDS */
  FPI_Round_zero = 0,
  FPI_Round_near = 1,
  FPI_Round_up = 2,
  FPI_Round_down = 3
};

#ifdef __cplusplus
extern "C" {
#endif

#define dtoa    __dtoa
#define gdtoa   __gdtoa
#define ldtoa   __ldtoa
#define hldtoa    __hldtoa
#define hdtoa   __hdtoa
#define freedtoa  __freedtoa
#define strtodg   __strtodg_D2A
#define strtopQ   __strtopQ_D2A
#define strtopx   __strtopx_D2A
#define strtopxL  __strtopxL_D2A
#define strtord   __strtord_D2A

extern char* dtoa  ANSI((double d, int mode, int ndigits, int *decpt,
      int *sign, char **rve));
extern char* hdtoa ANSI((double d, const char *xdigs, int ndigits, int *decpt,
      int *sign, char **rve));
extern char* ldtoa ANSI((long double *ld, int mode, int ndigits, int *decpt,
      int *sign, char **rve));
extern char* hldtoa ANSI((long double e, const char *xdigs, int ndigits,
      int *decpt, int *sign, char **rve));

extern char* gdtoa ANSI((FPI *fpi, int be, ULong *bits, int *kindp,
      int mode, int ndigits, int *decpt, char **rve));
extern void freedtoa ANSI((char*));
extern float  strtof ANSI((CONST char *, char **));
extern double strtod ANSI((CONST char *, char **));
extern int strtodg ANSI((CONST char*, char**, CONST FPI*, Long*, ULong*));

extern char*  g_ddfmt  ANSI((char*, double*, int, unsigned));
extern char*  g_dfmt   ANSI((char*, double*, int, unsigned));
extern char*  g_ffmt   ANSI((char*, float*,  int, unsigned));
extern char*  g_Qfmt   ANSI((char*, void*,   int, unsigned));
extern char*  g_xfmt   ANSI((char*, void*,   int, unsigned));
extern char*  g_xLfmt  ANSI((char*, void*,   int, unsigned));

extern int  strtoId  ANSI((CONST char*, char**, double*, double*));
extern int  strtoIdd ANSI((CONST char*, char**, double*, double*));
extern int  strtoIf  ANSI((CONST char*, char**, float*, float*));
extern int  strtoIQ  ANSI((CONST char*, char**, void*, void*));
extern int  strtoIx  ANSI((CONST char*, char**, void*, void*));
extern int  strtoIxL ANSI((CONST char*, char**, void*, void*));
extern int  strtord  ANSI((CONST char*, char**, int, double*));
extern int  strtordd ANSI((CONST char*, char**, int, double*));
extern int  strtorf  ANSI((CONST char*, char**, int, float*));
extern int  strtorQ  ANSI((CONST char*, char**, int, void*));
extern int  strtorx  ANSI((CONST char*, char**, int, void*));
extern int  strtorxL ANSI((CONST char*, char**, int, void*));

extern int  strtodI  ANSI((CONST char*, char**, double*));
extern int  strtopd  ANSI((CONST char*, char**, double*));
extern int  strtopdd ANSI((CONST char*, char**, double*));
extern int  strtopf  ANSI((CONST char*, char**, float*));
extern int  strtopQ  ANSI((CONST char*, char**, void*));
extern int  strtopx  ANSI((CONST char*, char**, void*));
extern int  strtopxL ANSI((CONST char*, char**, void*));

#ifdef __cplusplus
}
#endif
#endif /* GDTOA_H_INCLUDED */
