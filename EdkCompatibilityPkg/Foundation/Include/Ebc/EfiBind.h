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

  Processor or compiler specific defines and types for EBC.

--*/

#ifndef _EFI_BIND_H_
#define _EFI_BIND_H_

#define EFI_DRIVER_ENTRY_POINT(InitFunction)
#define EFI_APPLICATION_ENTRY_POINT EFI_DRIVER_ENTRY_POINT

//
// Disable warning that make it impossible to compile at /W3
// This only works for Intel EBC Compiler tools
//

//
// Disabling argument of type "TYPE **" is incompatible with parameter of type "void **"
//
#pragma warning ( disable : 167 )

//
// Disabling pointless comparison of unsigned integer with zero
//
#pragma warning ( disable : 186 )

//
// Disabling enumerated type mixed with another type
//
#pragma warning ( disable : 188 )

//
// Native integer types
//
typedef signed char           int8_t;
typedef unsigned char         uint8_t;

typedef short                 int16_t;
typedef unsigned short        uint16_t;

typedef int                   int32_t;
typedef unsigned int          uint32_t;

typedef __int64               int64_t;
typedef unsigned __int64      uint64_t;

//
// "long" type scales to the processor native size with EBC compiler
//
typedef long                  intn_t;
typedef unsigned long         uintn_t;

//
// Scalable macro to set the most significant bit in a natural number
//
#define EFI_MAX_BIT           ((UINTN)0x01 << ((sizeof (char *) * 8) - 1))
#define MAX_2_BITS            (EFI_MAX_BIT | (EFI_MAX_BIT >> 1))

//
// Maximum legal EBC address
//
#define EFI_MAX_ADDRESS   (UINTN)~0

//
//  Bad pointer value to use in check builds.
//  if you see this value you are using uninitialized or free'ed data
//
#define EFI_BAD_POINTER          (UINTN)0xAFAFAFAFAFAFAFAF
#define EFI_BAD_POINTER_AS_BYTE  (UINTN)0xAF

//
// _break() is an EBC compiler intrinsic function
//
extern 
uint64_t 
_break (
  unsigned char BreakCode
  );

//
// Macro to inject a break point in the code to assist debugging.
//
#define EFI_BREAKPOINT()  _break ( 3 )
#define EFI_DEADLOOP()    while (TRUE)

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


#define _EFIAPI       

#endif // ifndef _EFI_BIND_H_

