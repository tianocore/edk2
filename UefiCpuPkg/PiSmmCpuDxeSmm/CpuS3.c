/** @file
Code for Processor S3 restoration

Copyright (c) 2006 - 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"
#include <PiPei.h>

BOOLEAN  mRestoreSmmConfigurationInS3 = FALSE;

//
// S3 boot flag
//
BOOLEAN  mSmmS3Flag = FALSE;

//
// Pointer to structure used during S3 Resume
//
SMM_S3_RESUME_STATE  *mSmmS3ResumeState = NULL;

BOOLEAN  mAcpiS3Enable = TRUE;

/**
  Restore SMM Configuration in S3 boot path.

**/
VOID
RestoreSmmConfigurationInS3 (
  VOID
  )
{
  if (!mAcpiS3Enable) {
    return;
  }

  //
  // Restore SMM Configuration in S3 boot path.
  //
  if (mRestoreSmmConfigurationInS3) {
    //
    // Need make sure gMmst is correct because below function may use them.
    //
    gMmst->MmStartupThisAp       = gSmmCpuPrivate->SmmCoreEntryContext.SmmStartupThisAp;
    gMmst->CurrentlyExecutingCpu = gSmmCpuPrivate->SmmCoreEntryContext.CurrentlyExecutingCpu;
    gMmst->NumberOfCpus          = gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus;
    gMmst->CpuSaveStateSize      = gSmmCpuPrivate->SmmCoreEntryContext.CpuSaveStateSize;
    gMmst->CpuSaveState          = gSmmCpuPrivate->SmmCoreEntryContext.CpuSaveState;

    //
    // Configure SMM Code Access Check feature if available.
    //
    ConfigSmmCodeAccessCheck ();

    SmmCpuFeaturesCompleteSmmReadyToLock ();

    mRestoreSmmConfigurationInS3 = FALSE;
  }
}

/**
  Perform SMM initialization for all processors in the S3 boot path.

  For a native platform, MP initialization in the S3 boot path is also performed in this function.
**/
VOID
EFIAPI
SmmRestoreCpu (
  VOID
  )
{
  SMM_S3_RESUME_STATE       *SmmS3ResumeState;
  IA32_DESCRIPTOR           Ia32Idtr;
  IA32_DESCRIPTOR           X64Idtr;
  IA32_IDT_GATE_DESCRIPTOR  IdtEntryTable[EXCEPTION_VECTOR_NUMBER];
  EFI_STATUS                Status;

  DEBUG ((DEBUG_INFO, "SmmRestoreCpu()\n"));

  mSmmS3Flag = TRUE;

  //
  // See if there is enough context to resume PEI Phase
  //
  if (mSmmS3ResumeState == NULL) {
    DEBUG ((DEBUG_ERROR, "No context to return to PEI Phase\n"));
    CpuDeadLoop ();
  }

  SmmS3ResumeState = mSmmS3ResumeState;
  ASSERT (SmmS3ResumeState != NULL);

  //
  // Setup 64bit IDT in 64bit SMM env when called from 32bit PEI.
  // Note: 64bit PEI and 32bit DXE is not a supported combination.
  //
  if ((SmmS3ResumeState->Signature == SMM_S3_RESUME_SMM_64) && (FeaturePcdGet (PcdDxeIplSwitchToLongMode) == TRUE)) {
    //
    // Save the IA32 IDT Descriptor
    //
    AsmReadIdtr ((IA32_DESCRIPTOR *)&Ia32Idtr);

    //
    // Setup X64 IDT table
    //
    ZeroMem (IdtEntryTable, sizeof (IA32_IDT_GATE_DESCRIPTOR) * 32);
    X64Idtr.Base  = (UINTN)IdtEntryTable;
    X64Idtr.Limit = (UINT16)(sizeof (IA32_IDT_GATE_DESCRIPTOR) * 32 - 1);
    AsmWriteIdtr ((IA32_DESCRIPTOR *)&X64Idtr);

    //
    // Setup the default exception handler
    //
    Status = InitializeCpuExceptionHandlers (NULL);
    ASSERT_EFI_ERROR (Status);

    //
    // Initialize Debug Agent to support source level debug
    //
    if (mSmmDebugAgentSupport) {
      InitializeDebugAgent (DEBUG_AGENT_INIT_THUNK_PEI_IA32TOX64, (VOID *)&Ia32Idtr, NULL);
    }
  }

  //
  // Issue SMI IPI (All Excluding  Self SMM IPI + BSP SMM IPI) to execute first SMI init.
  //
  ExecuteFirstSmiInit ();

  //
  // Set a flag to restore SMM configuration in S3 path.
  //
  mRestoreSmmConfigurationInS3 = TRUE;

  DEBUG ((DEBUG_INFO, "SMM S3 Return CS                = %x\n", SmmS3ResumeState->ReturnCs));
  DEBUG ((DEBUG_INFO, "SMM S3 Return Entry Point       = %x\n", SmmS3ResumeState->ReturnEntryPoint));
  DEBUG ((DEBUG_INFO, "SMM S3 Return Context1          = %x\n", SmmS3ResumeState->ReturnContext1));
  DEBUG ((DEBUG_INFO, "SMM S3 Return Context2          = %x\n", SmmS3ResumeState->ReturnContext2));
  DEBUG ((DEBUG_INFO, "SMM S3 Return Stack Pointer     = %x\n", SmmS3ResumeState->ReturnStackPointer));

  //
  // If SMM is in 32-bit mode or PcdDxeIplSwitchToLongMode is FALSE, then use SwitchStack() to resume PEI Phase.
  // Note: 64bit PEI and 32bit DXE is not a supported combination.
  //
  if ((SmmS3ResumeState->Signature == SMM_S3_RESUME_SMM_32) || (FeaturePcdGet (PcdDxeIplSwitchToLongMode) == FALSE)) {
    DEBUG ((DEBUG_INFO, "Call SwitchStack() to return to S3 Resume in PEI Phase\n"));

    SwitchStack (
      (SWITCH_STACK_ENTRY_POINT)(UINTN)SmmS3ResumeState->ReturnEntryPoint,
      (VOID *)(UINTN)SmmS3ResumeState->ReturnContext1,
      (VOID *)(UINTN)SmmS3ResumeState->ReturnContext2,
      (VOID *)(UINTN)SmmS3ResumeState->ReturnStackPointer
      );
  }

  //
  // If SMM is in 64-bit mode, then use AsmDisablePaging64() to resume PEI Phase
  //
  if (SmmS3ResumeState->Signature == SMM_S3_RESUME_SMM_64) {
    DEBUG ((DEBUG_INFO, "Call AsmDisablePaging64() to return to S3 Resume in PEI Phase\n"));
    //
    // Disable interrupt of Debug timer, since new IDT table is for IA32 and will not work in long mode.
    //
    SaveAndSetDebugTimerInterrupt (FALSE);
    //
    // Restore IA32 IDT table
    //
    AsmWriteIdtr ((IA32_DESCRIPTOR *)&Ia32Idtr);
    AsmDisablePaging64 (
      SmmS3ResumeState->ReturnCs,
      (UINT32)SmmS3ResumeState->ReturnEntryPoint,
      (UINT32)SmmS3ResumeState->ReturnContext1,
      (UINT32)SmmS3ResumeState->ReturnContext2,
      (UINT32)SmmS3ResumeState->ReturnStackPointer
      );
  }

  //
  // Can not resume PEI Phase
  //
  DEBUG ((DEBUG_ERROR, "No context to return to PEI Phase\n"));
  CpuDeadLoop ();
}

/**
  Initialize SMM S3 resume state structure used during S3 Resume.

**/
VOID
InitSmmS3ResumeState (
  VOID
  )
{
  VOID                  *GuidHob;
  EFI_SMRAM_DESCRIPTOR  *SmramDescriptor;
  SMM_S3_RESUME_STATE   *SmmS3ResumeState;

  if (!mAcpiS3Enable) {
    return;
  }

  GuidHob = GetFirstGuidHob (&gEfiAcpiVariableGuid);
  if (GuidHob == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR:%a(): HOB(gEfiAcpiVariableGuid=%g) needed by S3 resume doesn't exist!\n",
      __func__,
      &gEfiAcpiVariableGuid
      ));
    CpuDeadLoop ();
  } else {
    SmramDescriptor = (EFI_SMRAM_DESCRIPTOR *)GET_GUID_HOB_DATA (GuidHob);

    DEBUG ((DEBUG_INFO, "SMM S3 SMRAM Structure = %x\n", SmramDescriptor));
    DEBUG ((DEBUG_INFO, "SMM S3 Structure = %x\n", SmramDescriptor->CpuStart));

    SmmS3ResumeState = (SMM_S3_RESUME_STATE *)(UINTN)SmramDescriptor->CpuStart;
    ZeroMem (SmmS3ResumeState, sizeof (SMM_S3_RESUME_STATE));

    mSmmS3ResumeState      = SmmS3ResumeState;
    SmmS3ResumeState->Smst = (EFI_PHYSICAL_ADDRESS)(UINTN)gMmst;

    SmmS3ResumeState->SmmS3ResumeEntryPoint = (EFI_PHYSICAL_ADDRESS)(UINTN)SmmRestoreCpu;

    SmmS3ResumeState->SmmS3StackSize = SIZE_32KB;
    SmmS3ResumeState->SmmS3StackBase = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocatePages (EFI_SIZE_TO_PAGES ((UINTN)SmmS3ResumeState->SmmS3StackSize));
    if (SmmS3ResumeState->SmmS3StackBase == 0) {
      SmmS3ResumeState->SmmS3StackSize = 0;
    }

    SmmS3ResumeState->SmmS3Cr0 = (UINT32)AsmReadCr0 ();
    SmmS3ResumeState->SmmS3Cr4 = (UINT32)AsmReadCr4 ();

    if (sizeof (UINTN) == sizeof (UINT64)) {
      SmmS3ResumeState->Signature = SMM_S3_RESUME_SMM_64;
    }

    if (sizeof (UINTN) == sizeof (UINT32)) {
      SmmS3ResumeState->Signature = SMM_S3_RESUME_SMM_32;
    }

    //
    // Patch SmmS3ResumeState->SmmS3Cr3
    // The SmmS3Cr3 is only used by S3Resume PEIM to switch CPU from 32bit to 64bit
    //
    InitSmmS3Cr3 ((UINTN *)&SmmS3ResumeState->SmmS3Cr3);
  }
}
