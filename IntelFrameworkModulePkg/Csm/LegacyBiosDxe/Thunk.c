/** @file
  Call into 16-bit BIOS code, Use AsmThunk16 function of BaseLib.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "LegacyBiosInterface.h"

THUNK_CONTEXT      mThunkContext;

/**
  Thunk to 16-bit real mode and execute a software interrupt with a vector
  of BiosInt. Regs will contain the 16-bit register context on entry and
  exit.

  @param  This    Protocol instance pointer.
  @param  BiosInt Processor interrupt vector to invoke
  @param  Regs    Register contexted passed into (and returned) from thunk to
                  16-bit mode

  @retval FALSE   Thunk completed, and there were no BIOS errors in the target code.
                  See Regs for status.
  @retval TRUE    There was a BIOS erro in the target code.

**/
BOOLEAN
EFIAPI
LegacyBiosInt86 (
  IN  EFI_LEGACY_BIOS_PROTOCOL      *This,
  IN  UINT8                         BiosInt,
  IN  EFI_IA32_REGISTER_SET         *Regs
  )
{
  Regs->X.Flags.Reserved1 = 1;
  Regs->X.Flags.Reserved2 = 0;
  Regs->X.Flags.Reserved3 = 0;
  Regs->X.Flags.Reserved4 = 0;
  Regs->X.Flags.IOPL      = 3;
  Regs->X.Flags.NT        = 0;
  Regs->X.Flags.IF        = 0;
  Regs->X.Flags.TF        = 0;
  Regs->X.Flags.CF        = 0;

  return InternalLegacyBiosFarCall (
           This,
           (UINT16) (((UINT32 *)NULL)[BiosInt] >> 16),
           (UINT16) ((UINT32 *)NULL)[BiosInt],
           Regs,
           &Regs->X.Flags,
           sizeof (Regs->X.Flags)
           );
}

/**
  Thunk to 16-bit real mode and call Segment:Offset. Regs will contain the
  16-bit register context on entry and exit. Arguments can be passed on
  the Stack argument

  @param  This                   Protocol instance pointer.
  @param  Segment                Segemnt of 16-bit mode call
  @param  Offset                 Offset of 16-bit mdoe call
  @param  Regs                   Register contexted passed into (and returned) from
                                 thunk to  16-bit mode
  @param  Stack                  Caller allocated stack used to pass arguments
  @param  StackSize              Size of Stack in bytes

  @retval FALSE                  Thunk completed, and there were no BIOS errors in
                                 the target code. See Regs for status.
  @retval TRUE                   There was a BIOS erro in the target code.

**/
BOOLEAN
EFIAPI
LegacyBiosFarCall86 (
  IN  EFI_LEGACY_BIOS_PROTOCOL        *This,
  IN  UINT16                          Segment,
  IN  UINT16                          Offset,
  IN  EFI_IA32_REGISTER_SET           *Regs,
  IN  VOID                            *Stack,
  IN  UINTN                           StackSize
  )
{
  Regs->X.Flags.Reserved1 = 1;
  Regs->X.Flags.Reserved2 = 0;
  Regs->X.Flags.Reserved3 = 0;
  Regs->X.Flags.Reserved4 = 0;
  Regs->X.Flags.IOPL      = 3;
  Regs->X.Flags.NT        = 0;
  Regs->X.Flags.IF        = 1;
  Regs->X.Flags.TF        = 0;
  Regs->X.Flags.CF        = 0;

  return InternalLegacyBiosFarCall (This, Segment, Offset, Regs, Stack, StackSize);
}

/**
  Thunk to 16-bit real mode and call Segment:Offset. Regs will contain the
  16-bit register context on entry and exit. Arguments can be passed on
  the Stack argument

  @param  This       Protocol instance pointer.
  @param  Segment    Segemnt of 16-bit mode call
  @param  Offset     Offset of 16-bit mdoe call
  @param  Regs       Register contexted passed into (and returned) from thunk to
                     16-bit mode
  @param  Stack      Caller allocated stack used to pass arguments
  @param  StackSize  Size of Stack in bytes

  @retval FALSE      Thunk completed, and there were no BIOS errors in the target code.
                     See Regs for status.
  @retval TRUE       There was a BIOS erro in the target code.

**/
BOOLEAN
EFIAPI
InternalLegacyBiosFarCall (
  IN  EFI_LEGACY_BIOS_PROTOCOL        *This,
  IN  UINT16                          Segment,
  IN  UINT16                          Offset,
  IN  EFI_IA32_REGISTER_SET           *Regs,
  IN  VOID                            *Stack,
  IN  UINTN                           StackSize
  )
{
  UINTN                 Status;
  LEGACY_BIOS_INSTANCE  *Private;
  UINT16                *Stack16;
  EFI_TPL               OriginalTpl;
  IA32_REGISTER_SET     ThunkRegSet;
  BOOLEAN               InterruptState;

  Private = LEGACY_BIOS_INSTANCE_FROM_THIS (This);

  ZeroMem (&ThunkRegSet, sizeof (ThunkRegSet));
  ThunkRegSet.X.DI   = Regs->X.DI;
  ThunkRegSet.X.SI   = Regs->X.SI;
  ThunkRegSet.X.BP   = Regs->X.BP;
  ThunkRegSet.X.BX   = Regs->X.BX;
  ThunkRegSet.X.DX   = Regs->X.DX;
  //
  // Sometimes, ECX is used to pass in 32 bit data. For example, INT 1Ah, AX = B10Dh is
  // "PCI BIOS v2.0c + Write Configuration DWORD" and ECX has the dword to write.
  //
  ThunkRegSet.E.ECX   = Regs->E.ECX;
  ThunkRegSet.X.AX   = Regs->X.AX;
  ThunkRegSet.E.DS   = Regs->X.DS;
  ThunkRegSet.E.ES   = Regs->X.ES;

  CopyMem (&(ThunkRegSet.E.EFLAGS.UintN), &(Regs->X.Flags), sizeof (Regs->X.Flags));

  //
  // Clear the error flag; thunk code may set it. Stack16 should be the high address
  // Make Statk16 address the low 16 bit must be not zero.
  //
  Stack16 = (UINT16 *)((UINT8 *) mThunkContext.RealModeBuffer + mThunkContext.RealModeBufferSize - sizeof (UINT16));

  //
  // Save and disable interrutp of debug timer
  //
  InterruptState = SaveAndSetDebugTimerInterrupt (FALSE);

  //
  // The call to Legacy16 is a critical section to EFI
  //
  OriginalTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  if (Stack != NULL && StackSize != 0) {
    //
    // Copy Stack to low memory stack
    //
    Stack16 -= StackSize / sizeof (UINT16);
    CopyMem (Stack16, Stack, StackSize);
  }

  ThunkRegSet.E.SS   = (UINT16) (((UINTN) Stack16 >> 16) << 12);
  ThunkRegSet.E.ESP  = (UINT16) (UINTN) Stack16;
  ThunkRegSet.E.CS   = Segment;
  ThunkRegSet.E.Eip  = Offset;

  mThunkContext.RealModeState      = &ThunkRegSet;

  //
  // Set Legacy16 state. 0x08, 0x70 is legacy 8259 vector bases.
  //
  Status = Private->Legacy8259->SetMode (Private->Legacy8259, Efi8259LegacyMode, NULL, NULL);
  ASSERT_EFI_ERROR (Status);

  AsmThunk16 (&mThunkContext);

  //
  // OPROM may allocate EBDA range by itself and change EBDA base and EBDA size.
  // Get the current EBDA base address, and compared with pre-allocate minimum
  // EBDA base address, if the current EBDA base address is smaller, it indicates
  // PcdEbdaReservedMemorySize should be adjusted to larger for more OPROMs.
  //
  DEBUG_CODE (
    {
      UINTN                 EbdaBaseAddress;
      UINTN                 ReservedEbdaBaseAddress;

      EbdaBaseAddress = (*(UINT16 *) (UINTN) 0x40E) << 4;
      ReservedEbdaBaseAddress = CONVENTIONAL_MEMORY_TOP - PcdGet32 (PcdEbdaReservedMemorySize);
      ASSERT (ReservedEbdaBaseAddress <= EbdaBaseAddress);
    }
  );

  if (Stack != NULL && StackSize != 0) {
    //
    // Copy low memory stack to Stack
    //
    CopyMem (Stack, Stack16, StackSize);
  }

  //
  // Restore protected mode interrupt state
  //
  Status = Private->Legacy8259->SetMode (Private->Legacy8259, Efi8259ProtectedMode, NULL, NULL);
  ASSERT_EFI_ERROR (Status);

  mThunkContext.RealModeState = NULL;

  //
  // End critical section
  //
  gBS->RestoreTPL (OriginalTpl);

  //
  // Restore interrutp of debug timer
  //
  SaveAndSetDebugTimerInterrupt (InterruptState);

  Regs->E.EDI      = ThunkRegSet.E.EDI;
  Regs->E.ESI      = ThunkRegSet.E.ESI;
  Regs->E.EBP      = ThunkRegSet.E.EBP;
  Regs->E.EBX      = ThunkRegSet.E.EBX;
  Regs->E.EDX      = ThunkRegSet.E.EDX;
  Regs->E.ECX      = ThunkRegSet.E.ECX;
  Regs->E.EAX      = ThunkRegSet.E.EAX;
  Regs->X.SS       = ThunkRegSet.E.SS;
  Regs->X.CS       = ThunkRegSet.E.CS;
  Regs->X.DS       = ThunkRegSet.E.DS;
  Regs->X.ES       = ThunkRegSet.E.ES;

  CopyMem (&(Regs->X.Flags), &(ThunkRegSet.E.EFLAGS.UintN), sizeof (Regs->X.Flags));

  return (BOOLEAN) (Regs->X.Flags.CF == 1);
}

/**
  Allocate memory < 1 MB and copy the thunker code into low memory. Se up
  all the descriptors.

  @param  Private                Private context for Legacy BIOS

  @retval EFI_SUCCESS            Should only pass.

**/
EFI_STATUS
LegacyBiosInitializeThunk (
  IN  LEGACY_BIOS_INSTANCE    *Private
  )
{
  EFI_PHYSICAL_ADDRESS    MemoryAddress;

  MemoryAddress   = (EFI_PHYSICAL_ADDRESS) (UINTN) Private->IntThunk;

  mThunkContext.RealModeBuffer     = (VOID *) (UINTN) (MemoryAddress + ((sizeof (LOW_MEMORY_THUNK) / EFI_PAGE_SIZE) + 1) * EFI_PAGE_SIZE);
  mThunkContext.RealModeBufferSize = EFI_PAGE_SIZE;
  mThunkContext.ThunkAttributes    = THUNK_ATTRIBUTE_BIG_REAL_MODE | THUNK_ATTRIBUTE_DISABLE_A20_MASK_INT_15;

  AsmPrepareThunk16 (&mThunkContext);

  return EFI_SUCCESS;
}
