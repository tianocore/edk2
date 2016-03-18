/** @file
  IPF specific types, macros, and definitions for Debug Support Driver.

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
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

#define EFI_ISA IsaIpf

typedef struct {
  UINT64  Low;
  UINT64  High;
} BUNDLE;

typedef
VOID
(*CALLBACK_FUNC) (
  );

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
  used by the driver.

  Must be public because it's referenced from DebugSuport.c

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
  C callable function that HookStub will be copied from it's loaded location into the IVT when
  an IVT entry is hooked.

**/
VOID
HookStub (
  VOID
  );

/**
  C callable function to chain an interrupt handler.

**/
VOID
ChainHandler (
  VOID
  );

/**
  C callable function to unchain an interrupt handler.

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
  Returns the maximum value that may be used for the ProcessorIndex parameter in
  RegisterPeriodicCallback() and RegisterExceptionCallback().

  Hard coded to support only 1 processor for now.

  @param  This                  A pointer to the EFI_DEBUG_SUPPORT_PROTOCOL instance.
  @param  MaxProcessorIndex     Pointer to a caller-allocated UINTN in which the maximum supported
                                processor index is returned. Always 0 returned.

  @retval EFI_SUCCESS           Always returned with **MaxProcessorIndex set to 0.

**/
EFI_STATUS
EFIAPI
GetMaximumProcessorIndex (
  IN EFI_DEBUG_SUPPORT_PROTOCOL   *This,
  OUT UINTN                       *MaxProcessorIndex
  );

/**
  Registers a function to be called back periodically in interrupt context.

  @param  This                  A pointer to the EFI_DEBUG_SUPPORT_PROTOCOL instance.
  @param  ProcessorIndex        Specifies which processor the callback function applies to.
  @param  PeriodicCallback      A pointer to a function of type PERIODIC_CALLBACK that is the main
                                periodic entry point of the debug agent.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_ALREADY_STARTED   Non-NULL PeriodicCallback parameter when a callback
                                function was previously registered.
  @retval EFI_OUT_OF_RESOURCES  System has insufficient memory resources to register new callback
                                function.
**/
EFI_STATUS
EFIAPI
RegisterPeriodicCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL   *This,
  IN UINTN                        ProcessorIndex,
  IN EFI_PERIODIC_CALLBACK        PeriodicCallback
  );

/**
  Registers a function to be called when a given processor exception occurs.

  This code executes in boot services context.

  @param  This                  A pointer to the EFI_DEBUG_SUPPORT_PROTOCOL instance.
  @param  ProcessorIndex        Specifies which processor the callback function applies to.
  @param  ExceptionCallback     A pointer to a function of type EXCEPTION_CALLBACK that is called
                                when the processor exception specified by ExceptionType occurs.  
  @param  ExceptionType         Specifies which processor exception to hook.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_ALREADY_STARTED   Non-NULL PeriodicCallback parameter when a callback
                                function was previously registered.
  @retval EFI_OUT_OF_RESOURCES  System has insufficient memory resources to register new callback
                                function.
**/
EFI_STATUS
EFIAPI
RegisterExceptionCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL   *This,
  IN UINTN                        ProcessorIndex,
  IN EFI_EXCEPTION_CALLBACK       ExceptionCallback,
  IN EFI_EXCEPTION_TYPE           ExceptionType
  );

/**
  Invalidates processor instruction cache for a memory range. Subsequent execution in this range
  causes a fresh memory fetch to retrieve code to be executed.

  @param  This                  A pointer to the EFI_DEBUG_SUPPORT_PROTOCOL instance.
  @param  ProcessorIndex        Specifies which processor's instruction cache is to be invalidated.
  @param  Start                 Specifies the physical base of the memory range to be invalidated.
  @param  Length                Specifies the minimum number of bytes in the processor's instruction
                                cache to invalidate.

  @retval EFI_SUCCESS           Always returned.

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
  exception dispatcher.

  Must be public because it's referenced from AsmFuncs.s.

  @param  ExceptionType        Specifies which processor exception.
  @param  Context              System Context.
**/
VOID
CommonHandler (
  IN EFI_EXCEPTION_TYPE ExceptionType,
  IN EFI_SYSTEM_CONTEXT Context
  );

/**
  This is the worker function that uninstalls and removes all handlers.

  @param  ExceptionType     Specifies which processor exception.
  @param  NewBundles        New Boundles.
  @param  NewCallback       A pointer to the new function to be registered.

  @retval EFI_ALEADY_STARTED Ivt already hooked.
  @retval EFI_SUCCESS        Successfully uninstalled.

**/
EFI_STATUS
ManageIvtEntryTable (
  IN  EFI_EXCEPTION_TYPE    ExceptionType,
  IN  BUNDLE                NewBundles[4],
  IN  CALLBACK_FUNC         NewCallback
  );

/**
  Saves original IVT contents and inserts a few new bundles which are fixed up
  to store the ExceptionType and then call the common handler.

  @param  ExceptionType      Specifies which processor exception.
  @param  NewBundles         New Boundles.
  @param  NewCallback        A pointer to the new function to be hooked.

**/
VOID
HookEntry (
  IN  EFI_EXCEPTION_TYPE    ExceptionType,
  IN  BUNDLE                NewBundles[4],
  IN  CALLBACK_FUNC         NewCallback
  );

/**
  Restores original IVT contents when unregistering a callback function.

  @param  ExceptionType     Specifies which processor exception.

**/
VOID
UnhookEntry (
  IN  EFI_EXCEPTION_TYPE    ExceptionType
  );

/**
  Sets up cache flush and calls assembly function to chain external interrupt.

  Records new callback in IvtEntryTable.

  @param  NewCallback     A pointer to the interrupt handle.

**/
VOID
ChainExternalInterrupt (
  IN  CALLBACK_FUNC         NewCallback
  );

/**
  Sets up cache flush and calls assembly function to restore external interrupt.
  Removes registered callback from IvtEntryTable.

**/
VOID
UnchainExternalInterrupt (
  VOID
  );

/**
  Given an integer number, return the physical address of the entry point in the IFT.

  @param  HandlerIndex       Index of the Handler
  @param  EntryPoint         IFT Entrypoint

**/
VOID
GetHandlerEntryPoint (
  UINTN                     HandlerIndex,
  VOID                      **EntryPoint
  );

#endif
