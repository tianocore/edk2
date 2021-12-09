/** @file

  AMD SEV helper function.

  Copyright (c) 2021, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MpLib.h"
#include <Library/VmgExitLib.h>
#include <Register/Amd/Fam17Msr.h>
#include <Register/Amd/Ghcb.h>

/**
  Create an SEV-SNP AP save area (VMSA) for use in running the vCPU.

  @param[in]  CpuMpData        Pointer to CPU MP Data
  @param[in]  CpuData          Pointer to CPU AP Data
  @param[in]  ApicId           APIC ID of the vCPU
**/
VOID
SevSnpCreateSaveArea (
  IN CPU_MP_DATA  *CpuMpData,
  IN CPU_AP_DATA  *CpuData,
  UINT32          ApicId
  )
{
  SEV_ES_SAVE_AREA          *SaveArea;
  IA32_CR0                  ApCr0;
  IA32_CR0                  ResetCr0;
  IA32_CR4                  ApCr4;
  IA32_CR4                  ResetCr4;
  UINTN                     StartIp;
  UINT8                     SipiVector;
  UINT32                    RmpAdjustStatus;
  UINT64                    VmgExitStatus;
  MSR_SEV_ES_GHCB_REGISTER  Msr;
  GHCB                      *Ghcb;
  BOOLEAN                   InterruptState;
  UINT64                    ExitInfo1;
  UINT64                    ExitInfo2;

  //
  // Allocate a single page for the SEV-ES Save Area and initialize it.
  //
  SaveArea = AllocateReservedPages (1);
  if (!SaveArea) {
    return;
  }

  ZeroMem (SaveArea, EFI_PAGE_SIZE);

  //
  // Propogate the CR0.NW and CR0.CD setting to the AP
  //
  ResetCr0.UintN = 0x00000010;
  ApCr0.UintN    = CpuData->VolatileRegisters.Cr0;
  if (ApCr0.Bits.NW) {
    ResetCr0.Bits.NW = 1;
  }

  if (ApCr0.Bits.CD) {
    ResetCr0.Bits.CD = 1;
  }

  //
  // Propagate the CR4.MCE setting to the AP
  //
  ResetCr4.UintN = 0;
  ApCr4.UintN    = CpuData->VolatileRegisters.Cr4;
  if (ApCr4.Bits.MCE) {
    ResetCr4.Bits.MCE = 1;
  }

  //
  // Convert the start IP into a SIPI Vector
  //
  StartIp    = CpuMpData->MpCpuExchangeInfo->BufferStart;
  SipiVector = (UINT8)(StartIp >> 12);

  //
  // Set the CS:RIP value based on the start IP
  //
  SaveArea->Cs.Base                    = SipiVector << 12;
  SaveArea->Cs.Selector                = SipiVector << 8;
  SaveArea->Cs.Limit                   = 0xFFFF;
  SaveArea->Cs.Attributes.Bits.Present = 1;
  SaveArea->Cs.Attributes.Bits.Sbit    = 1;
  SaveArea->Cs.Attributes.Bits.Type    = SEV_ES_RESET_CODE_SEGMENT_TYPE;
  SaveArea->Rip                        = StartIp & 0xFFF;

  //
  // Set the remaining values as defined in APM for INIT
  //
  SaveArea->Ds.Limit                   = 0xFFFF;
  SaveArea->Ds.Attributes.Bits.Present = 1;
  SaveArea->Ds.Attributes.Bits.Sbit    = 1;
  SaveArea->Ds.Attributes.Bits.Type    = SEV_ES_RESET_DATA_SEGMENT_TYPE;
  SaveArea->Es                         = SaveArea->Ds;
  SaveArea->Fs                         = SaveArea->Ds;
  SaveArea->Gs                         = SaveArea->Ds;
  SaveArea->Ss                         = SaveArea->Ds;

  SaveArea->Gdtr.Limit                   = 0xFFFF;
  SaveArea->Ldtr.Limit                   = 0xFFFF;
  SaveArea->Ldtr.Attributes.Bits.Present = 1;
  SaveArea->Ldtr.Attributes.Bits.Type    = SEV_ES_RESET_LDT_TYPE;
  SaveArea->Idtr.Limit                   = 0xFFFF;
  SaveArea->Tr.Limit                     = 0xFFFF;
  SaveArea->Ldtr.Attributes.Bits.Present = 1;
  SaveArea->Ldtr.Attributes.Bits.Type    = SEV_ES_RESET_TSS_TYPE;

  SaveArea->Efer   = 0x1000;
  SaveArea->Cr4    = ResetCr4.UintN;
  SaveArea->Cr0    = ResetCr0.UintN;
  SaveArea->Dr7    = 0x0400;
  SaveArea->Dr6    = 0xFFFF0FF0;
  SaveArea->Rflags = 0x0002;
  SaveArea->GPat   = 0x0007040600070406ULL;
  SaveArea->XCr0   = 0x0001;
  SaveArea->Mxcsr  = 0x1F80;
  SaveArea->X87Ftw = 0x5555;
  SaveArea->X87Fcw = 0x0040;

  //
  // Set the SEV-SNP specific fields for the save area:
  //   VMPL - always VMPL0
  //   SEV_FEATURES - equivalent to the SEV_STATUS MSR right shifted 2 bits
  //
  SaveArea->Vmpl        = 0;
  SaveArea->SevFeatures = AsmReadMsr64 (MSR_SEV_STATUS) >> 2;

  //
  // To turn the page into a recognized VMSA page, issue RMPADJUST:
  //   Target VMPL but numerically higher than current VMPL
  //   Target PermissionMask is not used
  //
  RmpAdjustStatus = SevSnpRmpAdjust (
                      (EFI_PHYSICAL_ADDRESS)(UINTN)SaveArea,
                      TRUE
                      );
  ASSERT (RmpAdjustStatus == 0);

  ExitInfo1  = (UINT64)ApicId << 32;
  ExitInfo1 |= SVM_VMGEXIT_SNP_AP_CREATE;
  ExitInfo2  = (UINT64)(UINTN)SaveArea;

  Msr.GhcbPhysicalAddress = AsmReadMsr64 (MSR_SEV_ES_GHCB);
  Ghcb                    = Msr.Ghcb;

  VmgInit (Ghcb, &InterruptState);
  Ghcb->SaveArea.Rax = SaveArea->SevFeatures;
  VmgSetOffsetValid (Ghcb, GhcbRax);
  VmgExitStatus = VmgExit (
                    Ghcb,
                    SVM_EXIT_SNP_AP_CREATION,
                    ExitInfo1,
                    ExitInfo2
                    );
  VmgDone (Ghcb, InterruptState);

  ASSERT (VmgExitStatus == 0);
  if (VmgExitStatus != 0) {
    RmpAdjustStatus = SevSnpRmpAdjust (
                        (EFI_PHYSICAL_ADDRESS)(UINTN)SaveArea,
                        FALSE
                        );
    if (RmpAdjustStatus == 0) {
      FreePages (SaveArea, 1);
    } else {
      DEBUG ((DEBUG_INFO, "SEV-SNP: RMPADJUST failed, leaking VMSA page\n"));
    }

    SaveArea = NULL;
  }

  if (CpuData->SevEsSaveArea) {
    RmpAdjustStatus = SevSnpRmpAdjust (
                        (EFI_PHYSICAL_ADDRESS)(UINTN)CpuData->SevEsSaveArea,
                        FALSE
                        );
    if (RmpAdjustStatus == 0) {
      FreePages (CpuData->SevEsSaveArea, 1);
    } else {
      DEBUG ((DEBUG_INFO, "SEV-SNP: RMPADJUST failed, leaking VMSA page\n"));
    }
  }

  CpuData->SevEsSaveArea = SaveArea;
}

/**
  Create SEV-SNP APs.

  @param[in]  CpuMpData        Pointer to CPU MP Data
  @param[in]  ProcessorNumber  The handle number of specified processor
                               (-1 for all APs)
**/
VOID
SevSnpCreateAP (
  IN CPU_MP_DATA  *CpuMpData,
  IN INTN         ProcessorNumber
  )
{
  CPU_INFO_IN_HOB  *CpuInfoInHob;
  CPU_AP_DATA      *CpuData;
  UINTN            Index;
  UINT32           ApicId;

  ASSERT (CpuMpData->MpCpuExchangeInfo->BufferStart < 0x100000);

  CpuInfoInHob = (CPU_INFO_IN_HOB *)(UINTN)CpuMpData->CpuInfoInHob;

  if (ProcessorNumber < 0) {
    for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
      if (Index != CpuMpData->BspNumber) {
        CpuData = &CpuMpData->CpuData[Index];
        ApicId  = CpuInfoInHob[Index].ApicId,
        SevSnpCreateSaveArea (CpuMpData, CpuData, ApicId);
      }
    }
  } else {
    Index   = (UINTN)ProcessorNumber;
    CpuData = &CpuMpData->CpuData[Index];
    ApicId  = CpuInfoInHob[ProcessorNumber].ApicId,
    SevSnpCreateSaveArea (CpuMpData, CpuData, ApicId);
  }
}

/**
  Issue RMPADJUST to adjust the VMSA attribute of an SEV-SNP page.

  @param[in]  PageAddress
  @param[in]  VmsaPage

  @return  RMPADJUST return value
**/
UINT32
SevSnpRmpAdjust (
  IN  EFI_PHYSICAL_ADDRESS  PageAddress,
  IN  BOOLEAN               VmsaPage
  )
{
  UINT64  Rdx;

  //
  // The RMPADJUST instruction is used to set or clear the VMSA bit for a
  // page. The VMSA change is only made when running at VMPL0 and is ignored
  // otherwise. If too low a target VMPL is specified, the instruction can
  // succeed without changing the VMSA bit when not running at VMPL0. Using a
  // target VMPL level of 1, RMPADJUST will return a FAIL_PERMISSION error if
  // not running at VMPL0, thus ensuring that the VMSA bit is set appropriately
  // when no error is returned.
  //
  Rdx = 1;
  if (VmsaPage) {
    Rdx |= RMPADJUST_VMSA_PAGE_BIT;
  }

  return AsmRmpAdjust ((UINT64)PageAddress, 0, Rdx);
}
