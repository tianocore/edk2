/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    plDebugSupport.h

Abstract:

    IA32 specific debug support macros, typedefs and prototypes.

Revision History

--*/

#ifndef _PLDEBUG_SUPPORT_H
#define _PLDEBUG_SUPPORT_H

#define NUM_IDT_ENTRIES                 0x78
#define SYSTEM_TIMER_VECTOR             0x68
#define VECTOR_ENTRY_PAGES              1
#define CopyDescriptor(Dest, Src)       CopyMem ((Dest), (Src), sizeof (DESCRIPTOR))
#define ZeroDescriptor(Dest)            CopyDescriptor ((Dest), &NullDesc)
#define ReadIdt(Vector, Dest)           CopyDescriptor ((Dest), &((GetIdtr ())[(Vector)]))
#define WriteIdt(Vector, Src)           CopyDescriptor (&((GetIdtr ())[(Vector)]), (Src))
#define CompareDescriptor(Desc1, Desc2) CompareMem ((Desc1), (Desc2), sizeof (DESCRIPTOR))
#define EFI_ISA                         IsaIa32
#define FF_FXSR                         (1 << 24)

typedef UINT64  DESCRIPTOR;

typedef struct {
  DESCRIPTOR  OrigDesc;
  VOID (*OrigVector) (VOID);
  DESCRIPTOR  NewDesc;
  VOID (*StubEntry) (VOID);
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

  TODO: Add function description

Arguments:

  None

Returns:

  TODO: add return values

--*/
;

BOOLEAN
FxStorSupport (
  VOID
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  None

Returns:

  TODO: add return values

--*/
;

DESCRIPTOR  *
GetIdtr (
  VOID
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  None

Returns:

  TODO: add return values

--*/
;

VOID
Vect2Desc (
  DESCRIPTOR * DestDesc,
  VOID (*Vector) (VOID)
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  DestDesc  - TODO: add argument description
  )         - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
WriteInterruptFlag (
  BOOLEAN NewState
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  NewState  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
plInitializeDebugSupportDriver (
  VOID
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  None

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
plUnloadDebugSupportDriver (
  IN EFI_HANDLE                       ImageHandle
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ImageHandle - TODO: add argument description

Returns:

  TODO: add return values

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

Routine Description:

  TODO: Add function description

Arguments:

  This              - TODO: add argument description
  MaxProcessorIndex - TODO: add argument description

Returns:

  TODO: add return values

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

Routine Description:

  TODO: Add function description

Arguments:

  This              - TODO: add argument description
  ProcessorIndex    - TODO: add argument description
  PeriodicCallback  - TODO: add argument description

Returns:

  TODO: add return values

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

  TODO: Add function description

Arguments:

  This            - TODO: add argument description
  ProcessorIndex  - TODO: add argument description
  NewCallback     - TODO: add argument description
  ExceptionType   - TODO: add argument description

Returns:

  TODO: add return values

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

  TODO: Add function description

Arguments:

  This            - TODO: add argument description
  ProcessorIndex  - TODO: add argument description
  Start           - TODO: add argument description
  Length          - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

#endif
