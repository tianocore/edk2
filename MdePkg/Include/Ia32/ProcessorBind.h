/** @file
  Processor or Compiler specific defines and types for Ia32 architecture.

  Copyright (c) 2006, Intel Corporation                                                         
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
/// Define the processor type so other code can make processor based choices
///
#define MDE_CPU_IA32

//
// Make sure we are useing the correct packing rules per EFI specification
//
#ifndef __GNUC__
#pragma pack()
#endif

#if __INTEL_COMPILER
//
// Disable ICC's remark #593: "LocalVariable" was set but never used
// This is legal ANSI C code so we disable the remark that is turned on with -Wall
//
#pragma warning ( disable : 593 )

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
// ASSERT(FALSE) or while (TRUE) are legal constructes so supress this warning
//
#pragma warning ( disable : 4127 )

//
// This warning is caused by functions defined but not used. For precompiled header only.
//
#pragma warning ( disable : 4505 )

//
// This warning is caused by empty (after preprocessing) souce file. For precompiled header only.
//
#pragma warning ( disable : 4206 )

#endif


#if !defined(__GNUC__) && (__STDC_VERSION__ < 199901L)
  //
  // No ANSI C 2000 stdint.h integer width declarations, so define equivalents
  //
 
  #if _MSC_EXTENSIONS 
    
    //
    // use Microsoft* C complier dependent interger width types 
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
    // Need to check portability of long long
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


///
/// Processor specific defines
///
#define MAX_BIT     0x80000000
#define MAX_2_BITS  0xC0000000

///
/// Maximum legal IA-32 address
///
#define MAX_ADDRESS   0xFFFFFFFF

///
/// The stack alignment required for IA-32
///
#define CPU_STACK_ALIGNMENT   sizeof(UINTN)

//
// Modifier to ensure that all protocol member functions and EFI intrinsics
// use the correct C calling convention. All protocol member functions and
// EFI intrinsics are required to modify thier member functions with EFIAPI.
//
#if _MSC_EXTENSIONS
  ///
  /// Microsoft* compiler requires _EFIAPI useage, __cdecl is Microsoft* specific C.
  /// 
  #define EFIAPI __cdecl  
#else
  #if __GNUC__
    #define EFIAPI __attribute__((cdecl))  
  #endif  
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

//
// For symbol name in GNU assembly code, an extra "_" is necessary
//
#if __GNUC__
  #if defined(linux)
    #define ASM_PFX(name) name
  #else
    #define ASM_PFX(name) _##name
  #endif 
#endif

#define FUNCTION_ENTRY_POINT(p) (p)

#endif

