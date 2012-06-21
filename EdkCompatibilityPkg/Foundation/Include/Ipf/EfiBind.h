/*++

Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiBind.h

Abstract:

  Processor or Compiler specific defines and types for Intel Itanium(TM).
  We are using the ANSI C 2000 _t type definitions for basic types.
  This it technically a violation of the coding standard, but they
  are used to make EfiTypes.h portable. Code other than EfiTypes.h
  should never use any ANSI C 2000 _t integer types.

--*/

#ifndef _EFI_BIND_H_
#define _EFI_BIND_H_


#define EFI_DRIVER_ENTRY_POINT(InitFunction)                 

#define EFI_APPLICATION_ENTRY_POINT EFI_DRIVER_ENTRY_POINT

#define ECP_CPU_IPF


//
// Make sure we are useing the correct packing rules per EFI specification
//
#pragma pack()


#if _MSC_EXTENSIONS 

#if __INTEL_COMPILER

//
// Disable the extra ";" warning;
// All places referencing EFI_GUID_STRING MACRO will generate this error.
//
#pragma warning ( disable : 424 )

//
// error #593: variable "Status" was set but never used
// This error may be flagged if a function only do ASSERT on return status when 
// EFI_DEBUG is not defined (EDK's ASSERT will be defined as empty statement).
// To make EdkCompatibilityPkg buildable by ICC with EFI_DEBUG undefined, disable
// this warning.
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

//
// Disable ICC's remark #869: "Parameter" was never referenced warning.
// This is legal ANSI C code so we disable the remark that is turned on with -Wall
//
#pragma warning ( disable : 869 )

#endif


    
//
// Disable warning that make it impossible to compile at /W4
// This only works for Microsoft tools. Copied from the 
// IA-32 version of efibind.h
//

//
// Disabling bitfield type checking warnings.
//
#pragma warning ( disable : 4214 )


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
// Can not cast a function pointer to a data pointer. We need to do this on 
// IPF to get access to the PLABEL.
//
#pragma warning ( disable : 4514 )

//
// Int64ShllMod32 unreferenced inline function
//
#pragma warning ( disable : 4054 )

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


#if (__STDC_VERSION__ < 199901L)
  //
  // No ANSI C 2000 stdint.h integer width declarations, so define equivalents
  //
 
  #if _MSC_EXTENSIONS 
    

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
    #ifdef _EFI_P64 
      //
      // P64 - is Intel Itanium(TM) speak for pointers being 64-bit and longs and ints 
      //  are 32-bits
      //
      typedef unsigned long long  uint64_t;
      typedef long long           int64_t;
      typedef unsigned int        uint32_t;
      typedef int                 int32_t;
      typedef unsigned short      uint16_t;
      typedef short               int16_t;
      typedef unsigned char       uint8_t;
      typedef signed char         int8_t;
    #else
      //
      // Assume LP64 - longs and pointers are 64-bit. Ints are 32-bit.
      //
      typedef unsigned long   uint64_t;
      typedef long            int64_t;
      typedef unsigned int    uint32_t;
      typedef int             int32_t;
      typedef unsigned short  uint16_t;
      typedef short           int16_t;
      typedef unsigned char   uint8_t;
      typedef signed char     int8_t;
    #endif
  #endif
#else
  //
  // Use ANSI C 2000 stdint.h integer width declarations
  //
  #include "stdint.h"
#endif

//
// Native integer size in stdint.h
//
typedef uint64_t  uintn_t;
typedef int64_t   intn_t;

//
// Processor specific defines
//
#define EFI_MAX_BIT  0x8000000000000000
#define MAX_2_BITS   0xC000000000000000

//
// Maximum legal Itanium-based address
//
#define EFI_MAX_ADDRESS   0xFFFFFFFFFFFFFFFF

//
//  Bad pointer value to use in check builds.
//  if you see this value you are using uninitialized or free'ed data
//
#define EFI_BAD_POINTER          0xAFAFAFAFAFAFAFAF
#define EFI_BAD_POINTER_AS_BYTE  0xAF

#define EFI_DEADLOOP()    while(TRUE)

#ifdef __GNUC__
#define EFI_BREAKPOINT  EcpEfiBreakPoint
#define MEMORY_FENCE    EcpMemoryFence
#else
//
// Inject a break point in the code to assist debugging.
//
#pragma intrinsic (__break)  
#define EFI_BREAKPOINT()  __break(0)
#define EFI_DEADLOOP()    while(TRUE)

//
// Memory Fence forces serialization, and is needed to support out of order
//  memory transactions. The Memory Fence is mainly used to make sure IO
//  transactions complete in a deterministic sequence, and to syncronize locks
//  an other MP code. Intel Itanium(TM) processors require explicit memory fence instructions
//  after every IO. Need to find a way of doing that in the function _mf.
//
void __mfa (void);                       
#pragma intrinsic (__mfa)  
#define MEMORY_FENCE()  __mfa()
#endif


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


#endif

