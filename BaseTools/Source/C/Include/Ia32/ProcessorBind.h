/** @file
  Processor or Compiler specific defines and types for x64.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PROCESSOR_BIND_H__
#define __PROCESSOR_BIND_H__

//
// Define the processor type so other code can make processor based choices
//
#define MDE_CPU_IA32

//
// Make sure we are useing the correct packing rules per EFI specification
//
#ifndef __GNUC__
#pragma pack()
#endif

#if _MSC_EXTENSIONS

//
// Disable warning that make it impossible to compile at /W4
// This only works for Microsoft* tools
//

//
// Disabling bitfield type checking warnings.
//
#pragma warning ( disable : 4214 )

//
// Disabling the unreferenced formal parameter warnings.
//
#pragma warning ( disable : 4100 )

//
// Disable slightly different base types warning as CHAR8 * can not be set
// to a constant string.
//
#pragma warning ( disable : 4057 )

//
// ASSERT(FALSE) or while (TRUE) are legal constructs so suppress this warning
//
#pragma warning ( disable : 4127 )


#endif


#if !defined(__GNUC__) && (__STDC_VERSION__ < 199901L)
  //
  // No ANSI C 2000 stdint.h integer width declarations, so define equivalents
  //

  #if _MSC_EXTENSIONS

    //
    // use Microsoft* C compiler dependent integer width types
    //
    typedef unsigned __int64    UINT64;
    typedef __int64             INT64;
    typedef unsigned __int32    UINT32;
    typedef __int32             INT32;
    typedef unsigned short      UINT16;
    typedef unsigned short      CHAR16;
    typedef short               INT16;
    typedef unsigned char       BOOLEAN;
    typedef unsigned char       UINT8;
    typedef char                CHAR8;
    typedef char                INT8;
  #else

    //
    // Assume standard IA-32 alignment.
    // BugBug: Need to check portability of long long
    //
    typedef unsigned long long  UINT64;
    typedef long long           INT64;
    typedef unsigned int        UINT32;
    typedef int                 INT32;
    typedef unsigned short      UINT16;
    typedef unsigned short      CHAR16;
    typedef short               INT16;
    typedef unsigned char       BOOLEAN;
    typedef unsigned char       UINT8;
    typedef char                CHAR8;
    typedef char                INT8;
  #endif

  #define UINT8_MAX 0xff

#else
  //
  // Use ANSI C 2000 stdint.h integer width declarations
  //
  #include "stdint.h"
  typedef uint8_t   BOOLEAN;
  typedef int8_t    INT8;
  typedef uint8_t   UINT8;
  typedef int16_t   INT16;
  typedef uint16_t  UINT16;
  typedef int32_t   INT32;
  typedef uint32_t  UINT32;
  typedef int64_t   INT64;
  typedef uint64_t  UINT64;
  typedef char      CHAR8;
  typedef uint16_t  CHAR16;

#endif

typedef UINT32  UINTN;
typedef INT32   INTN;


//
// Processor specific defines
//
#define MAX_BIT     0x80000000
#define MAX_2_BITS  0xC0000000

//
// Modifier to ensure that all protocol member functions and EFI intrinsics
// use the correct C calling convention. All protocol member functions and
// EFI intrinsics are required to modify their member functions with EFIAPI.
//
#if _MSC_EXTENSIONS
  //
  // Microsoft* compiler requires _EFIAPI usage, __cdecl is Microsoft* specific C.
  //
  #define EFIAPI __cdecl
#endif

#if __GNUC__
  #define EFIAPI __attribute__((cdecl))
#endif

//
// The Microsoft* C compiler can removed references to unreferenced data items
//  if the /OPT:REF linker option is used. We defined a macro as this is a
//  a non standard extension
//
#if _MSC_EXTENSIONS
  #define GLOBAL_REMOVE_IF_UNREFERENCED __declspec(selectany)
#else
  #define GLOBAL_REMOVE_IF_UNREFERENCED
#endif

#endif
