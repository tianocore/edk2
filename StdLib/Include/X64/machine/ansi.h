/** @file
    Machine dependent ANSI type definitions.

    Copyright (c) 2010-2012, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available
    under the terms and conditions of the BSD License that accompanies this
    distribution.  The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

    Copyright (c) 1990, 1993
    The Regents of the University of California.  All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
      - Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      - Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      - Neither the name of the University nor the names of its contributors
        may be used to endorse or promote products derived from this software
        without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

    NetBSD: ansi.h,v 1.4 2006/10/04 13:51:59 tnozaki Exp
    ansi.h  8.2 (Berkeley) 1/4/94
**/
#ifndef _ANSI_H_
#define _ANSI_H_

#include  <sys/EfiCdefs.h>

#include  <machine/int_types.h>

/*
 * Types which are fundamental to the implementation and may appear in
 * more than one standard header are defined here.  Standard headers
 * then use:
 *  #ifdef  _BSD_SIZE_T_
 *  typedef _BSD_SIZE_T_ size_t;
 *  #undef  _BSD_SIZE_T_
 *  #endif
 */
#define _BSD_CLOCK_T_     _EFI_CLOCK_T      /* clock() */
#define _BSD_PTRDIFF_T_   _EFI_PTRDIFF_T_   /* ptr1 - ptr2 */
#define _BSD_SIZE_T_      _EFI_SIZE_T_      /* sizeof() */
#define _BSD_SSIZE_T_     INTN              /* byte count or error */
#define _BSD_TIME_T_      _EFI_TIME_T       /* time() */
#define _BSD_VA_LIST_     VA_LIST
#define _BSD_CLOCKID_T_   INT64             /* clockid_t */
#define _BSD_TIMER_T_     INT64             /* timer_t */
#define _BSD_SUSECONDS_T_ INT64             /* suseconds_t */
#define _BSD_USECONDS_T_  UINT64            /* useconds_t */

/*
 * NOTE: rune_t is not covered by ANSI nor other standards, and should not
 * be instantiated outside of lib/libc/locale.  use wchar_t.
 *
 * Runes (wchar_t) is declared to be an ``int'' instead of the more natural
 * ``unsigned long'' or ``long''.  Two things are happening here.  It is not
 * unsigned so that EOF (-1) can be naturally assigned to it and used.  Also,
 * it looks like 10646 will be a 31 bit standard.  This means that if your
 * ints cannot hold 32 bits, you will be in trouble.  The reason an int was
 * chosen over a long is that the is*() and to*() routines take ints (says
 * ANSI C), but they use _RUNE_T_ instead of int.  By changing it here, you
 * lose a bit of ANSI conformance, but your programs will still work.
 *
 * Note that _WCHAR_T_ and _RUNE_T_ must be of the same type.  When wchar_t
 * and rune_t are typedef'd, _WCHAR_T_ will be undef'd, but _RUNE_T remains
 * defined for ctype.h.
 */
#define _BSD_WCHAR_T_     _EFI_WCHAR_T    /* wchar_t */
#define _BSD_WINT_T_      _EFI_WINT_T     /* wint_t */
#define _BSD_RUNE_T_      _EFI_WCHAR_T    /* rune_t */
#define _BSD_WCTRANS_T_   void *          /* wctrans_t */
#define _BSD_WCTYPE_T_    unsigned int    /* wctype_t */

/*
 * mbstate_t is an opaque object to keep conversion state, during multibyte
 * stream conversions.  The content must not be referenced by user programs.
 */
typedef struct {
  UINT32  A;      // Np;
  UINT32  B;      // U;
  UINT32  E;      // L
  UINT8   C[4];   // n[4]
  UINT16  D[2];   // w[2]
} __mbstate_t;
#define _BSD_MBSTATE_T_   __mbstate_t /* mbstate_t */

#endif  /* _ANSI_H_ */
