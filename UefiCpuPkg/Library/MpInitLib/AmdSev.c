/** @file
  CPU MP Initialize helper function for AMD SEV.

  Copyright (c) 2021, AMD Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MpLib.h"
#include <Library/CcExitLib.h>

/**
  Get Protected mode code segment with 16-bit default addressing
  from current GDT table.

  @return  Protected mode 16-bit code segment value.
**/
STATIC
UINT16
GetProtectedMode16CS (
  VOID
  )
{
  IA32_DESCRIPTOR          GdtrDesc;
  IA32_SEGMENT_DESCRIPTOR  *GdtEntry;
  UINTN                    GdtEntryCount;
  UINTN                    Index;
  UINT16                   CodeSegmentValue;
  EFI_STATUS               Status;

  Index = (UINT16)-1;
  AsmReadGdtr (&GdtrDesc);
  GdtEntryCount = (GdtrDesc.Limit + 1) / sizeof (IA32_SEGMENT_DESCRIPTOR);
  GdtEntry      = (IA32_SEGMENT_DESCRIPTOR *)GdtrDesc.Base;
  for (Index = 0; Index < GdtEntryCount; Index++) {
    if ((GdtEntry->Bits.L == 0) &&
        (GdtEntry->Bits.DB == 0) &&
        (GdtEntry->Bits.Type > 8))
    {
      break;
    }

    GdtEntry++;
  }

  Status = SafeUintnToUint16 (Index, &CodeSegmentValue);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return 0;
  }

  Status = SafeUint16Mult (CodeSegmentValue, 8, &CodeSegmentValue);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return 0;
  }

  return CodeSegmentValue;
}

/**
  Get Protected mode code segment with 32-bit default addressing
  from current GDT table.

  @return  Protected mode 32-bit code segment value.
**/
STATIC
UINT16
GetProtectedMode32CS (
  VOID
  )
{
  IA32_DESCRIPTOR          GdtrDesc;
  IA32_SEGMENT_DESCRIPTOR  *GdtEntry;
  UINTN                    GdtEntryCount;
  UINTN                    Index;
  UINT16                   CodeSegmentValue;
  EFI_STATUS               Status;

  Index = (UINT16)-1;
  AsmReadGdtr (&GdtrDesc);
  GdtEntryCount = (GdtrDesc.Limit + 1) / sizeof (IA32_SEGMENT_DESCRIPTOR);
  GdtEntry      = (IA32_SEGMENT_DESCRIPTOR *)GdtrDesc.Base;
  for (Index = 0; Index < GdtEntryCount; Index++) {
    if ((GdtEntry->Bits.L == 0) &&
        (GdtEntry->Bits.DB == 1) &&
        (GdtEntry->Bits.Type > 8))
    {
      break;
    }

    GdtEntry++;
  }

  ASSERT (Index != GdtEntryCount);
  Status = SafeUintnToUint16 (Index, &CodeSegmentValue);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return 0;
  }

  Status = SafeUint16Mult (CodeSegmentValue, 8, &CodeSegmentValue);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return 0;
  }

  return CodeSegmentValue;
}

/**
  Reset an AP when in SEV-ES mode.

  If successful, this function never returns.

  @param[in] Ghcb                 Pointer to the GHCB
  @param[in] CpuMpData            Pointer to CPU MP Data

**/
VOID
MpInitLibSevEsAPReset (
  IN GHCB         *Ghcb,
  IN CPU_MP_DATA  *CpuMpData
  )
{
  EFI_STATUS  Status;
  UINTN       ProcessorNumber;
  UINT16      Code16, Code32;
  AP_RESET    *APResetFn;
  UINTN       BufferStart;
  UINTN       StackStart;

  Status = GetProcessorNumber (CpuMpData, &ProcessorNumber);
  ASSERT_EFI_ERROR (Status);

  Code16 = GetProtectedMode16CS ();
  Code32 = GetProtectedMode32CS ();

  APResetFn = (AP_RESET *)(CpuMpData->WakeupBufferHigh + CpuMpData->AddressMap.SwitchToRealNoNxOffset);

  BufferStart = CpuMpData->MpCpuExchangeInfo->BufferStart;
  StackStart  = CpuMpData->SevEsAPResetStackStart -
                (AP_RESET_STACK_SIZE * ProcessorNumber);

  //
  // This call never returns.
  //
  APResetFn (BufferStart, Code16, Code32, StackStart);
}

/**
  Allocate the SEV-ES AP jump table buffer.

  @param[in, out]  CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
AllocateSevEsAPMemory (
  IN OUT CPU_MP_DATA  *CpuMpData
  )
{
  if (CpuMpData->SevEsAPBuffer == (UINTN)-1) {
    CpuMpData->SevEsAPBuffer =
      CpuMpData->SevEsIsEnabled ? GetSevEsAPMemory () : 0;
  }
}

/**
  Program the SEV-ES AP jump table buffer.

  @param[in]  SipiVector  The SIPI vector used for the AP Reset
**/
VOID
SetSevEsJumpTable (
  IN UINTN  SipiVector
  )
{
  SEV_ES_AP_JMP_FAR  *JmpFar;
  UINT32             Offset, InsnByte;
  UINT8              LoNib, HiNib;

  JmpFar = (SEV_ES_AP_JMP_FAR *)(UINTN)FixedPcdGet32 (PcdSevEsWorkAreaBase);
  ASSERT (JmpFar != NULL);

  //
  // Obtain the address of the Segment/Rip location in the workarea.
  // This will be set to a value derived from the SIPI vector and will
  // be the memory address used for the far jump below.
  //
  Offset  = FixedPcdGet32 (PcdSevEsWorkAreaBase);
  Offset += sizeof (JmpFar->InsnBuffer);
  LoNib   = (UINT8)Offset;
  HiNib   = (UINT8)(Offset >> 8);

  //
  // Program the workarea (which is the initial AP boot address) with
  // far jump to the SIPI vector (where XX and YY represent the
  // address of where the SIPI vector is stored.
  //
  //   JMP FAR [CS:XXYY] => 2E FF 2E YY XX
  //
  InsnByte                       = 0;
  JmpFar->InsnBuffer[InsnByte++] = 0x2E;  // CS override prefix
  JmpFar->InsnBuffer[InsnByte++] = 0xFF;  // JMP (FAR)
  JmpFar->InsnBuffer[InsnByte++] = 0x2E;  // ModRM (JMP memory location)
  JmpFar->InsnBuffer[InsnByte++] = LoNib; // YY offset ...
  JmpFar->InsnBuffer[InsnByte++] = HiNib; // XX offset ...

  //
  // Program the Segment/Rip based on the SIPI vector (always at least
  // 16-byte aligned, so Rip is set to 0).
  //
  JmpFar->Rip     = 0;
  JmpFar->Segment = (UINT16)(SipiVector >> 4);
}

/**
  The function puts the AP in halt loop.

  @param[in]  CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
SevEsPlaceApHlt (
  CPU_MP_DATA  *CpuMpData
  )
{
  MSR_SEV_ES_GHCB_REGISTER  Msr;
  GHCB                      *Ghcb;
  UINT64                    Status;
  BOOLEAN                   DoDecrement;
  BOOLEAN                   InterruptState;

  DoDecrement = (BOOLEAN)(CpuMpData->InitFlag == ApInitConfig);

  while (TRUE) {
    Msr.GhcbPhysicalAddress = AsmReadMsr64 (MSR_SEV_ES_GHCB);
    Ghcb                    = Msr.Ghcb;

    CcExitVmgInit (Ghcb, &InterruptState);

    if (DoDecrement) {
      DoDecrement = FALSE;

      //
      // Perform the delayed decrement just before issuing the first
      // VMGEXIT with AP_RESET_HOLD.
      //
      InterlockedDecrement ((UINT32 *)&CpuMpData->MpCpuExchangeInfo->NumApsExecuting);
    }

    Status = CcExitVmgExit (Ghcb, SVM_EXIT_AP_RESET_HOLD, 0, 0);
    if ((Status == 0) && (Ghcb->SaveArea.SwExitInfo2 != 0)) {
      CcExitVmgDone (Ghcb, InterruptState);
      break;
    }

    CcExitVmgDone (Ghcb, InterruptState);
  }

  //
  // Awakened in a new phase? Use the new CpuMpData
  //
  if (CpuMpData->NewCpuMpData != NULL) {
    CpuMpData = CpuMpData->NewCpuMpData;
  }

  MpInitLibSevEsAPReset (Ghcb, CpuMpData);
}

/**
  The function fills the exchange data for the AP.

  @param[in]   ExchangeInfo  The pointer to CPU Exchange Data structure
**/
VOID
FillExchangeInfoDataSevSnp (
  IN volatile MP_CPU_EXCHANGE_INFO  *ExchangeInfo
  )
{
  UINT32  StdRangeMax;

  AsmCpuid (CPUID_SIGNATURE, &StdRangeMax, NULL, NULL, NULL);
  if (StdRangeMax >= CPUID_EXTENDED_TOPOLOGY) {
    CPUID_EXTENDED_TOPOLOGY_EBX  ExtTopoEbx;

    AsmCpuidEx (
      CPUID_EXTENDED_TOPOLOGY,
      0,
      NULL,
      &ExtTopoEbx.Uint32,
      NULL,
      NULL
      );
    ExchangeInfo->ExtTopoAvail = !!ExtTopoEbx.Bits.LogicalProcessors;
  }
}

/**
  Get pointer to CPU MP Data structure from GUIDed HOB.

  @param[in] CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
AmdSevUpdateCpuMpData (
  IN CPU_MP_DATA  *CpuMpData
  )
{
  CPU_MP_DATA  *OldCpuMpData;

  OldCpuMpData = GetCpuMpDataFromGuidedHob ();

  OldCpuMpData->NewCpuMpData = CpuMpData;
}
