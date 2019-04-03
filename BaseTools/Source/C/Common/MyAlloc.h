/** @file
Header file for memory allocation tracking functions.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MYALLOC_H_
#define _MYALLOC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Common/BaseTypes.h>

//
// Default operation is to use the memory allocation tracking functions.
// To over-ride add "#define USE_MYALLOC 0" to your program header and/or
// source files as needed.  Or, just do not include this header file in
// your project.
//
#ifndef USE_MYALLOC
#define USE_MYALLOC 1
#endif

#if USE_MYALLOC
//
// Replace C library allocation routines with MyAlloc routines.
//
#define malloc(size)        MyAlloc ((size), __FILE__, __LINE__)
#define calloc(count, size) MyAlloc ((count) * (size), __FILE__, __LINE__)
#define realloc(ptr, size)  MyRealloc ((ptr), (size), __FILE__, __LINE__)
#define free(ptr)           MyFree ((ptr), __FILE__, __LINE__)
#define alloc_check(final)  MyCheck ((final), __FILE__, __LINE__)

//
// Structure for checking/tracking memory allocations.
//
typedef struct MyAllocStruct {
  UINTN                 Cksum;
  struct MyAllocStruct  *Next;
  UINTN                 Line;
  UINTN                 Size;
  UINT8                 *File;
  UINT8                 *Buffer;
} MY_ALLOC_STRUCT;
//
// Cksum := (UINTN)This + (UINTN)Next + Line + Size + (UINTN)File +
//          (UINTN)Buffer;
//
// Next := Pointer to next allocation structure in the list.
//
// Line := __LINE__
//
// Size := Size of allocation request.
//
// File := Pointer to __FILE__ string stored immediately following
//         MY_ALLOC_STRUCT in memory.
//
// Buffer := Pointer to UINT32 aligned storage immediately following
//           the NULL terminated __FILE__ string.  This is UINT32
//           aligned because the underflow signature is 32-bits and
//           this will place the first caller address on a 64-bit
//           boundary.
//
//
// Signatures used to check for buffer overflow/underflow conditions.
//
#define MYALLOC_HEAD_MAGIK  0xBADFACED
#define MYALLOC_TAIL_MAGIK  0xDEADBEEF

VOID
MyCheck (
  BOOLEAN      Final,
  UINT8        File[],
  UINTN        Line
  )
;
//
// *++
// Description:
//
//  Check for corruptions in the allocated memory chain.  If a corruption
//  is detection program operation stops w/ an exit(1) call.
//
// Parameters:
//
//  Final := When FALSE, MyCheck() returns if the allocated memory chain
//           has not been corrupted.  When TRUE, MyCheck() returns if there
//           are no un-freed allocations.  If there are un-freed allocations,
//           they are displayed and exit(1) is called.
//
//
//  File := Set to __FILE__ by macro expansion.
//
//  Line := Set to __LINE__ by macro expansion.
//
// Returns:
//
//  n/a
//
// --*/
//
VOID  *
MyAlloc (
  UINTN      Size,
  UINT8      File[],
  UINTN      Line
  )
;
//
// *++
// Description:
//
//  Allocate a new link in the allocation chain along with enough storage
//  for the File[] string, requested Size and alignment overhead.  If
//  memory cannot be allocated or the allocation chain has been corrupted,
//  exit(1) will be called.
//
// Parameters:
//
//  Size := Number of bytes (UINT8) requested by the called.
//          Size cannot be zero.
//
//  File := Set to __FILE__ by macro expansion.
//
//  Line := Set to __LINE__ by macro expansion.
//
// Returns:
//
//  Pointer to the caller's buffer.
//
// --*/
//
VOID  *
MyRealloc (
  VOID       *Ptr,
  UINTN      Size,
  UINT8      File[],
  UINTN      Line
  )
;
//
// *++
// Description:
//
//  This does a MyAlloc(), memcpy() and MyFree().  There is no optimization
//  for shrinking or expanding buffers.  An invalid parameter will cause
//  MyRealloc() to fail with a call to exit(1).
//
// Parameters:
//
//  Ptr := Pointer to the caller's buffer to be re-allocated.
//         Ptr cannot be NULL.
//
//  Size := Size of new buffer.  Size cannot be zero.
//
//  File := Set to __FILE__ by macro expansion.
//
//  Line := Set to __LINE__ by macro expansion.
//
// Returns:
//
//  Pointer to new caller's buffer.
//
// --*/
//
VOID
MyFree (
  VOID       *Ptr,
  UINT8      File[],
  UINTN      Line
  )
;
//
// *++
// Description:
//
//  Release a previously allocated buffer.  Invalid parameters will cause
//  MyFree() to fail with an exit(1) call.
//
// Parameters:
//
//  Ptr := Pointer to the caller's buffer to be freed.
//         A NULL pointer will be ignored.
//
//  File := Set to __FILE__ by macro expansion.
//
//  Line := Set to __LINE__ by macro expansion.
//
// Returns:
//
//  n/a
//
// --*/
//
#else /* USE_MYALLOC */

//
// Nothing to do when USE_MYALLOC is zero.
//
#define alloc_check(final)

#endif /* USE_MYALLOC */
#endif /* _MYALLOC_H_ */

/* eof - MyAlloc.h */
