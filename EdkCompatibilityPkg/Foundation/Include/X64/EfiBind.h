/*++

Copyright (c) 2005 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiBind.h

Abstract:

  Processor or Compiler specific defines and types for x64.
  We are using the ANSI C 2000 _t type definitions for basic types.
  This it technically a violation of the coding standard, but they
  are used to make EfiTypes.h portable. Code other than EfiTypes.h
  should never use any ANSI C 2000 _t integer types.

--*/

#ifndef _EFI_BIND_H_
#define _EFI_BIND_H_


#define EFI_DRIVER_ENTRY_POINT(InitFunction)                  
#define EFI_APPLICATION_ENTRY_POINT EFI_DRIVER_ENTRY_POINT



//
// Make sure we are useing the correct packing rules per EFI specification
//
#ifndef __GNUC__
#pragma pack()
#endif

#if __INTEL_COMPILER
//
// Disable ICC's warning: trailing comma is nonstandard
//
//#pragma warning ( disable : 271 )

//
// Disable ICC's warning: extra ";" ignored
//
#pragma warning ( disable : 424 )

//
// Disable ICC's warning: : variable "foo" was set but never used
//
#pragma warning ( disable : 593 )

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
// Disable ICC's remark #869: "Parameter" was never referenced warning.
// This is legal ANSI C code so we disable the remark that is turned on with -Wall
//
#pragma warning ( disable : 869 )

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
// Int64ShllMod32 unreferenced inline function
//
#pragma warning ( disable : 4514 )

//
// Unreferenced formal parameter - We are object oriented, so we pass This even
//  if we  don't need them.
//
#pragma warning ( disable : 4100 )

//
// This warning is caused by empty (after preprocessing) souce file.
//
#pragma warning ( disable : 4206 )

//
// Warning: The result of the unary '&' operator may be unaligned. Ignore it.
//
#pragma warning ( disable : 4366 )

#endif

#if defined(_MSC_EXTENSIONS)
  //
  // use Microsoft C complier dependent integer width types 
  //
  typedef unsigned __int64    uint64_t;
  typedef __int64             int64_t;
  typedef unsigned __int32    uint32_t;
  typedef __int32             int32_t;
  typedef unsigned short      uint16_t;
  typedef short               int16_t;
  typedef unsigned char       uint8_t;
  typedef signed char         int8_t;
#else
  typedef unsigned long long  uint64_t;
  typedef long long           int64_t;
  typedef unsigned int        uint32_t;
  typedef int                 int32_t;
  typedef unsigned short      uint16_t;
  typedef short               int16_t;
  typedef unsigned char       uint8_t;
  typedef signed char         int8_t;
#endif

//
// Native integer size in stdint.h
//
typedef uint64_t  uintn_t;
typedef int64_t   intn_t;

//
// Processor specific defines
//
#define EFI_MAX_BIT       0x8000000000000000ULL
#define MAX_2_BITS        0xC000000000000000ULL

//
// Maximum legal IA-32 address
//
#define EFI_MAX_ADDRESS   0xFFFFFFFFFFFFFFFFULL

//
//  Bad pointer value to use in check builds.
//  if you see this value you are using uninitialized or free'ed data
//
#define EFI_BAD_POINTER          0xAFAFAFAFAFAFAFAFULL
#define EFI_BAD_POINTER_AS_BYTE  0xAF

//
// Inject a break point in the code to assist debugging.
//
#define EFI_DEADLOOP()    { volatile int __iii; __iii = 1; while (__iii); }
#if _MSC_EXTENSIONS 
  #define EFI_BREAKPOINT()  __debugbreak()
#elif __GNUC__
  #define EFI_BREAKPOINT() asm("   int $3");
#endif

//
// Memory Fence forces serialization, and is needed to support out of order
//  memory transactions. The Memory Fence is mainly used to make sure IO
//  transactions complete in a deterministic sequence, and to syncronize locks
//  an other MP code. Currently no memory fencing is required.
//
#define MEMORY_FENCE()

//
// Some compilers don't support the forward reference construct:
//  typedef struct XXXXX. The forward reference is required for 
//  ANSI compatibility.
//
// The following macro provide a workaround for such cases.
//


#ifdef EFI_NO_INTERFACE_DECL
  #define EFI_FORWARD_DECLARATION(x)
#else
  #define EFI_FORWARD_DECLARATION(x) typedef struct _##x x
#endif


//
// Some C compilers optimize the calling conventions to increase performance.
// _EFIAPI is used to make all public APIs follow the standard C calling 
// convention.
//
#if _MSC_EXTENSIONS
  //
  // Microsoft* compiler requires _EFIAPI useage, __cdecl is Microsoft* specific C.
  // 

  #define _EFIAPI __cdecl  
#else
  #define _EFIAPI       
#endif


#ifdef _EFI_WINNT

  #define EFI_SUPPRESS_BENIGN_REDEFINITION_OF_TYPE_WARNING()  \
           warning ( disable : 4142 )

  #define EFI_DEFAULT_BENIGN_REDEFINITION_OF_TYPE_WARNING()  \
           warning ( default : 4142 )
#else

  #define EFI_SUPPRESS_BENIGN_REDEFINITION_OF_TYPE_WARNING()  \
           warning ( disable : 4068 )

  #define EFI_DEFAULT_BENIGN_REDEFINITION_OF_TYPE_WARNING()  \
           warning ( default : 4068 )

#endif

//
// For symbol name in GNU assembly code, an extra "_" is necessary
//
#if defined(__GNUC__)
  ///
  /// Private worker functions for ASM_PFX()
  ///
  #define _CONCATENATE(a, b)  __CONCATENATE(a, b)
  #define __CONCATENATE(a, b) a ## b

  ///
  /// The __USER_LABEL_PREFIX__ macro predefined by GNUC represents the prefix
  /// on symbols in assembly language.
  ///
  #define ASM_PFX(name) _CONCATENATE (__USER_LABEL_PREFIX__, name)

#endif

#endif

