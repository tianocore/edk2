/** @file
  Generic debug support macros, typedefs and prototypes for IA32/x64.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _DEBUG_SUPPORT_H_
#define _DEBUG_SUPPORT_H_

#include <Uefi.h>

#include <Protocol/DebugSupport.h>
#include <Protocol/LoadedImage.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>

#define NUM_IDT_ENTRIES                 0x78
#define SYSTEM_TIMER_VECTOR             0x68

typedef
VOID
(*DEBUG_PROC) (
  VOID
  );

typedef
VOID
(EFIAPI *CALLBACK_FUNC) (
  );

typedef struct {
  IA32_IDT_GATE_DESCRIPTOR  OrigDesc;
  DEBUG_PROC                OrigVector;
  IA32_IDT_GATE_DESCRIPTOR  NewDesc;
  DEBUG_PROC                StubEntry;
  CALLBACK_FUNC             RegisteredCallback;
} IDT_ENTRY;

extern UINT8                     InterruptEntryStub[];
extern UINT32                    StubSize;
extern VOID                      (*OrigVector) (VOID);
extern IDT_ENTRY                 *IdtEntryTable;
extern IA32_IDT_GATE_DESCRIPTOR  NullDesc;

/**
  Generic IDT entry.

**/
VOID
CommonIdtEntry (
  VOID
  );

/**
  Check whether FXSTOR is supported

  @retval TRUE   FXSTOR is supported.
  @retval FALSE  FXSTOR is not supported.

**/
BOOLEAN
FxStorSupport (
  VOID
  );

/**
  Encodes an IDT descriptor with the given physical address.

  @param  DestDesc    The IDT descriptor address.
  @param  Vecotr      The interrupt vector entry.

**/
VOID
Vect2Desc (
  IA32_IDT_GATE_DESCRIPTOR * DestDesc,
  VOID (*Vector) (VOID)
  );

/**
  Initializes driver's handler registration database. 
  
  This code executes in boot services context
  Must be public because it's referenced from DebugSupport.c

  @retval  EFI_UNSUPPORTED      If IA32 processor does not support FXSTOR/FXRSTOR instructions,
                                the context save will fail, so these processor's are not supported.
  @retval  EFI_OUT_OF_RESOURCES Fails to allocate memory.
  @retval  EFI_SUCCESS          Initializes successfully.

**/
EFI_STATUS
PlInitializeDebugSupportDriver (
  VOID
  );

/**
  This is the callback that is written to the LoadedImage protocol instance
  on the image handle. It uninstalls all registered handlers and frees all entry
  stub memory.

  @param  ImageHandle    The firmware allocated handle for the EFI image.

  @retval EFI_SUCCESS    Always.

**/
EFI_STATUS
EFIAPI
PlUnloadDebugSupportDriver (
  IN EFI_HANDLE                       ImageHandle
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
  IN EFI_DEBUG_SUPPORT_PROTOCOL       *This,
  OUT UINTN                           *MaxProcessorIndex
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
  IN EFI_DEBUG_SUPPORT_PROTOCOL       *This,
  IN UINTN                            ProcessorIndex,
  IN EFI_PERIODIC_CALLBACK            PeriodicCallback
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
  IN EFI_DEBUG_SUPPORT_PROTOCOL       *This,
  IN UINTN                            ProcessorIndex,
  IN EFI_EXCEPTION_CALLBACK           ExceptionCallback,
  IN EFI_EXCEPTION_TYPE               ExceptionType
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
  IN EFI_DEBUG_SUPPORT_PROTOCOL       *This,
  IN UINTN                            ProcessorIndex,
  IN VOID                             *Start,
  IN UINT64                           Length
  );

/**
  Allocate pool for a new IDT entry stub.

  Copy the generic stub into the new buffer and fixup the vector number
  and jump target address.

  @param  ExceptionType   This is the exception type that the new stub will be created
                          for.
  @param  Stub            On successful exit, *Stub contains the newly allocated entry stub.

**/
VOID
CreateEntryStub (
  IN EFI_EXCEPTION_TYPE     ExceptionType,
  OUT VOID                  **Stub
  );

/**
  Get Interrupt Handle from IDT Gate Descriptor.

  @param  IdtGateDecriptor  IDT Gate Descriptor.

  @return Interrupt Handle stored in IDT Gate Descriptor.

**/
UINTN
GetInterruptHandleFromIdt (
  IN IA32_IDT_GATE_DESCRIPTOR  *IdtGateDecriptor
  );

/**
  This is the main worker function that manages the state of the interrupt
  handlers.  It both installs and uninstalls interrupt handlers based on the
  value of NewCallback.  If NewCallback is NULL, then uninstall is indicated.
  If NewCallback is non-NULL, then install is indicated.

  @param  NewCallback   If non-NULL, NewCallback specifies the new handler to register.
                        If NULL, specifies that the previously registered handler should
                        be uninstalled.
  @param  ExceptionType Indicates which entry to manage.

  @retval EFI_SUCCESS            Process is ok.
  @retval EFI_INVALID_PARAMETER  Requested uninstalling a handler from a vector that has
                                 no handler registered for it
  @retval EFI_ALREADY_STARTED    Requested install to a vector that already has a handler registered.
  @retval others                 Possible return values are passed through from UnHookEntry and HookEntry.

**/
EFI_STATUS
ManageIdtEntryTable (
  CALLBACK_FUNC      NewCallback,
  EFI_EXCEPTION_TYPE ExceptionType
  );

/**
  Creates a nes entry stub.  Then saves the current IDT entry and replaces it
  with an interrupt gate for the new entry point.  The IdtEntryTable is updated
  with the new registered function.

  This code executes in boot services context.  The stub entry executes in interrupt
  context.

  @param  ExceptionType      Specifies which vector to hook.
  @param  NewCallback        A pointer to the new function to be registered.

**/
VOID
HookEntry (
  IN EFI_EXCEPTION_TYPE            ExceptionType,
  IN CALLBACK_FUNC                 NewCallback
  );

/**
  Undoes HookEntry. This code executes in boot services context.

  @param  ExceptionType   Specifies which entry to unhook

**/
VOID
UnhookEntry (
  IN EFI_EXCEPTION_TYPE           ExceptionType
  );

#endif
