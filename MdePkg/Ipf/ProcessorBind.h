/** @file
  Processor or Compiler specific defines and types for Intel Itanium(TM).

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  ProcessorBind.h

**/

#ifndef __PROCESSOR_BIND_H__
#define __PROCESSOR_BIND_H__


//
// Define the processor type so other code can make processor based choices
//
#define MDE_CPU_IPF


//
// Make sure we are useing the correct packing rules per EFI specification
//
#pragma pack()


#if _MSC_EXTENSIONS 
    
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
// Disable warning on conversion from function pointer to a data pointer
//
#pragma warning ( disable : 4054 )

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
// This warning is caused by functions defined but not used. For precompiled header only.
//
#pragma warning ( disable : 4505 )

//
// This warning is caused by empty (after preprocessing) souce file. For precompiled header only.
//
#pragma warning ( disable : 4206 )

#endif


#if (__STDC_VERSION__ < 199901L)
  //
  // No ANSI C 2000 stdint.h integer width declarations, so define equivalents
  //
 
  #if _MSC_EXTENSIONS 
    

    //
    // use Microsoft C complier dependent interger width types 
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
    #ifdef _EFI_P64 
      //
      // P64 - is Intel Itanium(TM) speak for pointers being 64-bit and longs and ints 
      //  are 32-bits
      //
      typedef unsigned long long  UINT64;
      typedef long long           INT64;
      typedef unsigned int        UINT32;
      typedef int                 INT32;
      typedef unsigned short      CHAR16;
      typedef unsigned short      UINT16;
      typedef short               INT16;
      typedef unsigned char       BOOLEAN;
      typedef unsigned char       UINT8;
      typedef char                CHAR8;
      typedef char                INT8;
    #else
      //
      // Assume LP64 - longs and pointers are 64-bit. Ints are 32-bit.
      //
      typedef unsigned long   UINT64;
      typedef long            INT64;
      typedef unsigned int    UINT32;
      typedef int             INT32;
      typedef unsigned short  UINT16;
      typedef unsigned short  CHAR16;
      typedef short           INT16;
      typedef unsigned char   BOOLEAN;
      typedef unsigned char   UINT8;
      typedef char            CHAR8;
      typedef char            INT8;
    #endif
  #endif

  #define UINT8_MAX 0xff

#else
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

#endif

typedef UINT64  UINTN;
typedef INT64   INTN;


//
// Processor specific defines
//
#define MAX_BIT     0x8000000000000000ULL
#define MAX_2_BITS  0xC000000000000000ULL

//
// Maximum legal Itanium-based address
//
#define MAX_ADDRESS   0xFFFFFFFFFFFFFFFFULL

//
// Per the Itanium Software Conventions and Runtime Architecture Guide,
// section 3.3.4, IPF stack must always be 16-byte aligned.
//
#define CPU_STACK_ALIGNMENT   16

//
// Modifier to ensure that all protocol member functions and EFI intrinsics
// use the correct C calling convention. All protocol member functions and
// EFI intrinsics are required to modify thier member functions with EFIAPI.
//
#if _MSC_EXTENSIONS 
  //
  // Microsoft* compiler requires _EFIAPI useage, __cdecl is Microsoft* specific C.
  // 
  #define EFIAPI __cdecl  
#else
  #define EFIAPI       
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
// A pointer to a function in IPF points to a plabel.
//
typedef struct {
  UINT64  EntryPoint;
  UINT64  GP;
} EFI_PLABEL;

#include <IpfDefines.h>

#endif

