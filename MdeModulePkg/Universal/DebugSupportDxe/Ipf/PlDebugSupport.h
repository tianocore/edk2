/**@file
  IPF specific debugsupport types, macros, and definitions.
  
Copyright (c) 2004 - 2006 Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _PLDEBUG_SUPPORT_H
#define _PLDEBUG_SUPPORT_H


#include <Uefi.h>

#include <Protocol/DebugSupport.h>
#include <Protocol/LoadedImage.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>

#define DISABLE_INTERRUPTS  0UL

//
// The remaining definitions comprise the protocol members.
//
#define EFI_ISA IsaIpf

//
// processor specific functions that must be public
//
EFI_STATUS
plInitializeDebugSupportDriver (
  VOID
  )
/*++

Routine Description:
  IPF specific DebugSupport driver initialization.  Must be public because it's
  referenced from DebugSupport.c

Arguments:

Returns:

  EFI_SUCCESS

--*/
;

EFI_STATUS
EFIAPI
plUnloadDebugSupportDriver (
  IN EFI_HANDLE                   ImageHandle
  )
/*++

Routine Description:
  Unload handler that is called during UnloadImage() - deallocates pool memory
  used by the driver.  Must be public because it's referenced from DebugSuport.c

Arguments:
  ImageHandle - Image handle

Returns:

  EFI_STATUS - anything other than EFI_SUCCESS indicates the callback was not registered.

--*/
;

//
// Assembly worker functions and data referenced from PlDebugSupport.c
//
VOID  *
GetIva (
  VOID
  )
/*++

Routine Description:

  C callable function to obtain the current value of IVA

Arguments:

  None

Returns:

  Current value if IVA

--*/
;

VOID
HookStub (
  VOID
  )
/*++

Routine Description:

  HookStub will be copied from it's loaded location into the IVT when
  an IVT entry is hooked.

Arguments:

  None

Returns:

  None

--*/
;

VOID
ChainHandler (
  VOID
  )
/*++

Routine Description:

  Chains an interrupt handler

Arguments:

  None

Returns:

  None

--*/
;

VOID
UnchainHandler (
  VOID
  )
/*++

Routine Description:

  Unchains an interrupt handler

Arguments:

  None

Returns:

  None

--*/
;

UINT64
ProgramInterruptFlags (
  IN UINT64                       NewInterruptState
  )
/*++

Routine Description:

  C callable function to enable/disable interrupts

Arguments:

  NewInterruptState - New Interrupt State

Returns:

  Previous state of psr.ic

--*/
;

VOID
InstructionCacheFlush (
  IN VOID    *StartAddress,
  IN UINTN   SizeInBytes
  )
/*++

Routine Description:

  Flushes instruction cache for specified number of bytes

Arguments:

  StartAddress  - Cache Start Address
  SizeInBytes   - Cache Size

Returns:

  None

--*/
;

EFI_STATUS
EFIAPI
GetMaximumProcessorIndex (
  IN EFI_DEBUG_SUPPORT_PROTOCOL   *This,
  OUT UINTN                       *MaxProcessorIndex
  )
/*++

Routine Description: This is a DebugSupport protocol member function.  Hard
  coded to support only 1 processor for now.

Arguments:
  This              - The DebugSupport instance
  MaxProcessorIndex - The maximuim supported processor index

Returns:
  Always returns EFI_SUCCESS with *MaxProcessorIndex set to 0

--*/
;

EFI_STATUS
EFIAPI
RegisterPeriodicCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL   *This,
  IN UINTN                        ProcessorIndex,
  IN EFI_PERIODIC_CALLBACK        PeriodicCallback
  )
/*++

Routine Description:
  DebugSupport protocol member function

Arguments:
  This             - The DebugSupport instance
  ProcessorIndex   - Which processor the callback applies to.
  PeriodicCallback - Callback function

Returns:

  EFI_STATUS - anything other than EFI_SUCCESS indicates the callback was not registered.

--*/
;

EFI_STATUS
EFIAPI
RegisterExceptionCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL   *This,
  IN UINTN                        ProcessorIndex,
  IN EFI_EXCEPTION_CALLBACK       NewHandler,
  IN EFI_EXCEPTION_TYPE           ExceptionType
  )
/*++

Routine Description:
  DebugSupport protocol member function

Arguments:
  This             - The DebugSupport instance
  ProcessorIndex   - Which processor the callback applies to.
  NewCallback      - Callback function
  ExceptionType    - Which exception to hook

Returns:

  EFI_STATUS - anything other than EFI_SUCCESS indicates the callback was not registered.

--*/
;

EFI_STATUS
EFIAPI
InvalidateInstructionCache (
  IN EFI_DEBUG_SUPPORT_PROTOCOL   *This,
  IN UINTN                        ProcessorIndex,
  IN VOID                         *Start,
  IN UINTN                        Length
  )
/*++

Routine Description:
  DebugSupport protocol member function.  Calls assembly routine to flush cache.

Arguments:
  This             - The DebugSupport instance
  ProcessorIndex   - Which processor the callback applies to.
  Start            - Physical base of the memory range to be invalidated
  Length           - mininum number of bytes in instruction cache to invalidate

Returns:
  EFI_SUCCESS

--*/
;

VOID
CommonHandler (
  IN EFI_EXCEPTION_TYPE ExceptionType,
  IN EFI_SYSTEM_CONTEXT Context
  )
/*++

Routine Description:
  C routine that is called for all registered exceptions.  This is the main
  exception dispatcher.  Must be public because it's referenced from AsmFuncs.s.

Arguments:
  ExceptionType - Exception Type
  Context       - System Context

Returns:

  Nothing
  
--*/
;

#endif
