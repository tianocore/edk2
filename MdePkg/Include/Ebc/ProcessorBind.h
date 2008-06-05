/** @file
  Processor or compiler specific defines and types for EBC.

  We currently only have one EBC complier so there may be some Intel compiler
  specific functions in this file.

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

//
// Define the processor type so other code can make processor based choices
//
#define MDE_CPU_EBC

//
// Native integer types
//
typedef char                  INT8;
typedef unsigned char         BOOLEAN;
typedef unsigned char         UINT8;
typedef char                  CHAR8;

typedef short                 INT16;
typedef unsigned short        UINT16;
typedef unsigned short        CHAR16;

typedef int                   INT32;
typedef unsigned int          UINT32;

typedef __int64               INT64;
typedef unsigned __int64      UINT64;

//
// "long" type scales to the processor native size with EBC compiler
//
typedef long                  INTN;
typedef unsigned long         UINTN;

#define UINT8_MAX 0xff

//
// Scalable macro to set the most significant bit in a natural number
//
#define MAX_BIT     (1ULL << (sizeof (INTN) * 8 - 1)) 
#define MAX_2_BITS  (3ULL << (sizeof (INTN) * 8 - 2))

//
// Maximum legal EBC address
//
#define MAX_ADDRESS   ((UINTN) ~0)

//
// The stack alignment required for EBC
//
#define CPU_STACK_ALIGNMENT   sizeof(UINTN)

//
// Modifier to ensure that all protocol member functions and EFI intrinsics
// use the correct C calling convention. All protocol member functions and
// EFI intrinsics are required to modify thier member functions with EFIAPI.
//
#define EFIAPI    

//
// The Microsoft* C compiler can removed references to unreferenced data items
//  if the /OPT:REF linker option is used. We defined a macro as this is a 
//  a non standard extension. Currently not supported by the EBC compiler
//
#define GLOBAL_REMOVE_IF_UNREFERENCED

#define FUNCTION_ENTRY_POINT(p) (p)

#endif 

