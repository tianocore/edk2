/** @file
  Call into 16-bit BIOS code

  BugBug: Thunker does A20 gate. Can we get rid of this code or
          put it into Legacy16 code.

Copyright (c) 1999 - 2014, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "LegacyBiosInterface.h"
#include "IpfThunk.h"

/**
  Gets the current flat GDT and IDT descriptors and  store them in
  Private->IntThunk.  These values are used by the Thunk code.
  This method must be called before every thunk in order to assure
  that the correct GDT and IDT are restored after the thunk.

  @param  Private            Private context for Legacy BIOS

  @retval EFI_SUCCESS        Should only pass.

**/
EFI_STATUS
LegacyBiosGetFlatDescs (
  IN  LEGACY_BIOS_INSTANCE    *Private
  )
{
  return EFI_SUCCESS;
}


/**
  BIOS interrupt call function.

  @param  BiosInt            Int number of BIOS call
  @param  Segment            Segment number
  @param  Offset             Offset in segment
  @param  Regs               IA32 Register set.
  @param  Stack              Base address of stack
  @param  StackSize          Size of stack

  @retval EFI_SUCCESS        BIOS interrupt call succeeds.

**/
EFI_STATUS
BiosIntCall (
  IN  UINT16                            BiosInt,
  IN  UINT16                            Segment,
  IN  UINT16                            Offset,
  IN  EFI_IA32_REGISTER_SET             *Regs,
  IN  VOID                              *Stack,
  IN  UINTN                             StackSize
  )
{
  IPF_DWORD_REGS  DwordRegs;
  UINT64          IntTypeVariable;

  IntTypeVariable = 0x8000000000000000;
  IntTypeVariable |= (UINT64)BiosInt;

  DwordRegs.Cs    = Segment;
  DwordRegs.Eip   = Offset;

  DwordRegs.Ds    = Regs->X.DS;
  DwordRegs.Es    = Regs->X.ES;
  DwordRegs.Fs    = Regs->X.ES;
  DwordRegs.Gs    = Regs->X.ES;
  DwordRegs.Ss    = 0xFFFF;

  DwordRegs.Eax   = Regs->X.AX;
  DwordRegs.Ebx   = Regs->X.BX;
  //
  // Sometimes, ECX is used to pass in 32 bit data. For example, INT 1Ah, AX = B10Dh is
  // "PCI BIOS v2.0c + Write Configuration DWORD" and ECX has the dword to write.
  //
  DwordRegs.Ecx   = Regs->E.ECX;
  DwordRegs.Edx   = Regs->X.DX;

  DwordRegs.Ebp   = Regs->X.BP;
  DwordRegs.Eflag = *((UINT16 *) &Regs->X.Flags);

  DwordRegs.Edi   = Regs->X.DI;
  DwordRegs.Esi   = Regs->X.SI;
  DwordRegs.Esp   = 0xFFFFFFFF;

  EfiIaEntryPoint (IntTypeVariable, &DwordRegs, ((UINTN) Stack + StackSize), StackSize);

  Regs->X.CS  = DwordRegs.Cs;

  Regs->X.DS  = (UINT16) DwordRegs.Ds;
  Regs->X.SS  = (UINT16) DwordRegs.Ss;

  Regs->E.EAX  = DwordRegs.Eax;
  Regs->E.EBX  = DwordRegs.Ebx;
  Regs->E.ECX  = DwordRegs.Ecx;
  Regs->E.EDX  = DwordRegs.Edx;

  Regs->E.EBP  = DwordRegs.Ebp;
  CopyMem (&Regs->X.Flags, &DwordRegs.Eflag, sizeof (EFI_FLAGS_REG));

  Regs->E.EDI  = DwordRegs.Edi;
  Regs->E.ESI  = DwordRegs.Esi;

  return EFI_SUCCESS;
}


/**
  Template of real mode code.

  @param  CodeStart          Start address of code.
  @param  CodeEnd            End address of code
  @param  ReverseThunkStart  Start of reverse thunk.
  @param  IntThunk           Low memory thunk.

**/
VOID
RealModeTemplate (
  OUT UINTN          *CodeStart,
  OUT UINTN          *CodeEnd,
  OUT UINTN          *ReverseThunkStart,
  LOW_MEMORY_THUNK   *IntThunk
  )
{
  SAL_RETURN_REGS SalStatus;

  SalStatus           = EsalGetReverseThunkAddress ();

  *CodeStart          = SalStatus.r9;
  *CodeEnd            = SalStatus.r10;
  *ReverseThunkStart  = SalStatus.r11;

}


/**
  Allocate memory < 1 MB and copy the thunker code into low memory. Se up
  all the descriptors.

  @param  Private            Private context for Legacy BIOS

  @retval EFI_SUCCESS        Should only pass.

**/
EFI_STATUS
LegacyBiosInitializeThunk (
  IN  LEGACY_BIOS_INSTANCE    *Private
  )
{
  GDT32               *CodeGdt;
  GDT32               *DataGdt;
  UINTN             CodeStart;
  UINTN             CodeEnd;
  UINTN             ReverseThunkStart;
  UINT32            Base;
  LOW_MEMORY_THUNK  *IntThunk;
  UINTN             TempData;

  ASSERT (Private);

  IntThunk = Private->IntThunk;

  //
  // Clear the reserved descriptor
  //
  ZeroMem (&(IntThunk->RealModeGdt[0]), sizeof (GDT32));

  //
  // Setup a descriptor for real-mode code
  //
  CodeGdt = &(IntThunk->RealModeGdt[1]);

  //
  // Fill in the descriptor with our real-mode segment value
  //
  CodeGdt->Type = 0xA;
  //
  // code/read
  //
  CodeGdt->System       = 1;
  CodeGdt->Dpl          = 0;
  CodeGdt->Present      = 1;
  CodeGdt->Software     = 0;
  CodeGdt->Reserved     = 0;
  CodeGdt->DefaultSize  = 0;
  //
  // 16 bit operands
  //
  CodeGdt->Granularity  = 0;

  CodeGdt->LimitHi      = 0;
  CodeGdt->LimitLo      = 0xffff;

  Base                  = (*((UINT32 *) &IntThunk->Code));
  CodeGdt->BaseHi       = (Base >> 24) & 0xFF;
  CodeGdt->BaseMid      = (Base >> 16) & 0xFF;
  CodeGdt->BaseLo       = Base & 0xFFFF;

  //
  // Setup a descriptor for read-mode data
  //
  DataGdt = &(IntThunk->RealModeGdt[2]);
  CopyMem (DataGdt, CodeGdt, sizeof (GDT32));

  DataGdt->Type = 0x2;
  //
  // read/write data
  //
  DataGdt->BaseHi = 0x0;
  //
  // Base = 0
  //
  DataGdt->BaseMid = 0x0;
  //
  DataGdt->BaseLo = 0x0;
  //
  DataGdt->LimitHi = 0x0F;
  //
  // Limit = 4Gb
  //
  DataGdt->LimitLo = 0xFFFF;
  //
  DataGdt->Granularity = 0x1;
  //
  //
  // Compute selector value
  //
  IntThunk->RealModeGdtDesc.Limit = (UINT16) (sizeof (IntThunk->RealModeGdt) - 1);
  CopyMem (&IntThunk->RealModeGdtDesc.Base, (UINT32 *) &IntThunk->RealModeGdt, sizeof (UINT32));
  //
  //  IntThunk->RealModeGdtDesc.Base = *((UINT32*) &IntThunk->RealModeGdt);
  //
  IntThunk->RealModeIdtDesc.Limit = 0xFFFF;
  IntThunk->RealModeIdtDesc.Base  = 0;
  IntThunk->LowCodeSelector       = (UINT32) ((UINTN) CodeGdt - IntThunk->RealModeGdtDesc.Base);
  IntThunk->LowDataSelector       = (UINT32) ((UINTN) DataGdt - IntThunk->RealModeGdtDesc.Base);

  //
  // Initialize low real-mode code thunk
  //
  RealModeTemplate (&CodeStart, &CodeEnd, &ReverseThunkStart, IntThunk);

  TempData                        = (UINTN) &(IntThunk->Code);
  IntThunk->LowReverseThunkStart  = ((UINT32) TempData + (UINT32) (ReverseThunkStart - CodeStart));

  EsalSetSalDataArea (TempData, (UINTN) IntThunk);
  CopyMem (IntThunk->Code, (VOID *) CodeStart, CodeEnd - CodeStart);

  IntThunk->EfiToLegacy16InitTable.ReverseThunkCallSegment = EFI_SEGMENT (*((UINT32 *) &IntThunk->LowReverseThunkStart));
  IntThunk->EfiToLegacy16InitTable.ReverseThunkCallOffset = EFI_OFFSET (*((UINT32 *) &IntThunk->LowReverseThunkStart));

  return EFI_SUCCESS;
}


/**
  Thunk to 16-bit real mode and execute a software interrupt with a vector
  of BiosInt. Regs will contain the 16-bit register context on entry and
  exit.

  @param  This               Protocol instance pointer.
  @param  BiosInt            Processor interrupt vector to invoke
  @param  Regs               Register contexted passed into (and returned) from
                             thunk to  16-bit mode

  @retval FALSE              Thunk completed, and there were no BIOS errors in the
                             target code. See Regs for status.
  @retval TRUE               There was a BIOS erro in the target code.

**/
BOOLEAN
EFIAPI
LegacyBiosInt86 (
  IN EFI_LEGACY_BIOS_PROTOCOL           *This,
  IN  UINT8                             BiosInt,
  IN  EFI_IA32_REGISTER_SET             *Regs
  )
{
  EFI_STATUS            Status;
  LEGACY_BIOS_INSTANCE  *Private;
  LOW_MEMORY_THUNK      *IntThunk;
  UINT16                *Stack16;
  EFI_TPL               OriginalTpl;
  UINTN                 IaSegment;
  UINTN                 IaOffset;
  UINTN                 *Address;
  UINTN                 TempData;

  Private   = LEGACY_BIOS_INSTANCE_FROM_THIS (This);
  IntThunk  = Private->IntThunk;

  //
  // Get the current flat GDT, IDT, and SS and store them in Private->IntThunk.
  //
  Status = LegacyBiosGetFlatDescs (Private);
  ASSERT_EFI_ERROR (Status);

  Regs->X.Flags.Reserved1 = 1;
  Regs->X.Flags.Reserved2 = 0;
  Regs->X.Flags.Reserved3 = 0;
  Regs->X.Flags.Reserved4 = 0;
  Regs->X.Flags.IOPL      = 3;
  Regs->X.Flags.NT        = 0;
  Regs->X.Flags.IF        = 1;
  Regs->X.Flags.TF        = 0;
  Regs->X.Flags.CF        = 0;
  //
  // Clear the error flag; thunk code may set it.
  //
  Stack16 = (UINT16 *) (IntThunk->Stack + LOW_STACK_SIZE);

  //
  // Copy regs to low memory stack
  //
  Stack16 -= sizeof (EFI_IA32_REGISTER_SET) / sizeof (UINT16);
  CopyMem (Stack16, Regs, sizeof (EFI_IA32_REGISTER_SET));

  //
  // Provide low stack esp
  //
  TempData            = ((UINTN) Stack16) - ((UINTN) IntThunk);
  IntThunk->LowStack  = *((UINT32 *) &TempData);

  //
  // Stack for reverse thunk flat mode.
  //    It must point to top of stack (end of stack space).
  //
  TempData                = ((UINTN) IntThunk->RevThunkStack) + LOW_STACK_SIZE;
  IntThunk->RevFlatStack  = *((UINT32 *) &TempData);

  //
  // The call to Legacy16 is a critical section to EFI
  //
  OriginalTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  //
  // Set Legacy16 state. 0x08, 0x70 is legacy 8259 vector bases.
  //
  Status = Private->Legacy8259->SetMode (Private->Legacy8259, Efi8259LegacyMode, NULL, NULL);
  ASSERT_EFI_ERROR (Status);

  //
  // Call the real mode thunk code
  //
  TempData  = BiosInt * 4;
  Address   = (UINTN *) TempData;
  IaOffset  = 0xFFFF & (*Address);
  IaSegment = 0xFFFF & ((*Address) >> 16);

  Status = BiosIntCall (
            BiosInt,
            (UINT16) IaSegment,
            (UINT16) IaOffset,
            (EFI_IA32_REGISTER_SET *) Stack16,
            IntThunk,
            IntThunk->LowStack
            );

  //
  // Check for errors with the thunk
  //
  switch (Status) {
  case THUNK_OK:
    break;

  case THUNK_ERR_A20_UNSUP:
  case THUNK_ERR_A20_FAILED:
  default:
    //
    // For all errors, set EFLAGS.CF (used by legacy BIOS to indicate error).
    //
    Regs->X.Flags.CF = 1;
    break;
  }

  Status  = Private->Legacy8259->SetMode (Private->Legacy8259, Efi8259ProtectedMode, NULL, NULL);
  ASSERT_EFI_ERROR (Status);

  //
  // End critical section
  //
  gBS->RestoreTPL (OriginalTpl);

  //
  // Return the resulting registers
  //
  CopyMem (Regs, Stack16, sizeof (EFI_IA32_REGISTER_SET));

  return (BOOLEAN) (Regs->X.Flags.CF != 0);
}


/**
  Thunk to 16-bit real mode and call Segment:Offset. Regs will contain the
  16-bit register context on entry and exit. Arguments can be passed on
  the Stack argument

  @param  This               Protocol instance pointer.
  @param  Segment            Segemnt of 16-bit mode call
  @param  Offset             Offset of 16-bit mdoe call
  @param  Regs               Register contexted passed into (and returned) from
                             thunk to  16-bit mode
  @param  Stack              Caller allocated stack used to pass arguments
  @param  StackSize          Size of Stack in bytes

  @retval FALSE              Thunk completed, and there were no BIOS errors in the
                             target code. See Regs for status.
  @retval TRUE               There was a BIOS erro in the target code.

**/
BOOLEAN
EFIAPI
LegacyBiosFarCall86 (
  IN EFI_LEGACY_BIOS_PROTOCOL           *This,
  IN  UINT16                            Segment,
  IN  UINT16                            Offset,
  IN  EFI_IA32_REGISTER_SET             *Regs,
  IN  VOID                              *Stack,
  IN  UINTN                             StackSize
  )
{
  EFI_STATUS            Status;
  LEGACY_BIOS_INSTANCE  *Private;
  LOW_MEMORY_THUNK      *IntThunk;
  UINT16                *Stack16;
  EFI_TPL               OriginalTpl;
  UINTN                 IaSegment;
  UINTN                 IaOffset;
  UINTN                 TempData;

  Private   = LEGACY_BIOS_INSTANCE_FROM_THIS (This);
  IntThunk  = Private->IntThunk;
  IaSegment = Segment;
  IaOffset  = Offset;

  //
  // Get the current flat GDT and IDT and store them in Private->IntThunk.
  //
  Status = LegacyBiosGetFlatDescs (Private);
  ASSERT_EFI_ERROR (Status);

  Regs->X.Flags.Reserved1 = 1;
  Regs->X.Flags.Reserved2 = 0;
  Regs->X.Flags.Reserved3 = 0;
  Regs->X.Flags.Reserved4 = 0;
  Regs->X.Flags.IOPL      = 3;
  Regs->X.Flags.NT        = 0;
  Regs->X.Flags.IF        = 1;
  Regs->X.Flags.TF        = 0;
  Regs->X.Flags.CF        = 0;
  //
  // Clear the error flag; thunk code may set it.
  //
  Stack16 = (UINT16 *) (IntThunk->Stack + LOW_STACK_SIZE);
  if (Stack != NULL && StackSize != 0) {
    //
    // Copy Stack to low memory stack
    //
    Stack16 -= StackSize / sizeof (UINT16);
    CopyMem (Stack16, Stack, StackSize);
  }
  //
  // Copy regs to low memory stack
  //
  Stack16 -= sizeof (EFI_IA32_REGISTER_SET) / sizeof (UINT16);
  CopyMem (Stack16, Regs, sizeof (EFI_IA32_REGISTER_SET));

  //
  // Provide low stack esp
  //
  TempData            = ((UINTN) Stack16) - ((UINTN) IntThunk);
  IntThunk->LowStack  = *((UINT32 *) &TempData);

  //
  // The call to Legacy16 is a critical section to EFI
  //
  OriginalTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  //
  // Set Legacy16 state. 0x08, 0x70 is legacy 8259 vector bases.
  //
  Status = Private->Legacy8259->SetMode (Private->Legacy8259, Efi8259LegacyMode, NULL, NULL);
  ASSERT_EFI_ERROR (Status);

  //
  // Call the real mode thunk code
  //
  Status = BiosIntCall (
            0x100,
            (UINT16) IaSegment,
            (UINT16) IaOffset,
            (EFI_IA32_REGISTER_SET *) Stack16,
            IntThunk,
            IntThunk->LowStack
            );

  //
  // Check for errors with the thunk
  //
  switch (Status) {
  case THUNK_OK:
    break;

  case THUNK_ERR_A20_UNSUP:
  case THUNK_ERR_A20_FAILED:
  default:
    //
    // For all errors, set EFLAGS.CF (used by legacy BIOS to indicate error).
    //
    Regs->X.Flags.CF = 1;
    break;
  }
  //
  // Restore protected mode interrupt state
  //
  Status = Private->Legacy8259->SetMode (Private->Legacy8259, Efi8259ProtectedMode, NULL, NULL);
  ASSERT_EFI_ERROR (Status);

  //
  // End critical section
  //
  gBS->RestoreTPL (OriginalTpl);

  //
  // Return the resulting registers
  //
  CopyMem (Regs, Stack16, sizeof (EFI_IA32_REGISTER_SET));
  Stack16 += sizeof (EFI_IA32_REGISTER_SET) / sizeof (UINT16);

  if (Stack != NULL && StackSize != 0) {
    //
    // Copy low memory stack to Stack
    //
    CopyMem (Stack, Stack16, StackSize);
    Stack16 += StackSize / sizeof (UINT16);
  }

  return (BOOLEAN) (Regs->X.Flags.CF != 0);
}
