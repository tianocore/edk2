/** @file
  Processor or Compiler specific defines and types for AArch64.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Portions copyright (c) 2013, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PROCESSOR_BIND_H__
#define __PROCESSOR_BIND_H__

///
/// Define the processor type so other code can make processor based choices
///
#define MDE_CPU_AARCH64

//
// Make sure we are using the correct packing rules per EFI specification
//
#ifndef __GNUC__
#pragma pack()
#endif

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
  typedef signed char         INT8;
#else
  //
  // Assume standard AARCH64 alignment.
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
  typedef signed char         INT8;

  #define UINT8_MAX 0xff
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
#define MAX_BIT      0x8000000000000000

///
/// A value of native width with the two highest bits set.
///
#define MAX_2_BITS   0xC000000000000000

///
/// The stack alignment required for AARCH64
///
#define CPU_STACK_ALIGNMENT  16

//
// Modifier to ensure that all protocol member functions and EFI intrinsics
// use the correct C calling convention. All protocol member functions and
// EFI intrinsics are required to modify their member functions with EFIAPI.
//
#define EFIAPI

#if defined(__GNUC__)
  ///
  /// For GNU assembly code, .global or .globl can declare global symbols.
  /// Define this macro to unify the usage.
  ///
  #define ASM_GLOBAL .globl

  #if !defined(__APPLE__)
    ///
    /// ARM EABI defines that the linker should not manipulate call relocations
    /// (do bl/blx conversion) unless the target symbol has function type.
    /// CodeSourcery 2010.09 started requiring the .type to function properly
    ///
    #define INTERWORK_FUNC(func__)   .type ASM_PFX(func__), %function

    #define GCC_ASM_EXPORT(func__)  \
             .global  _CONCATENATE (__USER_LABEL_PREFIX__, func__)    ;\
             .type ASM_PFX(func__), %function

    #define GCC_ASM_IMPORT(func__)  \
             .extern  _CONCATENATE (__USER_LABEL_PREFIX__, func__)

  #else
    //
    // .type not supported by Apple Xcode tools
    //
    #define INTERWORK_FUNC(func__)

    #define GCC_ASM_EXPORT(func__)  \
             .globl  _CONCATENATE (__USER_LABEL_PREFIX__, func__)    \

    #define GCC_ASM_IMPORT(name)

  #endif
#endif

/**
  Return the pointer to the first instruction of a function given a function pointer.
  On ARM CPU architectures, these two pointer values are the same,
  so the implementation of this macro is very simple.

  @param  FunctionPointer   A pointer to a function.

  @return The pointer to the first instruction of a function given a function pointer.

**/
#define FUNCTION_ENTRY_POINT(FunctionPointer) (VOID *)(UINTN)(FunctionPointer)

#endif

