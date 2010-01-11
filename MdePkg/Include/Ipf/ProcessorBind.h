/** @file
  Processor or Compiler specific defines and types for Intel Itanium(TM) processors.

  Copyright (c) 2006 - 2010, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PROCESSOR_BIND_H__
#define __PROCESSOR_BIND_H__


///
/// Define the processor type so other code can make processor-based choices
///
#define MDE_CPU_IPF


//
// Make sure we are using the correct packing rules per EFI specification
//
#pragma pack()


#if defined(__INTEL_COMPILER)
//
// Disable ICC's remark #869: "Parameter" was never referenced warning.
// This is legal ANSI C code so we disable the remark that is turned on with -Wall
//
#pragma warning ( disable : 869 )

//
// Disable ICC's remark #1418: external function definition with no prior declaration.
// This is legal ANSI C code so we disable the remark that is turned on with /W4
//
#pragma warning ( disable : 1418 )

//
// Disable ICC's remark #1419: external declaration in primary source file
// This is legal ANSI C code so we disable the remark that is turned on with /W4
//
#pragma warning ( disable : 1419 )

//
// Disable ICC's remark #593: "Variable" was set but never used.
// This is legal ANSI C code so we disable the remark that is turned on with /W4
//
#pragma warning ( disable : 593 )

#endif


#if defined(_MSC_EXTENSIONS)
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
// Disable warning on conversion from function pointer to a data pointer
//
#pragma warning ( disable : 4054 )

//
// ASSERT(FALSE) or while (TRUE) are legal constructes so supress this warning
//
#pragma warning ( disable : 4127 )

//
// Can not cast a function pointer to a data pointer. We need to do this on
// Itanium processors to get access to the PLABEL.
//
#pragma warning ( disable : 4514 )

//
// This warning is caused by functions defined but not used. For precompiled header only.
//
#pragma warning ( disable : 4505 )

//
// This warning is caused by empty (after preprocessing) source file. For precompiled header only.
//
#pragma warning ( disable : 4206 )

#endif


#if !defined (__STDC_VERSION__) || (__STDC_VERSION__) < 199901L
  //
  // No ANSI C 2000 stdint.h integer width declarations, so define equivalents
  //

  #if defined(_MSC_EXTENSIONS)
    //
    // use Microsoft C compiler dependent integer width types
    //

    ///
    /// 8-byte unsigned value
    ///
    typedef unsigned __int64    UINT64;
    ///
    /// 8-byte signed value
    ///
    typedef __int64             INT64;
    ///
    /// 4-byte unsigned value
    ///
    typedef unsigned __int32    UINT32;
    ///
    /// 4-byte signed value
    ///
    typedef __int32             INT32;
    ///
    /// 2-byte unsigned value
    ///
    typedef unsigned short      UINT16;
    ///
    /// 2-byte Character.  Unless otherwise specified all strings are stored in the
    /// UTF-16 encoding format as defined by Unicode 2.1 and ISO/IEC 10646 standards.
    ///
    typedef unsigned short      CHAR16;
    ///
    /// 2-byte signed value
    ///
    typedef short               INT16;
    ///
    /// Logical Boolean.  1-byte value containing 0 for FALSE or a 1 for TRUE.  Other
    /// values are undefined.
    ///
    typedef unsigned char       BOOLEAN;
    ///
    /// 1-byte unsigned value
    ///
    typedef unsigned char       UINT8;
    ///
    /// 1-byte Character
    ///
    typedef char                CHAR8;
    ///
    /// 1-byte signed value
    ///
    typedef char                INT8;
  #else
    #if defined(_EFI_P64)
      //
      // P64 - pointers being 64-bit and longs and ints are 32-bits.
      //

      ///
      /// 8-byte unsigned value
      ///
      typedef unsigned long long  UINT64;
      ///
      /// 8-byte signed value
      ///
      typedef long long           INT64;
      ///
      /// 4-byte unsigned value
      ///
      typedef unsigned int        UINT32;
      ///
      /// 4-byte signed value
      ///
      typedef int                 INT32;
      ///
      /// 2-byte Character.  Unless otherwise specified all strings are stored in the
      /// UTF-16 encoding format as defined by Unicode 2.1 and ISO/IEC 10646 standards.
      ///
      typedef unsigned short      CHAR16;
      ///
      /// 2-byte unsigned value
      ///
      typedef unsigned short      UINT16;
      ///
      /// 2-byte signed value
      ///
      typedef short               INT16;
      ///
      /// Logical Boolean.  1-byte value containing 0 for FALSE or a 1 for TRUE.  Other
      /// values are undefined.
      ///
      typedef unsigned char       BOOLEAN;
      ///
      /// 1-byte unsigned value
      ///
      typedef unsigned char       UINT8;
      ///
      /// 1-byte Character
      ///
      typedef char                CHAR8;
      ///
      /// 1-byte signed value
      ///
      typedef char                INT8;
    #else
      //
      // Assume LP64 - longs and pointers are 64-bit. Ints are 32-bit.
      //

      ///
      /// 8-byte unsigned value
      ///
      typedef unsigned long   UINT64;
      ///
      /// 8-byte signed value
      ///
      typedef long            INT64;
      ///
      /// 4-byte unsigned value
      ///
      typedef unsigned int    UINT32;
      ///
      /// 4-byte signed value
      ///
      typedef int             INT32;
      ///
      /// 2-byte unsigned value
      ///
      typedef unsigned short  UINT16;
      ///
      /// 2-byte Character.  Unless otherwise specified all strings are stored in the
      /// UTF-16 encoding format as defined by Unicode 2.1 and ISO/IEC 10646 standards.
      ///
      typedef unsigned short  CHAR16;
      ///
      /// 2-byte signed value
      ///
      typedef short           INT16;
      ///
      /// Logical Boolean.  1-byte value containing 0 for FALSE or a 1 for TRUE.  Other
      /// values are undefined.
      ///
      typedef unsigned char   BOOLEAN;
      ///
      /// 1-byte unsigned value
      ///
      typedef unsigned char   UINT8;
      ///
      /// 1-byte Character
      ///
      typedef char            CHAR8;
      ///
      /// 1-byte signed value
      ///
      typedef char            INT8;
    #endif
  #endif
#else
  //
  // Use ANSI C 2000 stdint.h integer width declarations
  //
  #include <stdint.h>

  ///
  /// Logical Boolean.  1-byte value containing 0 for FALSE or a 1 for TRUE.  Other
  /// values are undefined.
  ///
  typedef uint8_t   BOOLEAN;
  ///
  /// 1-byte signed value
  ///
  typedef int8_t    INT8;
  ///
  /// 1-byte unsigned value
  ///
  typedef uint8_t   UINT8;
  ///
  /// 2-byte signed value
  ///
  typedef int16_t   INT16;
  ///
  /// 2-byte unsigned value
  ///
  typedef uint16_t  UINT16;
  ///
  /// 4-byte signed value
  ///
  typedef int32_t   INT32;
  ///
  /// 4-byte unsigned value
  ///
  typedef uint32_t  UINT32;
  ///
  /// 8-byte signed value
  ///
  typedef int64_t   INT64;
  ///
  /// 8-byte unsigned value
  ///
  typedef uint64_t  UINT64;
  ///
  /// 1-byte Character
  ///
  typedef char      CHAR8;
  ///
  /// 2-byte Character.  Unless otherwise specified all strings are stored in the
  /// UTF-16 encoding format as defined by Unicode 2.1 and ISO/IEC 10646 standards.
  ///
  typedef uint16_t  CHAR16;

#endif

///
/// Unsigned value of native width.  (4 bytes on supported 32-bit processor instructions,
/// 8 bytes on supported 64-bit processor instructions)
///
typedef UINT64  UINTN;
///
/// Signed value of native width.  (4 bytes on supported 32-bit processor instructions,
/// 8 bytes on supported 64-bit processor instructions)
///
typedef INT64   INTN;


//
// Processor specific defines
//

///
/// A value of native width with the highest bit set.
///
#define MAX_BIT     0x8000000000000000ULL
///
/// A value of native width with the two highest bits set.
///
#define MAX_2_BITS  0xC000000000000000ULL

///
/// Maximum legal Itanium-based address
///
#define MAX_ADDRESS   0xFFFFFFFFFFFFFFFFULL

///
/// Per the Itanium Software Conventions and Runtime Architecture Guide,
/// section 3.3.4, IPF stack must always be 16-byte aligned.
///
#define CPU_STACK_ALIGNMENT   16

//
// Modifier to ensure that all protocol member functions and EFI intrinsics
// use the correct C calling convention. All protocol member functions and
// EFI intrinsics are required to modify their member functions with EFIAPI.
//
#ifdef EFIAPI
  ///
  /// If EFIAPI is already defined, then we use that definition.
  ///
#elif defined(_MSC_EXTENSIONS)
  ///
  /// Microsoft* compiler specific method for EFIAPI calling convension
  /// 
  #define EFIAPI __cdecl
#else
  #define EFIAPI
#endif

///
/// For GNU assembly code, .global or .globl can declare global symbols.
/// Define this macro to unify the usage.
///
#define ASM_GLOBAL .globl

///
/// A pointer to a function in IPF points to a plabel.
///
typedef struct {
  UINT64  EntryPoint;
  UINT64  GP;
} EFI_PLABEL;

///
/// PAL Call return structure.
///
typedef struct {
  UINT64                    Status;
  UINT64                    r9;
  UINT64                    r10;
  UINT64                    r11;
} PAL_CALL_RETURN;

/**
  Return the pointer to the first instruction of a function given a function pointer.
  For Itanium processors, all function calls are made through a PLABEL, so a pointer to a function 
  is actually a pointer to a PLABEL.  The pointer to the first instruction of the function 
  is contained within the PLABEL.  This macro may be used to retrieve a pointer to the first 
  instruction of a function independent of the CPU architecture being used.  This is very 
  useful when printing function addresses through DEBUG() macros.
  
  @param  FunctionPointer   A pointer to a function.

  @return The pointer to the first instruction of a function given a function pointer.
  
**/
#define FUNCTION_ENTRY_POINT(FunctionPointer) (VOID *)(UINTN)(((EFI_PLABEL *)(FunctionPointer))->EntryPoint)

#endif

