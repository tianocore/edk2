/** @file
Agent Module to load other modules to deploy SMM Entry Vector for X86 CPU.

Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PiSmmCpuDxeSmm.h"

//
// SMM CPU Private Data structure that contains SMM Configuration Protocol
// along its supporting fields.
//
SMM_CPU_PRIVATE_DATA  mSmmCpuPrivateData = {
  SMM_CPU_PRIVATE_DATA_SIGNATURE,               // Signature
  NULL,                                         // SmmCpuHandle
  NULL,                                         // Pointer to ProcessorInfo array
  NULL,                                         // Pointer to Operation array
  NULL,                                         // Pointer to CpuSaveStateSize array
  NULL,                                         // Pointer to CpuSaveState array
  { {0} },                                      // SmmReservedSmramRegion
  {
    SmmStartupThisAp,                           // SmmCoreEntryContext.SmmStartupThisAp
    0,                                          // SmmCoreEntryContext.CurrentlyExecutingCpu
    0,                                          // SmmCoreEntryContext.NumberOfCpus
    NULL,                                       // SmmCoreEntryContext.CpuSaveStateSize
    NULL                                        // SmmCoreEntryContext.CpuSaveState
  },
  NULL,                                         // SmmCoreEntry
  {
    mSmmCpuPrivateData.SmmReservedSmramRegion,  // SmmConfiguration.SmramReservedRegions
    RegisterSmmEntry                            // SmmConfiguration.RegisterSmmEntry
  },
};

CPU_HOT_PLUG_DATA mCpuHotPlugData = {
  CPU_HOT_PLUG_DATA_REVISION_1,                 // Revision
  0,                                            // Array Length of SmBase and APIC ID
  NULL,                                         // Pointer to APIC ID array
  NULL,                                         // Pointer to SMBASE array
  0,                                            // Reserved
  0,                                            // SmrrBase
  0                                             // SmrrSize
};

//
// Global pointer used to access mSmmCpuPrivateData from outside and inside SMM
//
SMM_CPU_PRIVATE_DATA  *gSmmCpuPrivate = &mSmmCpuPrivateData;

//
// SMM Relocation variables
//
volatile BOOLEAN  *mRebased;
volatile BOOLEAN  mIsBsp;

///
/// Handle for the SMM CPU Protocol
///
EFI_HANDLE  mSmmCpuHandle = NULL;

///
/// SMM CPU Protocol instance
///
EFI_SMM_CPU_PROTOCOL  mSmmCpu  = {
  SmmReadSaveState,
  SmmWriteSaveState
};

EFI_CPU_INTERRUPT_HANDLER   mExternalVectorTable[EXCEPTION_VECTOR_NUMBER];

//
// SMM stack information
//
UINTN mSmmStackArrayBase;
UINTN mSmmStackArrayEnd;
UINTN mSmmStackSize;

//
// Pointer to structure used during S3 Resume
//
SMM_S3_RESUME_STATE *mSmmS3ResumeState = NULL;

UINTN mMaxNumberOfCpus = 1;
UINTN mNumberOfCpus = 1;

//
// SMM ready to lock flag
//
BOOLEAN mSmmReadyToLock = FALSE;

//
// Global used to cache PCD for SMM Code Access Check enable
//
BOOLEAN                  mSmmCodeAccessCheckEnable = FALSE;

//
// Spin lock used to serialize setting of SMM Code Access Check feature
//
SPIN_LOCK                mConfigSmmCodeAccessCheckLock;

/**
  Initialize IDT to setup exception handlers for SMM.

**/
VOID
InitializeSmmIdt (
  VOID
  )
{
  EFI_STATUS               Status;
  BOOLEAN                  InterruptState;
  IA32_DESCRIPTOR          DxeIdtr;
  //
  // Disable Interrupt and save DXE IDT table
  //
  InterruptState = SaveAndDisableInterrupts ();
  AsmReadIdtr (&DxeIdtr);
  //
  // Load SMM temporary IDT table
  //
  AsmWriteIdtr (&gcSmiIdtr);
  //
  // Setup SMM default exception handlers, SMM IDT table
  // will be updated and saved in gcSmiIdtr
  //
  Status = InitializeCpuExceptionHandlers (NULL);
  ASSERT_EFI_ERROR (Status);
  //
  // Restore DXE IDT table and CPU interrupt
  //
  AsmWriteIdtr ((IA32_DESCRIPTOR *) &DxeIdtr);
  SetInterruptState (InterruptState);
}

/**
  Search module name by input IP address and output it.

  @param CallerIpAddress   Caller instruction pointer.

**/
VOID
DumpModuleInfoByIp (
  IN  UINTN              CallerIpAddress
  )
{
  UINTN                                Pe32Data;
  EFI_IMAGE_DOS_HEADER                 *DosHdr;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  Hdr;
  VOID                                 *PdbPointer;
  UINT64                               DumpIpAddress;

  //
  // Find Image Base
  //
  Pe32Data = CallerIpAddress & ~(SIZE_4KB - 1);
  while (Pe32Data != 0) {
    DosHdr = (EFI_IMAGE_DOS_HEADER *) Pe32Data;
    if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
      //
      // DOS image header is present, so read the PE header after the DOS image header.
      //
      Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)(Pe32Data + (UINTN) ((DosHdr->e_lfanew) & 0x0ffff));
      //
      // Make sure PE header address does not overflow and is less than the initial address.
      //
      if (((UINTN)Hdr.Pe32 > Pe32Data) && ((UINTN)Hdr.Pe32 < CallerIpAddress)) {
        if (Hdr.Pe32->Signature == EFI_IMAGE_NT_SIGNATURE) {
          //
          // It's PE image.
          //
          break;
        }
      }
    }

    //
    // Not found the image base, check the previous aligned address
    //
    Pe32Data -= SIZE_4KB;
  }

  DumpIpAddress = CallerIpAddress;
  DEBUG ((EFI_D_ERROR, "It is invoked from the instruction before IP(0x%lx)", DumpIpAddress));

  if (Pe32Data != 0) {
    PdbPointer = PeCoffLoaderGetPdbPointer ((VOID *) Pe32Data);
    if (PdbPointer != NULL) {
      DEBUG ((EFI_D_ERROR, " in module (%a)", PdbPointer));
    }
  }
}

/**
  Read information from the CPU save state.

  @param  This      EFI_SMM_CPU_PROTOCOL instance
  @param  Width     The number of bytes to read from the CPU save state.
  @param  Register  Specifies the CPU register to read form the save state.
  @param  CpuIndex  Specifies the zero-based index of the CPU save state.
  @param  Buffer    Upon return, this holds the CPU register value read from the save state.

  @retval EFI_SUCCESS   The register was read from Save State
  @retval EFI_NOT_FOUND The register is not defined for the Save State of Processor
  @retval EFI_INVALID_PARAMTER   This or Buffer is NULL.

**/
EFI_STATUS
EFIAPI
SmmReadSaveState (
  IN CONST EFI_SMM_CPU_PROTOCOL         *This,
  IN UINTN                              Width,
  IN EFI_SMM_SAVE_STATE_REGISTER        Register,
  IN UINTN                              CpuIndex,
  OUT VOID                              *Buffer
  )
{
  EFI_STATUS  Status;

  //
  // Retrieve pointer to the specified CPU's SMM Save State buffer
  //
  if ((CpuIndex >= gSmst->NumberOfCpus) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check for special EFI_SMM_SAVE_STATE_REGISTER_PROCESSOR_ID
  //
  if (Register == EFI_SMM_SAVE_STATE_REGISTER_PROCESSOR_ID) {
    //
    // The pseudo-register only supports the 64-bit size specified by Width.
    //
    if (Width != sizeof (UINT64)) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // If the processor is in SMM at the time the SMI occurred,
    // the pseudo register value for EFI_SMM_SAVE_STATE_REGISTER_PROCESSOR_ID is returned in Buffer.
    // Otherwise, EFI_NOT_FOUND is returned.
    //
    if (mSmmMpSyncData->CpuData[CpuIndex].Present) {
      *(UINT64 *)Buffer = gSmmCpuPrivate->ProcessorInfo[CpuIndex].ProcessorId;
      return EFI_SUCCESS;
    } else {
      return EFI_NOT_FOUND;
    }
  }

  if (!mSmmMpSyncData->CpuData[CpuIndex].Present) {
    return EFI_INVALID_PARAMETER;
  }

  Status = SmmCpuFeaturesReadSaveStateRegister (CpuIndex, Register, Width, Buffer);
  if (Status == EFI_UNSUPPORTED) {
    Status = ReadSaveStateRegister (CpuIndex, Register, Width, Buffer);
  }
  return Status;
}

/**
  Write data to the CPU save state.

  @param  This      EFI_SMM_CPU_PROTOCOL instance
  @param  Width     The number of bytes to read from the CPU save state.
  @param  Register  Specifies the CPU register to write to the save state.
  @param  CpuIndex  Specifies the zero-based index of the CPU save state
  @param  Buffer    Upon entry, this holds the new CPU register value.

  @retval EFI_SUCCESS   The register was written from Save State
  @retval EFI_NOT_FOUND The register is not defined for the Save State of Processor
  @retval EFI_INVALID_PARAMTER   ProcessorIndex or Width is not correct

**/
EFI_STATUS
EFIAPI
SmmWriteSaveState (
  IN CONST EFI_SMM_CPU_PROTOCOL         *This,
  IN UINTN                              Width,
  IN EFI_SMM_SAVE_STATE_REGISTER        Register,
  IN UINTN                              CpuIndex,
  IN CONST VOID                         *Buffer
  )
{
  EFI_STATUS  Status;

  //
  // Retrieve pointer to the specified CPU's SMM Save State buffer
  //
  if ((CpuIndex >= gSmst->NumberOfCpus) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Writes to EFI_SMM_SAVE_STATE_REGISTER_PROCESSOR_ID are ignored
  //
  if (Register == EFI_SMM_SAVE_STATE_REGISTER_PROCESSOR_ID) {
    return EFI_SUCCESS;
  }

  if (!mSmmMpSyncData->CpuData[CpuIndex].Present) {
    return EFI_INVALID_PARAMETER;
  }

  Status = SmmCpuFeaturesWriteSaveStateRegister (CpuIndex, Register, Width, Buffer);
  if (Status == EFI_UNSUPPORTED) {
    Status = WriteSaveStateRegister (CpuIndex, Register, Width, Buffer);
  }
  return Status;
}


/**
  C function for SMI handler. To change all processor's SMMBase Register.

**/
VOID
EFIAPI
SmmInitHandler (
  VOID
  )
{
  UINT32                            ApicId;
  UINTN                             Index;

  //
  // Update SMM IDT entries' code segment and load IDT
  //
  AsmWriteIdtr (&gcSmiIdtr);
  ApicId = GetApicId ();

  ASSERT (mNumberOfCpus <= PcdGet32 (PcdCpuMaxLogicalProcessorNumber));

  for (Index = 0; Index < mNumberOfCpus; Index++) {
    if (ApicId == (UINT32)gSmmCpuPrivate->ProcessorInfo[Index].ProcessorId) {
      //
      // Initialize SMM specific features on the currently executing CPU
      //
      SmmCpuFeaturesInitializeProcessor (
        Index,
        mIsBsp,
        gSmmCpuPrivate->ProcessorInfo,
        &mCpuHotPlugData
        );

      if (mIsBsp) {
        //
        // BSP rebase is already done above.
        // Initialize private data during S3 resume
        //
        InitializeMpSyncData ();
      }

      //
      // Hook return after RSM to set SMM re-based flag
      //
      SemaphoreHook (Index, &mRebased[Index]);

      return;
    }
  }
  ASSERT (FALSE);
}

/**
  Relocate SmmBases for each processor.

  Execute on first boot and all S3 resumes

**/
VOID
EFIAPI
SmmRelocateBases (
  VOID
  )
{
  UINT8                 BakBuf[BACK_BUF_SIZE];
  SMRAM_SAVE_STATE_MAP  BakBuf2;
  SMRAM_SAVE_STATE_MAP  *CpuStatePtr;
  UINT8                 *U8Ptr;
  UINT32                ApicId;
  UINTN                 Index;
  UINTN                 BspIndex;

  //
  // Make sure the reserved size is large enough for procedure SmmInitTemplate.
  //
  ASSERT (sizeof (BakBuf) >= gcSmmInitSize);

  //
  // Patch ASM code template with current CR0, CR3, and CR4 values
  //
  gSmmCr0 = (UINT32)AsmReadCr0 ();
  gSmmCr3 = (UINT32)AsmReadCr3 ();
  gSmmCr4 = (UINT32)AsmReadCr4 ();

  //
  // Patch GDTR for SMM base relocation
  //
  gcSmiInitGdtr.Base  = gcSmiGdtr.Base;
  gcSmiInitGdtr.Limit = gcSmiGdtr.Limit;

  U8Ptr = (UINT8*)(UINTN)(SMM_DEFAULT_SMBASE + SMM_HANDLER_OFFSET);
  CpuStatePtr = (SMRAM_SAVE_STATE_MAP *)(UINTN)(SMM_DEFAULT_SMBASE + SMRAM_SAVE_STATE_MAP_OFFSET);

  //
  // Backup original contents at address 0x38000
  //
  CopyMem (BakBuf, U8Ptr, sizeof (BakBuf));
  CopyMem (&BakBuf2, CpuStatePtr, sizeof (BakBuf2));

  //
  // Load image for relocation
  //
  CopyMem (U8Ptr, gcSmmInitTemplate, gcSmmInitSize);

  //
  // Retrieve the local APIC ID of current processor
  //
  ApicId = GetApicId ();

  //
  // Relocate SM bases for all APs
  // This is APs' 1st SMI - rebase will be done here, and APs' default SMI handler will be overridden by gcSmmInitTemplate
  //
  mIsBsp   = FALSE;
  BspIndex = (UINTN)-1;
  for (Index = 0; Index < mNumberOfCpus; Index++) {
    mRebased[Index] = FALSE;
    if (ApicId != (UINT32)gSmmCpuPrivate->ProcessorInfo[Index].ProcessorId) {
      SendSmiIpi ((UINT32)gSmmCpuPrivate->ProcessorInfo[Index].ProcessorId);
      //
      // Wait for this AP to finish its 1st SMI
      //
      while (!mRebased[Index]);
    } else {
      //
      // BSP will be Relocated later
      //
      BspIndex = Index;
    }
  }

  //
  // Relocate BSP's SMM base
  //
  ASSERT (BspIndex != (UINTN)-1);
  mIsBsp = TRUE;
  SendSmiIpi (ApicId);
  //
  // Wait for the BSP to finish its 1st SMI
  //
  while (!mRebased[BspIndex]);

  //
  // Restore contents at address 0x38000
  //
  CopyMem (CpuStatePtr, &BakBuf2, sizeof (BakBuf2));
  CopyMem (U8Ptr, BakBuf, sizeof (BakBuf));
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
  SMM_S3_RESUME_STATE           *SmmS3ResumeState;
  IA32_DESCRIPTOR               Ia32Idtr;
  IA32_DESCRIPTOR               X64Idtr;
  IA32_IDT_GATE_DESCRIPTOR      IdtEntryTable[EXCEPTION_VECTOR_NUMBER];
  EFI_STATUS                    Status;

  DEBUG ((EFI_D_INFO, "SmmRestoreCpu()\n"));

  //
  // See if there is enough context to resume PEI Phase
  //
  if (mSmmS3ResumeState == NULL) {
    DEBUG ((EFI_D_ERROR, "No context to return to PEI Phase\n"));
    CpuDeadLoop ();
  }

  SmmS3ResumeState = mSmmS3ResumeState;
  ASSERT (SmmS3ResumeState != NULL);

  if (SmmS3ResumeState->Signature == SMM_S3_RESUME_SMM_64) {
    //
    // Save the IA32 IDT Descriptor
    //
    AsmReadIdtr ((IA32_DESCRIPTOR *) &Ia32Idtr);

    //
    // Setup X64 IDT table
    //
    ZeroMem (IdtEntryTable, sizeof (IA32_IDT_GATE_DESCRIPTOR) * 32);
    X64Idtr.Base = (UINTN) IdtEntryTable;
    X64Idtr.Limit = (UINT16) (sizeof (IA32_IDT_GATE_DESCRIPTOR) * 32 - 1);
    AsmWriteIdtr ((IA32_DESCRIPTOR *) &X64Idtr);

    //
    // Setup the default exception handler
    //
    Status = InitializeCpuExceptionHandlers (NULL);
    ASSERT_EFI_ERROR (Status);

    //
    // Initialize Debug Agent to support source level debug
    //
    InitializeDebugAgent (DEBUG_AGENT_INIT_THUNK_PEI_IA32TOX64, (VOID *)&Ia32Idtr, NULL);
  }

  //
  // Skip initialization if mAcpiCpuData is not valid
  //
  if (mAcpiCpuData.NumberOfCpus > 0) {
    //
    // First time microcode load and restore MTRRs
    //
    EarlyInitializeCpu ();
  }

  //
  // Restore SMBASE for BSP and all APs
  //
  SmmRelocateBases ();

  //
  // Skip initialization if mAcpiCpuData is not valid
  //
  if (mAcpiCpuData.NumberOfCpus > 0) {
    //
    // Restore MSRs for BSP and all APs
    //
    InitializeCpu ();
  }

  //
  // Set a flag to restore SMM configuration in S3 path.
  //
  mRestoreSmmConfigurationInS3 = TRUE;

  DEBUG (( EFI_D_INFO, "SMM S3 Return CS                = %x\n", SmmS3ResumeState->ReturnCs));
  DEBUG (( EFI_D_INFO, "SMM S3 Return Entry Point       = %x\n", SmmS3ResumeState->ReturnEntryPoint));
  DEBUG (( EFI_D_INFO, "SMM S3 Return Context1          = %x\n", SmmS3ResumeState->ReturnContext1));
  DEBUG (( EFI_D_INFO, "SMM S3 Return Context2          = %x\n", SmmS3ResumeState->ReturnContext2));
  DEBUG (( EFI_D_INFO, "SMM S3 Return Stack Pointer     = %x\n", SmmS3ResumeState->ReturnStackPointer));

  //
  // If SMM is in 32-bit mode, then use SwitchStack() to resume PEI Phase
  //
  if (SmmS3ResumeState->Signature == SMM_S3_RESUME_SMM_32) {
    DEBUG ((EFI_D_INFO, "Call SwitchStack() to return to S3 Resume in PEI Phase\n"));

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
    DEBUG ((EFI_D_INFO, "Call AsmDisablePaging64() to return to S3 Resume in PEI Phase\n"));
    //
    // Disable interrupt of Debug timer, since new IDT table is for IA32 and will not work in long mode.
    //
    SaveAndSetDebugTimerInterrupt (FALSE);
    //
    // Restore IA32 IDT table
    //
    AsmWriteIdtr ((IA32_DESCRIPTOR *) &Ia32Idtr);
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
  DEBUG ((EFI_D_ERROR, "No context to return to PEI Phase\n"));
  CpuDeadLoop ();
}

/**
  Copy register table from ACPI NVS memory into SMRAM.

  @param[in] DestinationRegisterTableList  Points to destination register table.
  @param[in] SourceRegisterTableList       Points to source register table.
  @param[in] NumberOfCpus                  Number of CPUs.

**/
VOID
CopyRegisterTable (
  IN CPU_REGISTER_TABLE         *DestinationRegisterTableList,
  IN CPU_REGISTER_TABLE         *SourceRegisterTableList,
  IN UINT32                     NumberOfCpus
  )
{
  UINTN                      Index;
  UINTN                      Index1;
  CPU_REGISTER_TABLE_ENTRY   *RegisterTableEntry;

  CopyMem (DestinationRegisterTableList, SourceRegisterTableList, NumberOfCpus * sizeof (CPU_REGISTER_TABLE));
  for (Index = 0; Index < NumberOfCpus; Index++) {
    DestinationRegisterTableList[Index].RegisterTableEntry = AllocatePool (DestinationRegisterTableList[Index].AllocatedSize);
    ASSERT (DestinationRegisterTableList[Index].RegisterTableEntry != NULL);
    CopyMem (DestinationRegisterTableList[Index].RegisterTableEntry, SourceRegisterTableList[Index].RegisterTableEntry, DestinationRegisterTableList[Index].AllocatedSize);
    //
    // Go though all MSRs in register table to initialize MSR spin lock
    //
    RegisterTableEntry = DestinationRegisterTableList[Index].RegisterTableEntry;
    for (Index1 = 0; Index1 < DestinationRegisterTableList[Index].TableLength; Index1++, RegisterTableEntry++) {
      if ((RegisterTableEntry->RegisterType == Msr) && (RegisterTableEntry->ValidBitLength < 64)) {
        //
        // Initialize MSR spin lock only for those MSRs need bit field writing
        //
        InitMsrSpinLockByIndex (RegisterTableEntry->Index);
      }
    }
  }
}

/**
  SMM Ready To Lock event notification handler.

  The CPU S3 data is copied to SMRAM for security and mSmmReadyToLock is set to
  perform additional lock actions that must be performed from SMM on the next SMI.

  @param[in] Protocol   Points to the protocol's unique identifier.
  @param[in] Interface  Points to the interface instance.
  @param[in] Handle     The handle on which the interface was installed.

  @retval EFI_SUCCESS   Notification handler runs successfully.
 **/
EFI_STATUS
EFIAPI
SmmReadyToLockEventNotify (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  ACPI_CPU_DATA              *AcpiCpuData;
  IA32_DESCRIPTOR            *Gdtr;
  IA32_DESCRIPTOR            *Idtr;

  //
  // Prevent use of mAcpiCpuData by initialize NumberOfCpus to 0
  //
  mAcpiCpuData.NumberOfCpus = 0;

  //
  // If PcdCpuS3DataAddress was never set, then do not copy CPU S3 Data into SMRAM
  //
  AcpiCpuData = (ACPI_CPU_DATA *)(UINTN)PcdGet64 (PcdCpuS3DataAddress);
  if (AcpiCpuData == 0) {
    goto Done;
  }

  //
  // For a native platform, copy the CPU S3 data into SMRAM for use on CPU S3 Resume.
  //
  CopyMem (&mAcpiCpuData, AcpiCpuData, sizeof (mAcpiCpuData));

  mAcpiCpuData.MtrrTable = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocatePool (sizeof (MTRR_SETTINGS));
  ASSERT (mAcpiCpuData.MtrrTable != 0);

  CopyMem ((VOID *)(UINTN)mAcpiCpuData.MtrrTable, (VOID *)(UINTN)AcpiCpuData->MtrrTable, sizeof (MTRR_SETTINGS));

  mAcpiCpuData.GdtrProfile = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocatePool (sizeof (IA32_DESCRIPTOR));
  ASSERT (mAcpiCpuData.GdtrProfile != 0);

  CopyMem ((VOID *)(UINTN)mAcpiCpuData.GdtrProfile, (VOID *)(UINTN)AcpiCpuData->GdtrProfile, sizeof (IA32_DESCRIPTOR));

  mAcpiCpuData.IdtrProfile = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocatePool (sizeof (IA32_DESCRIPTOR));
  ASSERT (mAcpiCpuData.IdtrProfile != 0);

  CopyMem ((VOID *)(UINTN)mAcpiCpuData.IdtrProfile, (VOID *)(UINTN)AcpiCpuData->IdtrProfile, sizeof (IA32_DESCRIPTOR));

  mAcpiCpuData.PreSmmInitRegisterTable = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocatePool (mAcpiCpuData.NumberOfCpus * sizeof (CPU_REGISTER_TABLE));
  ASSERT (mAcpiCpuData.PreSmmInitRegisterTable != 0);

  CopyRegisterTable (
    (CPU_REGISTER_TABLE *)(UINTN)mAcpiCpuData.PreSmmInitRegisterTable,
    (CPU_REGISTER_TABLE *)(UINTN)AcpiCpuData->PreSmmInitRegisterTable,
    mAcpiCpuData.NumberOfCpus
    );

  mAcpiCpuData.RegisterTable = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocatePool (mAcpiCpuData.NumberOfCpus * sizeof (CPU_REGISTER_TABLE));
  ASSERT (mAcpiCpuData.RegisterTable != 0);

  CopyRegisterTable (
    (CPU_REGISTER_TABLE *)(UINTN)mAcpiCpuData.RegisterTable,
    (CPU_REGISTER_TABLE *)(UINTN)AcpiCpuData->RegisterTable,
    mAcpiCpuData.NumberOfCpus
    );

  //
  // Copy AP's GDT, IDT and Machine Check handler into SMRAM.
  //
  Gdtr = (IA32_DESCRIPTOR *)(UINTN)mAcpiCpuData.GdtrProfile;
  Idtr = (IA32_DESCRIPTOR *)(UINTN)mAcpiCpuData.IdtrProfile;

  mGdtForAp = AllocatePool ((Gdtr->Limit + 1) + (Idtr->Limit + 1) +  mAcpiCpuData.ApMachineCheckHandlerSize);
  ASSERT (mGdtForAp != NULL);
  mIdtForAp = (VOID *) ((UINTN)mGdtForAp + (Gdtr->Limit + 1));
  mMachineCheckHandlerForAp = (VOID *) ((UINTN)mIdtForAp + (Idtr->Limit + 1));

  CopyMem (mGdtForAp, (VOID *)Gdtr->Base, Gdtr->Limit + 1);
  CopyMem (mIdtForAp, (VOID *)Idtr->Base, Idtr->Limit + 1);
  CopyMem (mMachineCheckHandlerForAp, (VOID *)(UINTN)mAcpiCpuData.ApMachineCheckHandlerBase, mAcpiCpuData.ApMachineCheckHandlerSize);

Done:
  //
  // Set SMM ready to lock flag and return
  //
  mSmmReadyToLock = TRUE;
  return EFI_SUCCESS;
}

/**
  The module Entry Point of the CPU SMM driver.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
PiCpuSmmEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                 Status;
  EFI_MP_SERVICES_PROTOCOL   *MpServices;
  UINTN                      NumberOfEnabledProcessors;
  UINTN                      Index;
  VOID                       *Buffer;
  UINTN                      BufferPages;
  UINTN                      TileCodeSize;
  UINTN                      TileDataSize;
  UINTN                      TileSize;
  VOID                       *GuidHob;
  EFI_SMRAM_DESCRIPTOR       *SmramDescriptor;
  SMM_S3_RESUME_STATE        *SmmS3ResumeState;
  UINT8                      *Stacks;
  VOID                       *Registration;
  UINT32                     RegEax;
  UINT32                     RegEdx;
  UINTN                      FamilyId;
  UINTN                      ModelId;
  UINT32                     Cr3;

  //
  // Initialize Debug Agent to support source level debug in SMM code
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_SMM, NULL, NULL);

  //
  // Report the start of CPU SMM initialization.
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_PC_SMM_INIT
    );

  //
  // Fix segment address of the long-mode-switch jump
  //
  if (sizeof (UINTN) == sizeof (UINT64)) {
    gSmmJmpAddr.Segment = LONG_MODE_CODE_SEGMENT;
  }

  //
  // Find out SMRR Base and SMRR Size
  //
  FindSmramInfo (&mCpuHotPlugData.SmrrBase, &mCpuHotPlugData.SmrrSize);

  //
  // Get MP Services Protocol
  //
  Status = SystemTable->BootServices->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&MpServices);
  ASSERT_EFI_ERROR (Status);

  //
  // Use MP Services Protocol to retrieve the number of processors and number of enabled processors
  //
  Status = MpServices->GetNumberOfProcessors (MpServices, &mNumberOfCpus, &NumberOfEnabledProcessors);
  ASSERT_EFI_ERROR (Status);
  ASSERT (mNumberOfCpus <= PcdGet32 (PcdCpuMaxLogicalProcessorNumber));

  //
  // If support CPU hot plug, PcdCpuSmmEnableBspElection should be set to TRUE.
  // A constant BSP index makes no sense because it may be hot removed.
  //
  DEBUG_CODE (
    if (FeaturePcdGet (PcdCpuHotPlugSupport)) {

      ASSERT (FeaturePcdGet (PcdCpuSmmEnableBspElection));
    }
  );

  //
  // Save the PcdCpuSmmCodeAccessCheckEnable value into a global variable.
  //
  mSmmCodeAccessCheckEnable = PcdGetBool (PcdCpuSmmCodeAccessCheckEnable);
  DEBUG ((EFI_D_INFO, "PcdCpuSmmCodeAccessCheckEnable = %d\n", mSmmCodeAccessCheckEnable));

  //
  // If support CPU hot plug, we need to allocate resources for possibly hot-added processors
  //
  if (FeaturePcdGet (PcdCpuHotPlugSupport)) {
    mMaxNumberOfCpus = PcdGet32 (PcdCpuMaxLogicalProcessorNumber);
  } else {
    mMaxNumberOfCpus = mNumberOfCpus;
  }
  gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus = mMaxNumberOfCpus;

  //
  // The CPU save state and code for the SMI entry point are tiled within an SMRAM
  // allocated buffer.  The minimum size of this buffer for a uniprocessor system
  // is 32 KB, because the entry point is SMBASE + 32KB, and CPU save state area
  // just below SMBASE + 64KB.  If more than one CPU is present in the platform,
  // then the SMI entry point and the CPU save state areas can be tiles to minimize
  // the total amount SMRAM required for all the CPUs.  The tile size can be computed
  // by adding the   // CPU save state size, any extra CPU specific context, and
  // the size of code that must be placed at the SMI entry point to transfer
  // control to a C function in the native SMM execution mode.  This size is
  // rounded up to the nearest power of 2 to give the tile size for a each CPU.
  // The total amount of memory required is the maximum number of CPUs that
  // platform supports times the tile size.  The picture below shows the tiling,
  // where m is the number of tiles that fit in 32KB.
  //
  //  +-----------------------------+  <-- 2^n offset from Base of allocated buffer
  //  |   CPU m+1 Save State        |
  //  +-----------------------------+
  //  |   CPU m+1 Extra Data        |
  //  +-----------------------------+
  //  |   Padding                   |
  //  +-----------------------------+
  //  |   CPU 2m  SMI Entry         |
  //  +#############################+  <-- Base of allocated buffer + 64 KB
  //  |   CPU m-1 Save State        |
  //  +-----------------------------+
  //  |   CPU m-1 Extra Data        |
  //  +-----------------------------+
  //  |   Padding                   |
  //  +-----------------------------+
  //  |   CPU 2m-1 SMI Entry        |
  //  +=============================+  <-- 2^n offset from Base of allocated buffer
  //  |   . . . . . . . . . . . .   |
  //  +=============================+  <-- 2^n offset from Base of allocated buffer
  //  |   CPU 2 Save State          |
  //  +-----------------------------+
  //  |   CPU 2 Extra Data          |
  //  +-----------------------------+
  //  |   Padding                   |
  //  +-----------------------------+
  //  |   CPU m+1 SMI Entry         |
  //  +=============================+  <-- Base of allocated buffer + 32 KB
  //  |   CPU 1 Save State          |
  //  +-----------------------------+
  //  |   CPU 1 Extra Data          |
  //  +-----------------------------+
  //  |   Padding                   |
  //  +-----------------------------+
  //  |   CPU m SMI Entry           |
  //  +#############################+  <-- Base of allocated buffer + 32 KB == CPU 0 SMBASE + 64 KB
  //  |   CPU 0 Save State          |
  //  +-----------------------------+
  //  |   CPU 0 Extra Data          |
  //  +-----------------------------+
  //  |   Padding                   |
  //  +-----------------------------+
  //  |   CPU m-1 SMI Entry         |
  //  +=============================+  <-- 2^n offset from Base of allocated buffer
  //  |   . . . . . . . . . . . .   |
  //  +=============================+  <-- 2^n offset from Base of allocated buffer
  //  |   Padding                   |
  //  +-----------------------------+
  //  |   CPU 1 SMI Entry           |
  //  +=============================+  <-- 2^n offset from Base of allocated buffer
  //  |   Padding                   |
  //  +-----------------------------+
  //  |   CPU 0 SMI Entry           |
  //  +#############################+  <-- Base of allocated buffer == CPU 0 SMBASE + 32 KB
  //

  //
  // Retrieve CPU Family
  //
  AsmCpuid (CPUID_VERSION_INFO, &RegEax, NULL, NULL, NULL);
  FamilyId = (RegEax >> 8) & 0xf;
  ModelId = (RegEax >> 4) & 0xf;
  if (FamilyId == 0x06 || FamilyId == 0x0f) {
    ModelId = ModelId | ((RegEax >> 12) & 0xf0);
  }

  RegEdx = 0;
  AsmCpuid (CPUID_EXTENDED_FUNCTION, &RegEax, NULL, NULL, NULL);
  if (RegEax >= CPUID_EXTENDED_CPU_SIG) {
    AsmCpuid (CPUID_EXTENDED_CPU_SIG, NULL, NULL, NULL, &RegEdx);
  }
  //
  // Determine the mode of the CPU at the time an SMI occurs
  //   Intel(R) 64 and IA-32 Architectures Software Developer's Manual
  //   Volume 3C, Section 34.4.1.1
  //
  mSmmSaveStateRegisterLma = EFI_SMM_SAVE_STATE_REGISTER_LMA_32BIT;
  if ((RegEdx & BIT29) != 0) {
    mSmmSaveStateRegisterLma = EFI_SMM_SAVE_STATE_REGISTER_LMA_64BIT;
  }
  if (FamilyId == 0x06) {
    if (ModelId == 0x17 || ModelId == 0x0f || ModelId == 0x1c) {
      mSmmSaveStateRegisterLma = EFI_SMM_SAVE_STATE_REGISTER_LMA_64BIT;
    }
  }

  //
  // Compute tile size of buffer required to hold the CPU SMRAM Save State Map, extra CPU
  // specific context in a PROCESSOR_SMM_DESCRIPTOR, and the SMI entry point.  This size
  // is rounded up to nearest power of 2.
  //
  TileCodeSize = GetSmiHandlerSize ();
  TileCodeSize = ALIGN_VALUE(TileCodeSize, SIZE_4KB);
  TileDataSize = sizeof (SMRAM_SAVE_STATE_MAP) + sizeof (PROCESSOR_SMM_DESCRIPTOR);
  TileDataSize = ALIGN_VALUE(TileDataSize, SIZE_4KB);
  TileSize = TileDataSize + TileCodeSize - 1;
  TileSize = 2 * GetPowerOfTwo32 ((UINT32)TileSize);
  DEBUG ((EFI_D_INFO, "SMRAM TileSize = 0x%08x (0x%08x, 0x%08x)\n", TileSize, TileCodeSize, TileDataSize));

  //
  // If the TileSize is larger than space available for the SMI Handler of CPU[i],
  // the PROCESSOR_SMM_DESCRIPTOR of CPU[i+1] and the SMRAM Save State Map of CPU[i+1],
  // the ASSERT().  If this ASSERT() is triggered, then the SMI Handler size must be
  // reduced.
  //
  ASSERT (TileSize <= (SMRAM_SAVE_STATE_MAP_OFFSET + sizeof (SMRAM_SAVE_STATE_MAP) - SMM_HANDLER_OFFSET));

  //
  // Allocate buffer for all of the tiles.
  //
  // Intel(R) 64 and IA-32 Architectures Software Developer's Manual
  // Volume 3C, Section 34.11 SMBASE Relocation
  //   For Pentium and Intel486 processors, the SMBASE values must be
  //   aligned on a 32-KByte boundary or the processor will enter shutdown
  //   state during the execution of a RSM instruction.
  //
  // Intel486 processors: FamilyId is 4
  // Pentium processors : FamilyId is 5
  //
  BufferPages = EFI_SIZE_TO_PAGES (SIZE_32KB + TileSize * (mMaxNumberOfCpus - 1));
  if ((FamilyId == 4) || (FamilyId == 5)) {
    Buffer = AllocateAlignedPages (BufferPages, SIZE_32KB);
  } else {
    Buffer = AllocateAlignedPages (BufferPages, SIZE_4KB);
  }
  ASSERT (Buffer != NULL);
  DEBUG ((EFI_D_INFO, "SMRAM SaveState Buffer (0x%08x, 0x%08x)\n", Buffer, EFI_PAGES_TO_SIZE(BufferPages)));

  //
  // Allocate buffer for pointers to array in  SMM_CPU_PRIVATE_DATA.
  //
  gSmmCpuPrivate->ProcessorInfo = (EFI_PROCESSOR_INFORMATION *)AllocatePool (sizeof (EFI_PROCESSOR_INFORMATION) * mMaxNumberOfCpus);
  ASSERT (gSmmCpuPrivate->ProcessorInfo != NULL);

  gSmmCpuPrivate->Operation = (SMM_CPU_OPERATION *)AllocatePool (sizeof (SMM_CPU_OPERATION) * mMaxNumberOfCpus);
  ASSERT (gSmmCpuPrivate->Operation != NULL);

  gSmmCpuPrivate->CpuSaveStateSize = (UINTN *)AllocatePool (sizeof (UINTN) * mMaxNumberOfCpus);
  ASSERT (gSmmCpuPrivate->CpuSaveStateSize != NULL);

  gSmmCpuPrivate->CpuSaveState = (VOID **)AllocatePool (sizeof (VOID *) * mMaxNumberOfCpus);
  ASSERT (gSmmCpuPrivate->CpuSaveState != NULL);

  mSmmCpuPrivateData.SmmCoreEntryContext.CpuSaveStateSize = gSmmCpuPrivate->CpuSaveStateSize;
  mSmmCpuPrivateData.SmmCoreEntryContext.CpuSaveState     = gSmmCpuPrivate->CpuSaveState;

  //
  // Allocate buffer for pointers to array in CPU_HOT_PLUG_DATA.
  //
  mCpuHotPlugData.ApicId = (UINT64 *)AllocatePool (sizeof (UINT64) * mMaxNumberOfCpus);
  ASSERT (mCpuHotPlugData.ApicId != NULL);
  mCpuHotPlugData.SmBase = (UINTN *)AllocatePool (sizeof (UINTN) * mMaxNumberOfCpus);
  ASSERT (mCpuHotPlugData.SmBase != NULL);
  mCpuHotPlugData.ArrayLength = (UINT32)mMaxNumberOfCpus;

  //
  // Retrieve APIC ID of each enabled processor from the MP Services protocol.
  // Also compute the SMBASE address, CPU Save State address, and CPU Save state
  // size for each CPU in the platform
  //
  for (Index = 0; Index < mMaxNumberOfCpus; Index++) {
    mCpuHotPlugData.SmBase[Index]          = (UINTN)Buffer + Index * TileSize - SMM_HANDLER_OFFSET;
    gSmmCpuPrivate->CpuSaveStateSize[Index] = sizeof(SMRAM_SAVE_STATE_MAP);
    gSmmCpuPrivate->CpuSaveState[Index]     = (VOID *)(mCpuHotPlugData.SmBase[Index] + SMRAM_SAVE_STATE_MAP_OFFSET);
    gSmmCpuPrivate->Operation[Index] = SmmCpuNone;

    if (Index < mNumberOfCpus) {
      Status = MpServices->GetProcessorInfo (MpServices, Index, &gSmmCpuPrivate->ProcessorInfo[Index]);
      ASSERT_EFI_ERROR (Status);
      mCpuHotPlugData.ApicId[Index] = gSmmCpuPrivate->ProcessorInfo[Index].ProcessorId;

      DEBUG ((EFI_D_INFO, "CPU[%03x]  APIC ID=%04x  SMBASE=%08x  SaveState=%08x  Size=%08x\n",
        Index,
        (UINT32)gSmmCpuPrivate->ProcessorInfo[Index].ProcessorId,
        mCpuHotPlugData.SmBase[Index],
        gSmmCpuPrivate->CpuSaveState[Index],
        gSmmCpuPrivate->CpuSaveStateSize[Index]
        ));
    } else {
      gSmmCpuPrivate->ProcessorInfo[Index].ProcessorId = INVALID_APIC_ID;
      mCpuHotPlugData.ApicId[Index] = INVALID_APIC_ID;
    }
  }

  //
  // Allocate SMI stacks for all processors.
  //
  if (FeaturePcdGet (PcdCpuSmmStackGuard)) {
    //
    // 2 more pages is allocated for each processor.
    // one is guard page and the other is known good stack.
    //
    // +-------------------------------------------+-----+-------------------------------------------+
    // | Known Good Stack | Guard Page | SMM Stack | ... | Known Good Stack | Guard Page | SMM Stack |
    // +-------------------------------------------+-----+-------------------------------------------+
    // |                                           |     |                                           |
    // |<-------------- Processor 0 -------------->|     |<-------------- Processor n -------------->|
    //
    mSmmStackSize = EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (PcdGet32 (PcdCpuSmmStackSize)) + 2);
    Stacks = (UINT8 *) AllocatePages (gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus * (EFI_SIZE_TO_PAGES (PcdGet32 (PcdCpuSmmStackSize)) + 2));
    ASSERT (Stacks != NULL);
    mSmmStackArrayBase = (UINTN)Stacks;
    mSmmStackArrayEnd = mSmmStackArrayBase + gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus * mSmmStackSize - 1;
  } else {
    mSmmStackSize = PcdGet32 (PcdCpuSmmStackSize);
    Stacks = (UINT8 *) AllocatePages (EFI_SIZE_TO_PAGES (gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus * mSmmStackSize));
    ASSERT (Stacks != NULL);
  }

  //
  // Set SMI stack for SMM base relocation
  //
  gSmmInitStack = (UINTN) (Stacks + mSmmStackSize - sizeof (UINTN));

  //
  // Initialize IDT
  //
  InitializeSmmIdt ();

  //
  // Relocate SMM Base addresses to the ones allocated from SMRAM
  //
  mRebased = (BOOLEAN *)AllocateZeroPool (sizeof (BOOLEAN) * mMaxNumberOfCpus);
  ASSERT (mRebased != NULL);
  SmmRelocateBases ();

  //
  // Call hook for BSP to perform extra actions in normal mode after all
  // SMM base addresses have been relocated on all CPUs
  //
  SmmCpuFeaturesSmmRelocationComplete ();

  //
  // SMM Time initialization
  //
  InitializeSmmTimer ();

  //
  // Initialize MP globals
  //
  Cr3 = InitializeMpServiceData (Stacks, mSmmStackSize);

  //
  // Fill in SMM Reserved Regions
  //
  gSmmCpuPrivate->SmmReservedSmramRegion[0].SmramReservedStart = 0;
  gSmmCpuPrivate->SmmReservedSmramRegion[0].SmramReservedSize  = 0;

  //
  // Install the SMM Configuration Protocol onto a new handle on the handle database.
  // The entire SMM Configuration Protocol is allocated from SMRAM, so only a pointer
  // to an SMRAM address will be present in the handle database
  //
  Status = SystemTable->BootServices->InstallMultipleProtocolInterfaces (
                                        &gSmmCpuPrivate->SmmCpuHandle,
                                        &gEfiSmmConfigurationProtocolGuid, &gSmmCpuPrivate->SmmConfiguration,
                                        NULL
                                        );
  ASSERT_EFI_ERROR (Status);

  //
  // Install the SMM CPU Protocol into SMM protocol database
  //
  Status = gSmst->SmmInstallProtocolInterface (
                    &mSmmCpuHandle,
                    &gEfiSmmCpuProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mSmmCpu
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Expose address of CPU Hot Plug Data structure if CPU hot plug is supported.
  //
  if (FeaturePcdGet (PcdCpuHotPlugSupport)) {
    Status = PcdSet64S (PcdCpuHotPlugDataAddress, (UINT64)(UINTN)&mCpuHotPlugData);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Initialize SMM CPU Services Support
  //
  Status = InitializeSmmCpuServices (mSmmCpuHandle);
  ASSERT_EFI_ERROR (Status);

  //
  // register SMM Ready To Lock Protocol notification
  //
  Status = gSmst->SmmRegisterProtocolNotify (
                    &gEfiSmmReadyToLockProtocolGuid,
                    SmmReadyToLockEventNotify,
                    &Registration
                    );
  ASSERT_EFI_ERROR (Status);

  GuidHob = GetFirstGuidHob (&gEfiAcpiVariableGuid);
  if (GuidHob != NULL) {
    SmramDescriptor = (EFI_SMRAM_DESCRIPTOR *) GET_GUID_HOB_DATA (GuidHob);

    DEBUG ((EFI_D_INFO, "SMM S3 SMRAM Structure = %x\n", SmramDescriptor));
    DEBUG ((EFI_D_INFO, "SMM S3 Structure = %x\n", SmramDescriptor->CpuStart));

    SmmS3ResumeState = (SMM_S3_RESUME_STATE *)(UINTN)SmramDescriptor->CpuStart;
    ZeroMem (SmmS3ResumeState, sizeof (SMM_S3_RESUME_STATE));

    mSmmS3ResumeState = SmmS3ResumeState;
    SmmS3ResumeState->Smst = (EFI_PHYSICAL_ADDRESS)(UINTN)gSmst;

    SmmS3ResumeState->SmmS3ResumeEntryPoint = (EFI_PHYSICAL_ADDRESS)(UINTN)SmmRestoreCpu;

    SmmS3ResumeState->SmmS3StackSize = SIZE_32KB;
    SmmS3ResumeState->SmmS3StackBase = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocatePages (EFI_SIZE_TO_PAGES ((UINTN)SmmS3ResumeState->SmmS3StackSize));
    if (SmmS3ResumeState->SmmS3StackBase == 0) {
      SmmS3ResumeState->SmmS3StackSize = 0;
    }

    SmmS3ResumeState->SmmS3Cr0 = gSmmCr0;
    SmmS3ResumeState->SmmS3Cr3 = Cr3;
    SmmS3ResumeState->SmmS3Cr4 = gSmmCr4;

    if (sizeof (UINTN) == sizeof (UINT64)) {
      SmmS3ResumeState->Signature = SMM_S3_RESUME_SMM_64;
    }
    if (sizeof (UINTN) == sizeof (UINT32)) {
      SmmS3ResumeState->Signature = SMM_S3_RESUME_SMM_32;
    }
  }

  //
  // Check XD and BTS features
  //
  CheckProcessorFeature ();

  //
  // Initialize SMM Profile feature
  //
  InitSmmProfile (Cr3);

  //
  // Patch SmmS3ResumeState->SmmS3Cr3
  //
  InitSmmS3Cr3 ();

  DEBUG ((EFI_D_INFO, "SMM CPU Module exit from SMRAM with EFI_SUCCESS\n"));

  return EFI_SUCCESS;
}

/**

  Find out SMRAM information including SMRR base and SMRR size.

  @param          SmrrBase          SMRR base
  @param          SmrrSize          SMRR size

**/
VOID
FindSmramInfo (
  OUT UINT32   *SmrrBase,
  OUT UINT32   *SmrrSize
  )
{
  EFI_STATUS                        Status;
  UINTN                             Size;
  EFI_SMM_ACCESS2_PROTOCOL          *SmmAccess;
  EFI_SMRAM_DESCRIPTOR              *CurrentSmramRange;
  EFI_SMRAM_DESCRIPTOR              *SmramRanges;
  UINTN                             SmramRangeCount;
  UINTN                             Index;
  UINT64                            MaxSize;
  BOOLEAN                           Found;

  //
  // Get SMM Access Protocol
  //
  Status = gBS->LocateProtocol (&gEfiSmmAccess2ProtocolGuid, NULL, (VOID **)&SmmAccess);
  ASSERT_EFI_ERROR (Status);

  //
  // Get SMRAM information
  //
  Size = 0;
  Status = SmmAccess->GetCapabilities (SmmAccess, &Size, NULL);
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  SmramRanges = (EFI_SMRAM_DESCRIPTOR *)AllocatePool (Size);
  ASSERT (SmramRanges != NULL);

  Status = SmmAccess->GetCapabilities (SmmAccess, &Size, SmramRanges);
  ASSERT_EFI_ERROR (Status);

  SmramRangeCount = Size / sizeof (EFI_SMRAM_DESCRIPTOR);

  //
  // Find the largest SMRAM range between 1MB and 4GB that is at least 256K - 4K in size
  //
  CurrentSmramRange = NULL;
  for (Index = 0, MaxSize = SIZE_256KB - EFI_PAGE_SIZE; Index < SmramRangeCount; Index++) {
    //
    // Skip any SMRAM region that is already allocated, needs testing, or needs ECC initialization
    //
    if ((SmramRanges[Index].RegionState & (EFI_ALLOCATED | EFI_NEEDS_TESTING | EFI_NEEDS_ECC_INITIALIZATION)) != 0) {
      continue;
    }

    if (SmramRanges[Index].CpuStart >= BASE_1MB) {
      if ((SmramRanges[Index].CpuStart + SmramRanges[Index].PhysicalSize) <= BASE_4GB) {
        if (SmramRanges[Index].PhysicalSize >= MaxSize) {
          MaxSize = SmramRanges[Index].PhysicalSize;
          CurrentSmramRange = &SmramRanges[Index];
        }
      }
    }
  }

  ASSERT (CurrentSmramRange != NULL);

  *SmrrBase = (UINT32)CurrentSmramRange->CpuStart;
  *SmrrSize = (UINT32)CurrentSmramRange->PhysicalSize;

  do {
    Found = FALSE;
    for (Index = 0; Index < SmramRangeCount; Index++) {
      if (SmramRanges[Index].CpuStart < *SmrrBase && *SmrrBase == (SmramRanges[Index].CpuStart + SmramRanges[Index].PhysicalSize)) {
        *SmrrBase = (UINT32)SmramRanges[Index].CpuStart;
        *SmrrSize = (UINT32)(*SmrrSize + SmramRanges[Index].PhysicalSize);
        Found = TRUE;
      } else if ((*SmrrBase + *SmrrSize) == SmramRanges[Index].CpuStart && SmramRanges[Index].PhysicalSize > 0) {
        *SmrrSize = (UINT32)(*SmrrSize + SmramRanges[Index].PhysicalSize);
        Found = TRUE;
      }
    }
  } while (Found);

  DEBUG ((EFI_D_INFO, "SMRR Base: 0x%x, SMRR Size: 0x%x\n", *SmrrBase, *SmrrSize));
}

/**
Configure SMM Code Access Check feature on an AP.
SMM Feature Control MSR will be locked after configuration.

@param[in,out] Buffer  Pointer to private data buffer.
**/
VOID
EFIAPI
ConfigSmmCodeAccessCheckOnCurrentProcessor (
  IN OUT VOID  *Buffer
  )
{
  UINTN   CpuIndex;
  UINT64  SmmFeatureControlMsr;
  UINT64  NewSmmFeatureControlMsr;

  //
  // Retrieve the CPU Index from the context passed in
  //
  CpuIndex = *(UINTN *)Buffer;

  //
  // Get the current SMM Feature Control MSR value
  //
  SmmFeatureControlMsr = SmmCpuFeaturesGetSmmRegister (CpuIndex, SmmRegFeatureControl);

  //
  // Compute the new SMM Feature Control MSR value
  //
  NewSmmFeatureControlMsr = SmmFeatureControlMsr;
  if (mSmmCodeAccessCheckEnable) {
    NewSmmFeatureControlMsr |= SMM_CODE_CHK_EN_BIT;
    if (FeaturePcdGet (PcdCpuSmmFeatureControlMsrLock)) {
      NewSmmFeatureControlMsr |= SMM_FEATURE_CONTROL_LOCK_BIT;
    }
  }

  //
  // Only set the SMM Feature Control MSR value if the new value is different than the current value
  //
  if (NewSmmFeatureControlMsr != SmmFeatureControlMsr) {
    SmmCpuFeaturesSetSmmRegister (CpuIndex, SmmRegFeatureControl, NewSmmFeatureControlMsr);
  }

  //
  // Release the spin lock user to serialize the updates to the SMM Feature Control MSR
  //
  ReleaseSpinLock (&mConfigSmmCodeAccessCheckLock);
}

/**
Configure SMM Code Access Check feature for all processors.
SMM Feature Control MSR will be locked after configuration.
**/
VOID
ConfigSmmCodeAccessCheck (
  VOID
  )
{
  UINTN       Index;
  EFI_STATUS  Status;

  //
  // Check to see if the Feature Control MSR is supported on this CPU
  //
  Index = gSmmCpuPrivate->SmmCoreEntryContext.CurrentlyExecutingCpu;
  if (!SmmCpuFeaturesIsSmmRegisterSupported (Index, SmmRegFeatureControl)) {
    mSmmCodeAccessCheckEnable = FALSE;
    return;
  }

  //
  // Check to see if the CPU supports the SMM Code Access Check feature
  // Do not access this MSR unless the CPU supports the SmmRegFeatureControl
  //
  if ((AsmReadMsr64 (EFI_MSR_SMM_MCA_CAP) & SMM_CODE_ACCESS_CHK_BIT) == 0) {
    mSmmCodeAccessCheckEnable = FALSE;
    return;
  }

  //
  // Initialize the lock used to serialize the MSR programming in BSP and all APs
  //
  InitializeSpinLock (&mConfigSmmCodeAccessCheckLock);

  //
  // Acquire Config SMM Code Access Check spin lock.  The BSP will release the
  // spin lock when it is done executing ConfigSmmCodeAccessCheckOnCurrentProcessor().
  //
  AcquireSpinLock (&mConfigSmmCodeAccessCheckLock);

  //
  // Enable SMM Code Access Check feature on the BSP.
  //
  ConfigSmmCodeAccessCheckOnCurrentProcessor (&Index);

  //
  // Enable SMM Code Access Check feature for the APs.
  //
  for (Index = 0; Index < gSmst->NumberOfCpus; Index++) {
    if (Index != gSmmCpuPrivate->SmmCoreEntryContext.CurrentlyExecutingCpu) {

      //
      // Acquire Config SMM Code Access Check spin lock.  The AP will release the
      // spin lock when it is done executing ConfigSmmCodeAccessCheckOnCurrentProcessor().
      //
      AcquireSpinLock (&mConfigSmmCodeAccessCheckLock);

      //
      // Call SmmStartupThisAp() to enable SMM Code Access Check on an AP.
      //
      Status = gSmst->SmmStartupThisAp (ConfigSmmCodeAccessCheckOnCurrentProcessor, Index, &Index);
      ASSERT_EFI_ERROR (Status);

      //
      // Wait for the AP to release the Config SMM Code Access Check spin lock.
      //
      while (!AcquireSpinLockOrFail (&mConfigSmmCodeAccessCheckLock)) {
        CpuPause ();
      }

      //
      // Release the Config SMM Code Access Check spin lock.
      //
      ReleaseSpinLock (&mConfigSmmCodeAccessCheckLock);
    }
  }
}

/**
  This API provides a way to allocate memory for page table.

  This API can be called more once to allocate memory for page tables.

  Allocates the number of 4KB pages of type EfiRuntimeServicesData and returns a pointer to the
  allocated buffer.  The buffer returned is aligned on a 4KB boundary.  If Pages is 0, then NULL
  is returned.  If there is not enough memory remaining to satisfy the request, then NULL is
  returned.

  @param  Pages                 The number of 4 KB pages to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
AllocatePageTableMemory (
  IN UINTN           Pages
  )
{
  VOID  *Buffer;

  Buffer = SmmCpuFeaturesAllocatePageTableMemory (Pages);
  if (Buffer != NULL) {
    return Buffer;
  }
  return AllocatePages (Pages);
}

/**
  Perform the remaining tasks.

**/
VOID
PerformRemainingTasks (
  VOID
  )
{
  if (mSmmReadyToLock) {
    //
    // Start SMM Profile feature
    //
    if (FeaturePcdGet (PcdCpuSmmProfileEnable)) {
      SmmProfileStart ();
    }
    //
    // Create a mix of 2MB and 4KB page table. Update some memory ranges absent and execute-disable.
    //
    InitPaging ();
    //
    // Configure SMM Code Access Check feature if available.
    //
    ConfigSmmCodeAccessCheck ();

    SmmCpuFeaturesCompleteSmmReadyToLock ();

    //
    // Clean SMM ready to lock flag
    //
    mSmmReadyToLock = FALSE;
  }
}

/**
  Perform the pre tasks.

**/
VOID
PerformPreTasks (
  VOID
  )
{
  //
  // Restore SMM Configuration in S3 boot path.
  //
  if (mRestoreSmmConfigurationInS3) {
    //
    // Need make sure gSmst is correct because below function may use them.
    //
    gSmst->SmmStartupThisAp      = gSmmCpuPrivate->SmmCoreEntryContext.SmmStartupThisAp;
    gSmst->CurrentlyExecutingCpu = gSmmCpuPrivate->SmmCoreEntryContext.CurrentlyExecutingCpu;
    gSmst->NumberOfCpus          = gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus;
    gSmst->CpuSaveStateSize      = gSmmCpuPrivate->SmmCoreEntryContext.CpuSaveStateSize;
    gSmst->CpuSaveState          = gSmmCpuPrivate->SmmCoreEntryContext.CpuSaveState;

    //
    // Configure SMM Code Access Check feature if available.
    //
    ConfigSmmCodeAccessCheck ();

    SmmCpuFeaturesCompleteSmmReadyToLock ();

    mRestoreSmmConfigurationInS3 = FALSE;
  }
}
