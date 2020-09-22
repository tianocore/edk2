/** @file
  Processor or Compiler specific defines and types for RISC-V.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PROCESSOR_BIND_H__
#define __PROCESSOR_BIND_H__

//
// Define the processor type so other code can make processor based choices
//
#define MDE_CPU_RISCV64

//
// Make sure we are using the correct packing rules per EFI specification
//
#ifndef __GNUC__
#pragma pack()
#endif

//
// Use ANSI C 2000 stdint.h integer width declarations
//
#include <stdint.h>
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

//
// Unsigned value of native width.  (4 bytes on supported 32-bit processor instructions,
// 8 bytes on supported 64-bit processor instructions)
//
typedef UINT64  UINTN;

//
// Signed value of native width.  (4 bytes on supported 32-bit processor instructions,
// 8 bytes on supported 64-bit processor instructions)
//
typedef INT64   INTN;

//
// Processor specific defines
//

//
// A value of native width with the highest bit set.
//
#define MAX_BIT      0x8000000000000000

//
// A value of native width with the two highest bits set.
//
#define MAX_2_BITS   0xC000000000000000

//
// The stack alignment required for RISC-V
//
#define CPU_STACK_ALIGNMENT  16

//
// Modifier to ensure that all protocol member functions and EFI intrinsics
// use the correct C calling convention. All protocol member functions and
// EFI intrinsics are required to modify their member functions with EFIAPI.
//
#define EFIAPI

#if defined(__GNUC__)
  //
  // For GNU assembly code, .global or .globl can declare global symbols.
  // Define this macro to unify the usage.
  //
  #define ASM_GLOBAL .globl
#endif

#endif
