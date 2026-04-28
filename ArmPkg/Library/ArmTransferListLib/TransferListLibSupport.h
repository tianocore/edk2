/** @file
  Root include file of C runtime library to support building the third-party
  Transfer List library.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TRANSFER_LIST_LIB_SUPPORT_H_
#define TRANSFER_LIST_LIB_SUPPORT_H_

#include <Base.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

//
// Standard C integer types mapping to UEFI types
//
typedef UINT8   uint8_t;
typedef UINT16  uint16_t;
typedef INT32   int32_t;
typedef UINT32  uint32_t;
typedef UINT64  uint64_t;
typedef UINTN   uintptr_t;
typedef UINTN   size_t;
typedef INTN    ptrdiff_t;
typedef INT64   intmax_t;
typedef UINT64  uintmax_t;

//
// Boolean type support
//
#if defined __STDC_VERSION__ && __STDC_VERSION__ > 201710L
/* bool, true and false are keywords.  */
#else
typedef BOOLEAN bool;
#define true   (1 == 1)
#define false  (1 == 0)
#endif

//
// Standard definitions
//
#ifndef NULL
#define NULL  ((void *)0)
#endif

//
// Definitions for global constants used by Transfer List library routines
//
#define INT_MAX     0x7FFFFFFF           /* Maximum (signed) int value */
#define INT32_MAX   0x7FFFFFFF           /* Maximum (signed) int32 value */
#define UINT32_MAX  0xFFFFFFFF           /* Maximum unsigned int32 value */

//
// Printf format specifiers from inttypes.h
//
#define PRIx32  "x"
#define PRIX32  "X"
#define PRId32  "d"
#define PRIu32  "u"
#define PRIx64  "llx"
#define PRIX64  "llX"
#define PRId64  "lld"
#define PRIu64  "llu"

//
// Function prototypes of Transfer List Library routines
//
void *
memset     (
  void *,
  int,
  size_t
  );

int
memcmp      (
  const void *,
  const void *,
  size_t
  );

void *
memcpy (
  void        *dest,
  const void  *source,
  size_t      count
  );

void *
memmove (
  void        *dest,
  const void  *source,
  size_t      count
  );

int
printf (
  const char  *format,
  ...
  );

int
vprintf (
  const char  *format,
  VA_LIST     args
  );

size_t
strlen (
  const char  *str
  );

char *
strcpy (
  char        *dest,
  const char  *src
  );

int
strcmp (
  const char  *str1,
  const char  *str2
  );

//
// Variable argument support - map to UEFI VA_LIST
//
typedef VA_LIST va_list;
#define va_start(ap, last)  VA_START(ap, last)
#define va_end(ap)          VA_END(ap)
#define va_arg(ap, type)    VA_ARG(ap, type)

//
// Macros that directly map functions to BaseLib, BaseMemoryLib functions
//
#define assert(expression)
#define memcpy(dest, source, count)   CopyMem(dest, source, (UINTN)(count))
#define memset(dest, ch, count)       SetMem(dest, (UINTN)(count), (UINT8)(ch))
#define memcmp(buf1, buf2, count)     (int)(CompareMem(buf1, buf2, (UINTN)(count)))
#define memmove(dest, source, count)  CopyMem(dest, source, (UINTN)(count))
#define printf(...)                   DebugPrint(DEBUG_INFO, __VA_ARGS__)
#define vprintf(format, args)         DebugVPrint(DEBUG_INFO, format, args)
#define strlen(str)                   (size_t)(AsciiStrLen(str))
#define strcpy(dest, src)             AsciiStrCpyS(dest, AsciiStrSize(src), src)
#define strcmp(str1, str2)            (int)(AsciiStrCmp(str1, str2))

#endif /* TRANSFER_LIST_LIB_SUPPORT_H_ */
