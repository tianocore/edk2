/** @file
    Common declarations and definitions for Standard C Library headers.

    This header consolidates definitions and declarations for compiler specific
    features in one place in order to assist in making the remainder of the
    library as compiler independent as possible.

    Certain macro and type definitions are required to be provided by several
    different headers.  In order to avoid having multiple definitions, and the
    attendant risk of having the definitions get out of sync, they are defined in
    this header.

    Note that MdePkg/Include/Base.h is automatically included and will bring
    processor architecture specific definitions along with it.

    Throughout the library, the following macros are used instead of keywords so
    that the library can be easily tuned for different compilers.
    __inline    Defined to the appropriate keyword or not defined.
    __func__    Defined to __FUNC__, __FUNCTION__, or NULL as appropriate.
    __restrict  Defined to nothing for VC++ or to restrict for GCC and C99 compliant compilers.

    This file and its contents are inspired by the <sys/cdefs.h> files in Berkeley
    Unix.  They have been re-implemented to be specific to the EFI environment.

    Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

    Portions Copyright (c) 1991, 1993
    The Regents of the University of California.  All rights reserved.

    Portions of this code are derived from software contributed to Berkeley by
    Berkeley Software Design, Inc.
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
      1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      3. Neither the name of the University nor the names of its contributors
        may be used to endorse or promote products derived from this software
        without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
**/
#ifndef _EFI_CDEFS_H
#define _EFI_CDEFS_H

/*
* Macro to test if we're using a GNU C compiler of a specific vintage
* or later, for e.g. features that appeared in a particular version
* of GNU C.  Usage:
*
*  #if __GNUC_PREREQ__(major, minor)
*  ...cool feature...
*  #else
*  ...delete feature...
*  #endif
*/
#ifdef __GNUC__
#define __GNUC_PREREQ__(x, y)           \
((__GNUC__ == (x) && __GNUC_MINOR__ >= (y)) ||      \
 (__GNUC__ > (x)))

#define DONT_USE_STRONG_WEAK_ALIAS  1

#else
#define __GNUC_PREREQ__(x, y) 0
#endif

#include  <sys/featuretest.h>
//#include <machine/_EfiCdefs.h>
#ifdef __PE32__
#include <sys/_EfiCdefs_PE32.h>
#else
#include <sys/cdefs_aout.h>
#endif

/* NULL is defined by the automatic inclusion of Base.h by the build tools. */

#ifdef __GNUC__
  #define _EFI_SIZE_T_      __SIZE_TYPE__     /* sizeof() */
  #define _EFI_WCHAR_T      __WCHAR_TYPE__
  #define _EFI_WINT_T       __WINT_TYPE__
  //#define _EFI_WINT_MIN     (0)
  //#define _EFI_WINT_MAX     (0xFFFF)
  #define _EFI_PTRDIFF_T_   __PTRDIFF_TYPE__  /* ptr1 - ptr2 --- Must be same size as size_t */

#else
#define _EFI_SIZE_T_      UINTN       /* sizeof() */
#define _EFI_WCHAR_T      UINT16
#define _EFI_WINT_T       INT32
  //#define _EFI_WINT_MIN     (-2147483647)       /* wint_t   */
  //#define _EFI_WINT_MAX     ( 2147483647)       /* wint_t   */
  #define _EFI_PTRDIFF_T_   INTN       /* ptr1 - ptr2 --- Must be same size as size_t */
#endif  /* __GNUC__ */

#define _EFI_CLOCK_T      UINT64
#define _EFI_TIME_T       INT32

#if defined(__cplusplus)
#define __BEGIN_DECLS   extern "C" {
#define __END_DECLS   }
#define __static_cast(x,y)  static_cast<x>(y)
#else
#define __BEGIN_DECLS
#define __END_DECLS
#define __static_cast(x,y)  (x)y
#endif

  /*
  * The __CONCAT macro is used to concatenate parts of symbol names, e.g.
  * with "#define OLD(foo) __CONCAT(old,foo)", OLD(foo) produces oldfoo.
  * The __CONCAT macro is a bit tricky -- make sure you don't put spaces
  * in between its arguments.  __CONCAT can also concatenate double-quoted
  * strings produced by the __STRING macro, but this only works with ANSI C.
  */

#define ___STRING(x)  __STRING(x)
#define ___CONCAT(x,y)  __CONCAT(x,y)
#define __CONCAT(x,y) x ## y
#define __STRING(x) #x

#define __const     CONST
#define __signed    signed
#define __volatile  volatile

#if __STDC__ || defined(__cplusplus)
  #if defined(__cplusplus)
    #define __inline  inline    /* convert to C++ keyword */
  #else
    #if defined(_MSC_VER) || (!defined(__GNUC__) && !defined(__lint__))
      #define __inline      /* delete C99 keyword */
    #endif /* !__GNUC__  && !__lint__ */
  #endif /* !__cplusplus */
#endif  /* !(__STDC__ || __cplusplus) */

/* Used in NetBSD for internal auditing of the source tree. */
#define __aconst

  /*
  * The following macro is used to remove const cast-away warnings
  * from gcc -Wcast-qual; it should be used with caution because it
  * can hide valid errors; in particular most valid uses are in
  * situations where the API requires it, not to cast away string
  * constants. We don't use *intptr_t on purpose here and we are
  * explicit about unsigned long so that we don't have additional
  * dependencies.
  */
#define __UNCONST(a)  ((void *)(a))
//#define __UNCONST(a)  ((void *)(PHYSICAL_ADDRESS)(const void *)(a))

  /*
  * The following macro is used to remove the volatile cast-away warnings
  * from gcc -Wcast-qual; as above it should be used with caution
  * because it can hide valid errors or warnings.  Valid uses include
  * making it possible to pass a volatile pointer to memset().
  * For the same reasons as above, we use unsigned long and not intptr_t.
  */
#define __UNVOLATILE(a) ((void *)(PHYSICAL_ADDRESS)(volatile void *)(a))

  /*
  * GCC2 provides __extension__ to suppress warnings for various GNU C
  * language extensions under "-ansi -pedantic".
  */
#if !__GNUC_PREREQ__(2, 0)
#define __extension__   /* delete __extension__ if non-gcc or gcc1 */
#endif

  /*
  * GCC1 and some versions of GCC2 declare dead (non-returning) and
  * pure (no side effects) functions using "volatile" and "const";
  * unfortunately, these then cause warnings under "-ansi -pedantic".
  * GCC2 uses a new, peculiar __attribute__((attrs)) style.  All of
  * these work for GNU C++ (modulo a slight glitch in the C++ grammar
  * in the distribution version of 2.5.5).
  */
#if !__GNUC_PREREQ__(2, 5)
#define __attribute__(x)  /* delete __attribute__ if non-gcc or gcc1 */
#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
#define __dead    __volatile
#define __pure    __const
#endif
#endif

  /* Delete pseudo-keywords wherever they are not available or needed. */
#ifndef __dead
#define __dead
#define __pure
#endif

#if __GNUC_PREREQ__(2, 7)
#define __unused  __attribute__((__unused__))
#define __noreturn  __attribute__((__noreturn__))
#else
#define __unused  /* delete */
#define __noreturn  /* delete */
#endif

#if __GNUC_PREREQ__(3, 1)
#define __used    __attribute__((__used__))
#else
#define __used    __unused
#endif

#if __GNUC_PREREQ__(2, 7)
#define __packed  __attribute__((__packed__))
#define __aligned(x)  __attribute__((__aligned__(x)))
#define __section(x)  __attribute__((__section__(x)))
#elif defined(__lint__)
#define __packed  /* delete */
#define __aligned(x)  /* delete */
#define __section(x)  /* delete */
#else
#define __packed  error: no __packed for this compiler
#define __aligned(x)  error: no __aligned for this compiler
#define __section(x)  error: no __section for this compiler
#endif

/*
* C99 defines the restrict type qualifier keyword, which was made available
* in GCC 2.92.
*/
#if __STDC_VERSION__ >= 199901L
  #define __restrict  restrict
#else
  #if defined(_MSC_VER) || !__GNUC_PREREQ__(2, 92)
    #define __restrict  /* delete __restrict when not supported */
  #endif
#endif

/*
* C99 defines __func__ predefined identifier, which was made available
* in GCC 2.95.
*/
#if !(__STDC_VERSION__ >= 199901L)
  #if defined(_MSC_VER)
    #define __func__    __FUNCTION__  /* Use the MS-specific predefined macro */
  #elif __GNUC_PREREQ__(2, 6)
    #define __func__  __PRETTY_FUNCTION__
  #elif __GNUC_PREREQ__(2, 4)
    #define __func__  __FUNCTION__
  #else
    #define __func__  ""
  #endif
#endif /* !(__STDC_VERSION__ >= 199901L) */

#define __RENAME(x)

  /*
  * A barrier to stop the optimizer from moving code or assume live
  * register values. This is gcc specific, the version is more or less
  * arbitrary, might work with older compilers.
  */
#if __GNUC_PREREQ__(2, 95)
#define __insn_barrier()  __asm __volatile("":::"memory")
#else
#define __insn_barrier()  /* */
#endif

  /*
  * GNU C version 2.96 adds explicit branch prediction so that
  * the CPU back-end can hint the processor and also so that
  * code blocks can be reordered such that the predicted path
  * sees a more linear flow, thus improving cache behavior, etc.
  *
  * The following two macros provide us with a way to use this
  * compiler feature.  Use __predict_true() if you expect the expression
  * to evaluate to true, and __predict_false() if you expect the
  * expression to evaluate to false.
  *
  * A few notes about usage:
  *
  *  * Generally, __predict_false() error condition checks (unless
  *    you have some _strong_ reason to do otherwise, in which case
  *    document it), and/or __predict_true() `no-error' condition
  *    checks, assuming you want to optimize for the no-error case.
  *
  *  * Other than that, if you don't know the likelihood of a test
  *    succeeding from empirical or other `hard' evidence, don't
  *    make predictions.
  *
  *  * These are meant to be used in places that are run `a lot'.
  *    It is wasteful to make predictions in code that is run
  *    seldomly (e.g. at subsystem initialization time) as the
  *    basic block reordering that this affects can often generate
  *    larger code.
  */
#if __GNUC_PREREQ__(2, 96)
#define __predict_true(exp) __builtin_expect((exp) != 0, 1)
#define __predict_false(exp)  __builtin_expect((exp) != 0, 0)
#else
#define __predict_true(exp) (exp)
#define __predict_false(exp)  (exp)
#endif

/* find least significant bit that is set */
#define __LOWEST_SET_BIT(__mask) ((((__mask) - 1) & (__mask)) ^ (__mask))

#define __SHIFTOUT(__x, __mask) (((__x) & (__mask)) / __LOWEST_SET_BIT(__mask))
#define __SHIFTIN(__x, __mask) ((__x) * __LOWEST_SET_BIT(__mask))
#define __SHIFTOUT_MASK(__mask) __SHIFTOUT((__mask), (__mask))

#if defined(_MSC_VER)           /* Handle Microsoft VC++ compiler specifics. */

    /*  VC++, by default, defines wchar_t as an intrinsic type, equivalent to
    unsigned short.  This conflicts which Standard C Library
    implementations which try to define wchar_t.
    Make sure that this behavior has been turned off by using
    /Zc:wchar_t- on the command line.
    */
  #ifdef _NATIVE_WCHAR_T_DEFINED
  #error You must specify /Zc:wchar_t- to the compiler to turn off intrinsic wchar_t.
  #endif

  /* Get rid of pre-defined macros that are misleading in this environment. */
  #undef  _WIN32
  #undef  _WIN64

  // Keep compiler quiet about casting from smaller to larger types
  #pragma warning ( disable : 4306 )

  #define __STDC__            1
  #define __STDC_VERSION__    199409L
  #define __STDC_HOSTED__     1

#endif  /* defined(_MSC_VER) */
extern int _fltused;    // VC++ requires this if you use floating point.  KEEP for all compilers.

#define _Bool BOOLEAN
#define _DIAGASSERT(e)

// Types used to replace long so that it will have constant length regardless of compiler.
typedef  INT32   LONG32;
typedef UINT32  ULONG32;
typedef  INT64   LONG64;
typedef UINT64  ULONG64;

typedef   INTN   EFI_LONG_T;
typedef  UINTN   EFI_ULONG_T;

/* These types reflect the compiler's size for long */
#if defined(__GNUC__)
  #if __GNUC_PREREQ__(4,4)
    /* GCC 4.4 or later */
    typedef   INTN    LONGN;
    typedef  UINTN    ULONGN;
  #else
    /* minGW gcc variant */
    typedef   INT32   LONGN;
    typedef  UINT32   ULONGN;
  #endif  /* __GNUC_PREREQ__(4,4) */
#else   /* NOT GCC */
  /* Microsoft or Intel compilers */
  typedef   INT32   LONGN;
  typedef  UINT32   ULONGN;
#endif  /* defined(__GNUC__) */

#endif  /* _EFI_CDEFS_H */
