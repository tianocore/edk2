/**@file
  X64 specific debug support macros, typedefs and prototypes.

Copyright (c) 2006, Intel Corporation                                                         
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
#define NUM_IDT_ENTRIES                 0x78
#define SYSTEM_TIMER_VECTOR             0x68
#define VECTOR_ENTRY_PAGES              1
#define CopyDescriptor(Dest, Src)       CopyMem ((Dest), (Src), sizeof (DESCRIPTOR))
#define ZeroDescriptor(Dest)            CopyDescriptor ((Dest), &NullDesc)
#define ReadIdt(Vector, Dest)           CopyDescriptor ((Dest), &((GetIdtr ())[(Vector)]))
#define WriteIdt(Vector, Src)           CopyDescriptor (&((GetIdtr ())[(Vector)]), (Src))
#define CompareDescriptor(Desc1, Desc2) CompareMem ((Desc1), (Desc2), sizeof (DESCRIPTOR))
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
  )
;

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

VOID
CommonIdtEntry (
  VOID
  )
/*++

Routine Description:

  Generic IDT entry

Arguments:

  None

Returns:

  None

--*/
;


BOOLEAN
FxStorSupport (
  VOID
  )
/*++

Routine Description:

  Check whether FXSTOR is supported

Arguments:

  None

Returns:

  TRUE  - supported
  FALSE - not supported

--*/
;

DESCRIPTOR  *
GetIdtr (
  VOID
  )
/*++

Routine Description:

  Return the physical address of IDTR

Arguments:

  None

Returns:

  The physical address of IDTR

--*/
;

VOID
Vect2Desc (
  DESCRIPTOR * DestDesc,
  VOID (*Vector) (VOID)
  )
/*++

Routine Description:

  Encodes an IDT descriptor with the given physical address

Arguments:

  DestDesc  - The IDT descriptor address
  Vector    - The interrupt vector entry

Returns:

  None

--*/
;

BOOLEAN
WriteInterruptFlag (
  BOOLEAN NewState
  )
/*++

Routine Description:

  Programs interrupt flag to the requested state and returns previous
  state.

Arguments:

  NewState  - New interrupt status

Returns:

  Old interrupt status

--*/
;

EFI_STATUS
plInitializeDebugSupportDriver (
  VOID
  )
/*++

Routine Description:
  Initializes driver's handler registration database.

  This code executes in boot services context.

Arguments:
  None

Returns:
  EFI_SUCCESS
  EFI_UNSUPPORTED - if X64 processor does not support FXSTOR/FXRSTOR instructions,
                    the context save will fail, so these processor's are not supported.
  EFI_OUT_OF_RESOURCES - not resource to finish initialization

--*/
;

EFI_STATUS
EFIAPI
plUnloadDebugSupportDriver (
  IN EFI_HANDLE                       ImageHandle
  )
/*++

Routine Description:
  This is the callback that is written to the LoadedImage protocol instance
  on the image handle. It uninstalls all registered handlers and frees all entry
  stub memory.

  This code executes in boot services context.

Arguments:
  ImageHandle - The image handle of the unload handler

Returns:

  EFI_SUCCESS - always return success

--*/
;

//
// DebugSupport protocol member functions
//
EFI_STATUS
EFIAPI
GetMaximumProcessorIndex (
  IN EFI_DEBUG_SUPPORT_PROTOCOL       *This,
  OUT UINTN                           *MaxProcessorIndex
  )
/*++

Routine Description: This is a DebugSupport protocol member function.

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
  IN EFI_DEBUG_SUPPORT_PROTOCOL       *This,
  IN UINTN                            ProcessorIndex,
  IN EFI_PERIODIC_CALLBACK            PeriodicCallback
  )
/*++

Routine Description: This is a DebugSupport protocol member function.

Arguments:
  This             - The DebugSupport instance
  ProcessorIndex   - Which processor the callback applies to.
  PeriodicCallback - Callback function

Returns:

  EFI_SUCCESS
  EFI_INVALID_PARAMETER - requested uninstalling a handler from a vector that has
                          no handler registered for it
  EFI_ALREADY_STARTED   - requested install to a vector that already has a handler registered.

  Other possible return values are passed through from UnHookEntry and HookEntry.

--*/
;

EFI_STATUS
EFIAPI
RegisterExceptionCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL       *This,
  IN UINTN                            ProcessorIndex,
  IN EFI_EXCEPTION_CALLBACK           NewCallback,
  IN EFI_EXCEPTION_TYPE               ExceptionType
  )
/*++

Routine Description:
  This is a DebugSupport protocol member function.

  This code executes in boot services context.

Arguments:
  This             - The DebugSupport instance
  ProcessorIndex   - Which processor the callback applies to.
  NewCallback      - Callback function
  ExceptionType    - Which exception to hook

Returns:

  EFI_SUCCESS
  EFI_INVALID_PARAMETER - requested uninstalling a handler from a vector that has
                          no handler registered for it
  EFI_ALREADY_STARTED   - requested install to a vector that already has a handler registered.

  Other possible return values are passed through from UnHookEntry and HookEntry.

--*/
;

EFI_STATUS
EFIAPI
InvalidateInstructionCache (
  IN EFI_DEBUG_SUPPORT_PROTOCOL       *This,
  IN UINTN                            ProcessorIndex,
  IN VOID                             *Start,
  IN UINT64                           Length
  )
/*++

Routine Description:
  This is a DebugSupport protocol member function.
  Calls assembly routine to flush cache.

Arguments:
  This             - The DebugSupport instance
  ProcessorIndex   - Which processor the callback applies to.
  Start            - Physical base of the memory range to be invalidated
  Length           - mininum number of bytes in instruction cache to invalidate

Returns:

  EFI_SUCCESS - always return success

--*/
;

#endif
