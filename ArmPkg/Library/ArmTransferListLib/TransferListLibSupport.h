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
#define NULL      ((void *)0)
#endif

//
// Definitions for global constants used by Transfer List library routines
//
#define INT_MAX     0x7FFFFFFF           /* Maximum (signed) int value */
#define INT32_MAX   0x7FFFFFFF           /* Maximum (signed) int32 value */
#define UINT32_MAX  0xFFFFFFFF           /* Maximum unsigned int32 value */

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



//
// Macros that directly map functions to BaseLib, BaseMemoryLib functions
//
#define assert(expression)
#define memcpy(dest, source, count)   CopyMem(dest, source, (UINTN)(count))
#define memset(dest, ch, count)       SetMem(dest, (UINTN)(count), (UINT8)(ch))
#define memcmp(buf1, buf2, count)     (int)(CompareMem(buf1, buf2, (UINTN)(count)))
#define memmove(dest, source, count)  CopyMem(dest, source, (UINTN)(count))

#endif /* TRANSFER_LIST_LIB_SUPPORT_H_ */
