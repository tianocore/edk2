/** @file
Page table manipulation functions for IA-32 processors

Copyright (c) 2009 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuDxeSmm.h"

/**
  Create PageTable for SMM use.

  @return     PageTable Address

**/
UINT32
SmmInitPageTable (
  VOID
  )
{
  UINTN                     PageFaultHandlerHookAddress;
  IA32_IDT_GATE_DESCRIPTOR  *IdtEntry;
  EFI_STATUS                Status;

  //
  // Initialize spin lock
  //
  InitializeSpinLock (mPFLock);

  mPhysicalAddressBits = 32;

  if (FeaturePcdGet (PcdCpuSmmProfileEnable) ||
      HEAP_GUARD_NONSTOP_MODE ||
      NULL_DETECTION_NONSTOP_MODE)
  {
    //
    // Set own Page Fault entry instead of the default one, because SMM Profile
    // feature depends on IRET instruction to do Single Step
    //
    PageFaultHandlerHookAddress = (UINTN)PageFaultIdtHandlerSmmProfile;
    IdtEntry                    = (IA32_IDT_GATE_DESCRIPTOR *)gcSmiIdtr.Base;
    IdtEntry                   += EXCEPT_IA32_PAGE_FAULT;
    IdtEntry->Bits.OffsetLow    = (UINT16)PageFaultHandlerHookAddress;
    IdtEntry->Bits.Reserved_0   = 0;
    IdtEntry->Bits.GateType     = IA32_IDT_GATE_TYPE_INTERRUPT_32;
    IdtEntry->Bits.OffsetHigh   = (UINT16)(PageFaultHandlerHookAddress >> 16);
  } else {
    //
    // Register SMM Page Fault Handler
    //
    Status = SmmRegisterExceptionHandler (&mSmmCpuService, EXCEPT_IA32_PAGE_FAULT, SmiPFHandler);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Additional SMM IDT initialization for SMM stack guard
  //
  if (FeaturePcdGet (PcdCpuSmmStackGuard)) {
    InitializeIDTSmmStackGuard ();
  }

  return Gen4GPageTable (TRUE);
}

/**
  Page Fault handler for SMM use.

**/
VOID
SmiDefaultPFHandler (
  VOID
  )
{
  CpuDeadLoop ();
}

/**
  ThePage Fault handler wrapper for SMM use.

  @param  InterruptType    Defines the type of interrupt or exception that
                           occurred on the processor.This parameter is processor architecture specific.
  @param  SystemContext    A pointer to the processor context when
                           the interrupt occurred on the processor.
**/
VOID
EFIAPI
SmiPFHandler (
  IN EFI_EXCEPTION_TYPE  InterruptType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  UINTN  PFAddress;
  UINTN  GuardPageAddress;
  UINTN  CpuIndex;

  ASSERT (InterruptType == EXCEPT_IA32_PAGE_FAULT);

  AcquireSpinLock (mPFLock);

  PFAddress = AsmReadCr2 ();

  //
  // If a page fault occurs in SMRAM range, it might be in a SMM stack guard page,
  // or SMM page protection violation.
  //
  if ((PFAddress >= mCpuHotPlugData.SmrrBase) &&
      (PFAddress < (mCpuHotPlugData.SmrrBase + mCpuHotPlugData.SmrrSize)))
  {
    DumpCpuContext (InterruptType, SystemContext);
    CpuIndex         = GetCpuIndex ();
    GuardPageAddress = (mSmmStackArrayBase + EFI_PAGE_SIZE + CpuIndex * mSmmStackSize);
    if ((FeaturePcdGet (PcdCpuSmmStackGuard)) &&
        (PFAddress >= GuardPageAddress) &&
        (PFAddress < (GuardPageAddress + EFI_PAGE_SIZE)))
    {
      DEBUG ((DEBUG_ERROR, "SMM stack overflow!\n"));
    } else {
      if ((SystemContext.SystemContextIa32->ExceptionData & IA32_PF_EC_ID) != 0) {
        DEBUG ((DEBUG_ERROR, "SMM exception at execution (0x%x)\n", PFAddress));
        DEBUG_CODE (
          DumpModuleInfoByIp (*(UINTN *)(UINTN)SystemContext.SystemContextIa32->Esp);
          );
      } else {
        DEBUG ((DEBUG_ERROR, "SMM exception at access (0x%x)\n", PFAddress));
        DEBUG_CODE (
          DumpModuleInfoByIp ((UINTN)SystemContext.SystemContextIa32->Eip);
          );
      }

      if (HEAP_GUARD_NONSTOP_MODE) {
        GuardPagePFHandler (SystemContext.SystemContextIa32->ExceptionData);
        goto Exit;
      }
    }

    CpuDeadLoop ();
    goto Exit;
  }

  //
  // If a page fault occurs in non-SMRAM range.
  //
  if ((PFAddress < mCpuHotPlugData.SmrrBase) ||
      (PFAddress >= mCpuHotPlugData.SmrrBase + mCpuHotPlugData.SmrrSize))
  {
    if ((SystemContext.SystemContextIa32->ExceptionData & IA32_PF_EC_ID) != 0) {
      DumpCpuContext (InterruptType, SystemContext);
      DEBUG ((DEBUG_ERROR, "Code executed on IP(0x%x) out of SMM range after SMM is locked!\n", PFAddress));
      DEBUG_CODE (
        DumpModuleInfoByIp (*(UINTN *)(UINTN)SystemContext.SystemContextIa32->Esp);
        );
      CpuDeadLoop ();
      goto Exit;
    }

    //
    // If NULL pointer was just accessed
    //
    if (((PcdGet8 (PcdNullPointerDetectionPropertyMask) & BIT1) != 0) &&
        (PFAddress < EFI_PAGE_SIZE))
    {
      DumpCpuContext (InterruptType, SystemContext);
      DEBUG ((DEBUG_ERROR, "!!! NULL pointer access !!!\n"));
      DEBUG_CODE (
        DumpModuleInfoByIp ((UINTN)SystemContext.SystemContextIa32->Eip);
        );

      if (NULL_DETECTION_NONSTOP_MODE) {
        GuardPagePFHandler (SystemContext.SystemContextIa32->ExceptionData);
        goto Exit;
      }

      CpuDeadLoop ();
      goto Exit;
    }

    if (IsSmmCommBufferForbiddenAddress (PFAddress)) {
      DumpCpuContext (InterruptType, SystemContext);
      DEBUG ((DEBUG_ERROR, "Access SMM communication forbidden address (0x%x)!\n", PFAddress));
      DEBUG_CODE (
        DumpModuleInfoByIp ((UINTN)SystemContext.SystemContextIa32->Eip);
        );
      CpuDeadLoop ();
      goto Exit;
    }
  }

  if (FeaturePcdGet (PcdCpuSmmProfileEnable)) {
    SmmProfilePFHandler (
      SystemContext.SystemContextIa32->Eip,
      SystemContext.SystemContextIa32->ExceptionData
      );
  } else {
    DumpCpuContext (InterruptType, SystemContext);
    SmiDefaultPFHandler ();
  }

Exit:
  ReleaseSpinLock (mPFLock);
}

/**
  This function returns with no action for 32 bit.

  @param[out]  *Cr2  Pointer to variable to hold CR2 register value.
**/
VOID
SaveCr2 (
  OUT UINTN  *Cr2
  )
{
  return;
}

/**
  This function returns with no action for 32 bit.

  @param[in]  Cr2  Value to write into CR2 register.
**/
VOID
RestoreCr2 (
  IN UINTN  Cr2
  )
{
  return;
}

/**
  Return whether access to non-SMRAM is restricted.

  @retval TRUE  Access to non-SMRAM is restricted.
  @retval FALSE Access to non-SMRAM is not restricted.
**/
BOOLEAN
IsRestrictedMemoryAccess (
  VOID
  )
{
  return TRUE;
}
