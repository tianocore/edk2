/** @file
  IPF specific debugsupport types, macros, and definitions.
  
Copyright (c) 2004 - 2006 Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _PLDEBUG_SUPPORT_H_
#define _PLDEBUG_SUPPORT_H_


#include <Uefi.h>

#include <Protocol/DebugSupport.h>
#include <Protocol/LoadedImage.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define DISABLE_INTERRUPTS  0UL

//
// The remaining definitions comprise the protocol members.
//
#define EFI_ISA IsaIpf

/**
  IPF specific DebugSupport driver initialization. 

  Must be public because it's referenced from DebugSupport.c

  @retval  EFI_SUCCESS     Always.

**/
EFI_STATUS
PlInitializeDebugSupportDriver (
  VOID
  );

/**
  Unload handler that is called during UnloadImage() - deallocates pool memory
  used by the driver.  Must be public because it's referenced from DebugSuport.c

  @param  ImageHandle    The firmware allocated handle for the EFI image.

  @retval EFI_SUCCESS    Always.

**/
EFI_STATUS
EFIAPI
PlUnloadDebugSupportDriver (
  IN EFI_HANDLE                   ImageHandle
  );

/**
  C callable function to obtain the current value of IVA.

  @return Current value of IVA.

**/
VOID  *
GetIva (
  VOID
  );

/**
  HookStub will be copied from it's loaded location into the IVT when
  an IVT entry is hooked.

**/
VOID
HookStub (
  VOID
  );

/**
  Chains an interrupt handler.

**/
VOID
ChainHandler (
  VOID
  );

/**
  Unchains an interrupt handler.

**/
VOID
UnchainHandler (
  VOID
  );

/**
  C callable function to enable/disable interrupts.

  @param  NewInterruptState   New Interrupt State.

  @return Previous state of psr.ic.

**/
UINT64
ProgramInterruptFlags (
  IN UINT64                       NewInterruptState
  );

/**
  Flushes instruction cache for specified number of bytes.

  @param  StartAddress     Cache Start Address.
  @param  SizeInBytes      Cache Size.

**/
VOID
InstructionCacheFlush (
  IN VOID    *StartAddress,
  IN UINTN   SizeInBytes
  );

/**
  This is a DebugSupport protocol member function, hard
  coded to support only 1 processor for now.

  @param  This                The DebugSupport instance
  @param  MaxProcessorIndex   The maximuim supported processor index

  @retval EFI_SUCCESS         Always returned with **MaxProcessorIndex set to 0.

**/
EFI_STATUS
EFIAPI
GetMaximumProcessorIndex (
  IN EFI_DEBUG_SUPPORT_PROTOCOL   *This,
  OUT UINTN                       *MaxProcessorIndex
  );

/**
  DebugSupport protocol member function.

  @param  This               The DebugSupport instance
  @param  ProcessorIndex     Which processor the callback applies to.
  @param  PeriodicCallback   Callback function

  @retval EFI_SUCCESS        Indicates the callback was registered.
  @retval others             Callback was not registered.

**/
EFI_STATUS
EFIAPI
RegisterPeriodicCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL   *This,
  IN UINTN                        ProcessorIndex,
  IN EFI_PERIODIC_CALLBACK        PeriodicCallback
  );

/**
  DebugSupport protocol member function.

  @param  This              The DebugSupport instance
  @param  ProcessorIndex    Which processor the callback applies to.
  @param  NewCallback       Callback function
  @param  ExceptionType     Which exception to hook

  @retval EFI_SUCCESS        Indicates the callback was registered.
  @retval others             Callback was not registered.

**/
EFI_STATUS
EFIAPI
RegisterExceptionCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL   *This,
  IN UINTN                        ProcessorIndex,
  IN EFI_EXCEPTION_CALLBACK       NewCallback,
  IN EFI_EXCEPTION_TYPE           ExceptionType
  );

/**
  DebugSupport protocol member function.  Calls assembly routine to flush cache.

  @param  This              The DebugSupport instance
  @param  ProcessorIndex    Which processor the callback applies to.
  @param  Start             Physical base of the memory range to be invalidated
  @param  Length            mininum number of bytes in instruction cache to invalidate

  @retval EFI_SUCCESS       Always returned.

**/
EFI_STATUS
EFIAPI
InvalidateInstructionCache (
  IN EFI_DEBUG_SUPPORT_PROTOCOL   *This,
  IN UINTN                        ProcessorIndex,
  IN VOID                         *Start,
  IN UINTN                        Length
  );

/**
  C routine that is called for all registered exceptions.  This is the main
  exception dispatcher.  Must be public because it's referenced from AsmFuncs.s.

  @param  ExceptionType        Exception Type
  @param  Context              System Context
**/
VOID
CommonHandler (
  IN EFI_EXCEPTION_TYPE ExceptionType,
  IN EFI_SYSTEM_CONTEXT Context
  );

#endif
