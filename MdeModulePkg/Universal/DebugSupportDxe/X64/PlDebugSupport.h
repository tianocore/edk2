/** @file
  X64 specific debug support macros, typedefs and prototypes.

Copyright (c) 2006 - 2008, Intel Corporation                                                         
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
#include <Library/BaseLib.h>

#include <Library/PcdLib.h>
#define NUM_IDT_ENTRIES                 0x78
#define SYSTEM_TIMER_VECTOR             0x68
#define VECTOR_ENTRY_PAGES              1
#define COPY_DESCRIPTOR(Dest, Src)       CopyMem ((Dest), (Src), sizeof (DESCRIPTOR))
#define READ_IDT(Vector, Dest)           COPY_DESCRIPTOR ((Dest), &((GetIdtr ())[(Vector)]))
#define WRITE_IDT(Vector, Src)           COPY_DESCRIPTOR (&((GetIdtr ())[(Vector)]), (Src))
#define COMPARE_DESCRIPTOR(Desc1, Desc2) CompareMem ((Desc1), (Desc2), sizeof (DESCRIPTOR))
#define EFI_ISA                         IsaX64
#define FF_FXSR                         (1 << 24)

typedef struct {
  UINT64 Low;
  UINT64 High;
} DESCRIPTOR;

typedef
VOID
(*DEBUG_PROC) (
  VOID
  );

typedef struct {
  DESCRIPTOR  OrigDesc;
  DEBUG_PROC  OrigVector;
  DESCRIPTOR  NewDesc;
  DEBUG_PROC  StubEntry;
  VOID (*RegisteredCallback) ();
} IDT_ENTRY;

extern EFI_SYSTEM_CONTEXT SystemContext;
extern UINT8              InterruptEntryStub[];
extern UINT32             StubSize;
extern VOID (*OrigVector) (VOID);

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
  Return the physical address of IDTR.

  @return The physical address of IDTR.

**/
DESCRIPTOR  *
GetIdtr (
  VOID
  );

/**
  Encodes an IDT descriptor with the given physical address.

  @param  DestDesc    The IDT descriptor address.
  @param  Vecotr      The interrupt vector entry.

**/
VOID
Vect2Desc (
  DESCRIPTOR * DestDesc,
  VOID (*Vector) (VOID)
  );

/**
  Programs interrupt flag to the requested state and returns previous
  state.

  @param  NewState    New interrupt status.

  @retval TRUE     Old interrupt status is TRUE.
  @retval FALSE    Old interrupt status is FALSE

**/
BOOLEAN
WriteInterruptFlag (
  BOOLEAN NewState
  );

/**
  Initializes driver's handler registration databas. 
  
  This code executes in boot services context
  Must be public because it's referenced from DebugSupport.c

  @retval  EFI_UNSUPPORTED      If x64 processor does not support FXSTOR/FXRSTOR instructions,
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
  This is a DebugSupport protocol member function, hard
  coded to support only 1 processor for now.

  @param  This                The DebugSupport instance
  @param  MaxProcessorIndex   The maximuim supported processor index

  @retval EFI_SUCCESS         Always returned with **MaxProcessorIndex set to 0.

**/
EFI_STATUS
EFIAPI
GetMaximumProcessorIndex (
  IN EFI_DEBUG_SUPPORT_PROTOCOL       *This,
  OUT UINTN                           *MaxProcessorIndex
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
  IN EFI_DEBUG_SUPPORT_PROTOCOL       *This,
  IN UINTN                            ProcessorIndex,
  IN EFI_PERIODIC_CALLBACK            PeriodicCallback
  );

/**
  DebugSupport protocol member function.

  This code executes in boot services context.

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
  IN EFI_DEBUG_SUPPORT_PROTOCOL       *This,
  IN UINTN                            ProcessorIndex,
  IN EFI_EXCEPTION_CALLBACK           NewCallback,
  IN EFI_EXCEPTION_TYPE               ExceptionType
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
  IN EFI_DEBUG_SUPPORT_PROTOCOL       *This,
  IN UINTN                            ProcessorIndex,
  IN VOID                             *Start,
  IN UINT64                           Length
  );

#endif
