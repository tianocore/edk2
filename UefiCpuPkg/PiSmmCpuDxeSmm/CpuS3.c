/** @file
Code for Processor S3 restoration

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PiSmmCpuDxeSmm.h"

#pragma pack(1)
typedef struct {
  UINTN             Lock;
  VOID              *StackStart;
  UINTN             StackSize;
  VOID              *ApFunction;
  IA32_DESCRIPTOR   GdtrProfile;
  IA32_DESCRIPTOR   IdtrProfile;
  UINT32            BufferStart;
  UINT32            Cr3;
  UINTN             InitializeFloatingPointUnitsAddress;
} MP_CPU_EXCHANGE_INFO;
#pragma pack()

typedef struct {
  UINT8 *RendezvousFunnelAddress;
  UINTN PModeEntryOffset;
  UINTN FlatJumpOffset;
  UINTN Size;
  UINTN LModeEntryOffset;
  UINTN LongJumpOffset;
} MP_ASSEMBLY_ADDRESS_MAP;

//
// Spin lock used to serialize MemoryMapped operation
//
SPIN_LOCK                *mMemoryMappedLock = NULL;

//
// Signal that SMM BASE relocation is complete.
//
volatile BOOLEAN         mInitApsAfterSmmBaseReloc;

/**
  Get starting address and size of the rendezvous entry for APs.
  Information for fixing a jump instruction in the code is also returned.

  @param AddressMap  Output buffer for address map information.
**/
VOID *
EFIAPI
AsmGetAddressMap (
  MP_ASSEMBLY_ADDRESS_MAP                     *AddressMap
  );

#define LEGACY_REGION_SIZE    (2 * 0x1000)
#define LEGACY_REGION_BASE    (0xA0000 - LEGACY_REGION_SIZE)

ACPI_CPU_DATA                mAcpiCpuData;
volatile UINT32              mNumberToFinish;
MP_CPU_EXCHANGE_INFO         *mExchangeInfo;
BOOLEAN                      mRestoreSmmConfigurationInS3 = FALSE;
VOID                         *mGdtForAp = NULL;
VOID                         *mIdtForAp = NULL;
VOID                         *mMachineCheckHandlerForAp = NULL;
MP_MSR_LOCK                  *mMsrSpinLocks = NULL;
UINTN                        mMsrSpinLockCount;
UINTN                        mMsrCount = 0;

//
// S3 boot flag
//
BOOLEAN                      mSmmS3Flag = FALSE;

//
// Pointer to structure used during S3 Resume
//
SMM_S3_RESUME_STATE          *mSmmS3ResumeState = NULL;

BOOLEAN                      mAcpiS3Enable = TRUE;

UINT8                        *mApHltLoopCode = NULL;
UINT8                        mApHltLoopCodeTemplate[] = {
                               0x8B, 0x44, 0x24, 0x04,  // mov  eax, dword ptr [esp+4]
                               0xF0, 0xFF, 0x08,        // lock dec  dword ptr [eax]
                               0xFA,                    // cli
                               0xF4,                    // hlt
                               0xEB, 0xFC               // jmp $-2
                               };

/**
  Get MSR spin lock by MSR index.

  @param  MsrIndex       MSR index value.

  @return Pointer to MSR spin lock.

**/
SPIN_LOCK *
GetMsrSpinLockByIndex (
  IN UINT32      MsrIndex
  )
{
  UINTN     Index;
  for (Index = 0; Index < mMsrCount; Index++) {
    if (MsrIndex == mMsrSpinLocks[Index].MsrIndex) {
      return mMsrSpinLocks[Index].SpinLock;
    }
  }
  return NULL;
}

/**
  Initialize MSR spin lock by MSR index.

  @param  MsrIndex       MSR index value.

**/
VOID
InitMsrSpinLockByIndex (
  IN UINT32      MsrIndex
  )
{
  UINTN    MsrSpinLockCount;
  UINTN    NewMsrSpinLockCount;
  UINTN    Index;
  UINTN    AddedSize;

  if (mMsrSpinLocks == NULL) {
    MsrSpinLockCount = mSmmCpuSemaphores.SemaphoreMsr.AvailableCounter;
    mMsrSpinLocks = (MP_MSR_LOCK *) AllocatePool (sizeof (MP_MSR_LOCK) * MsrSpinLockCount);
    ASSERT (mMsrSpinLocks != NULL);
    for (Index = 0; Index < MsrSpinLockCount; Index++) {
      mMsrSpinLocks[Index].SpinLock =
       (SPIN_LOCK *)((UINTN)mSmmCpuSemaphores.SemaphoreMsr.Msr + Index * mSemaphoreSize);
      mMsrSpinLocks[Index].MsrIndex = (UINT32)-1;
    }
    mMsrSpinLockCount = MsrSpinLockCount;
    mSmmCpuSemaphores.SemaphoreMsr.AvailableCounter = 0;
  }
  if (GetMsrSpinLockByIndex (MsrIndex) == NULL) {
    //
    // Initialize spin lock for MSR programming
    //
    mMsrSpinLocks[mMsrCount].MsrIndex = MsrIndex;
    InitializeSpinLock (mMsrSpinLocks[mMsrCount].SpinLock);
    mMsrCount ++;
    if (mMsrCount == mMsrSpinLockCount) {
      //
      // If MSR spin lock buffer is full, enlarge it
      //
      AddedSize = SIZE_4KB;
      mSmmCpuSemaphores.SemaphoreMsr.Msr =
                        AllocatePages (EFI_SIZE_TO_PAGES(AddedSize));
      ASSERT (mSmmCpuSemaphores.SemaphoreMsr.Msr != NULL);
      NewMsrSpinLockCount = mMsrSpinLockCount + AddedSize / mSemaphoreSize;
      mMsrSpinLocks = ReallocatePool (
                        sizeof (MP_MSR_LOCK) * mMsrSpinLockCount,
                        sizeof (MP_MSR_LOCK) * NewMsrSpinLockCount,
                        mMsrSpinLocks
                        );
      ASSERT (mMsrSpinLocks != NULL);
      mMsrSpinLockCount = NewMsrSpinLockCount;
      for (Index = mMsrCount; Index < mMsrSpinLockCount; Index++) {
        mMsrSpinLocks[Index].SpinLock =
                 (SPIN_LOCK *)((UINTN)mSmmCpuSemaphores.SemaphoreMsr.Msr +
                 (Index - mMsrCount)  * mSemaphoreSize);
        mMsrSpinLocks[Index].MsrIndex = (UINT32)-1;
      }
    }
  }
}

/**
  Sync up the MTRR values for all processors.

  @param MtrrTable  Table holding fixed/variable MTRR values to be loaded.
**/
VOID
EFIAPI
LoadMtrrData (
  EFI_PHYSICAL_ADDRESS       MtrrTable
  )
/*++

Routine Description:

  Sync up the MTRR values for all processors.

Arguments:

Returns:
    None

--*/
{
  MTRR_SETTINGS   *MtrrSettings;

  MtrrSettings = (MTRR_SETTINGS *) (UINTN) MtrrTable;
  MtrrSetAllMtrrs (MtrrSettings);
}

/**
  Programs registers for the calling processor.

  This function programs registers for the calling processor.

  @param  RegisterTables        Pointer to register table of the running processor.
  @param  RegisterTableCount    Register table count.

**/
VOID
SetProcessorRegister (
  IN CPU_REGISTER_TABLE        *RegisterTables,
  IN UINTN                     RegisterTableCount
  )
{
  CPU_REGISTER_TABLE_ENTRY  *RegisterTableEntry;
  UINTN                     Index;
  UINTN                     Value;
  SPIN_LOCK                 *MsrSpinLock;
  UINT32                    InitApicId;
  CPU_REGISTER_TABLE        *RegisterTable;

  InitApicId = GetInitialApicId ();
  RegisterTable = NULL;
  for (Index = 0; Index < RegisterTableCount; Index++) {
    if (RegisterTables[Index].InitialApicId == InitApicId) {
      RegisterTable =  &RegisterTables[Index];
      break;
    }
  }
  ASSERT (RegisterTable != NULL);

  //
  // Traverse Register Table of this logical processor
  //
  RegisterTableEntry = (CPU_REGISTER_TABLE_ENTRY *) (UINTN) RegisterTable->RegisterTableEntry;
  for (Index = 0; Index < RegisterTable->TableLength; Index++, RegisterTableEntry++) {
    //
    // Check the type of specified register
    //
    switch (RegisterTableEntry->RegisterType) {
    //
    // The specified register is Control Register
    //
    case ControlRegister:
      switch (RegisterTableEntry->Index) {
      case 0:
        Value = AsmReadCr0 ();
        Value = (UINTN) BitFieldWrite64 (
                          Value,
                          RegisterTableEntry->ValidBitStart,
                          RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
                          (UINTN) RegisterTableEntry->Value
                          );
        AsmWriteCr0 (Value);
        break;
      case 2:
        Value = AsmReadCr2 ();
        Value = (UINTN) BitFieldWrite64 (
                          Value,
                          RegisterTableEntry->ValidBitStart,
                          RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
                          (UINTN) RegisterTableEntry->Value
                          );
        AsmWriteCr2 (Value);
        break;
      case 3:
        Value = AsmReadCr3 ();
        Value = (UINTN) BitFieldWrite64 (
                          Value,
                          RegisterTableEntry->ValidBitStart,
                          RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
                          (UINTN) RegisterTableEntry->Value
                          );
        AsmWriteCr3 (Value);
        break;
      case 4:
        Value = AsmReadCr4 ();
        Value = (UINTN) BitFieldWrite64 (
                          Value,
                          RegisterTableEntry->ValidBitStart,
                          RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
                          (UINTN) RegisterTableEntry->Value
                          );
        AsmWriteCr4 (Value);
        break;
      default:
        break;
      }
      break;
    //
    // The specified register is Model Specific Register
    //
    case Msr:
      //
      // If this function is called to restore register setting after INIT signal,
      // there is no need to restore MSRs in register table.
      //
      if (RegisterTableEntry->ValidBitLength >= 64) {
        //
        // If length is not less than 64 bits, then directly write without reading
        //
        AsmWriteMsr64 (
          RegisterTableEntry->Index,
          RegisterTableEntry->Value
          );
      } else {
        //
        // Get lock to avoid Package/Core scope MSRs programming issue in parallel execution mode
        // to make sure MSR read/write operation is atomic.
        //
        MsrSpinLock = GetMsrSpinLockByIndex (RegisterTableEntry->Index);
        AcquireSpinLock (MsrSpinLock);
        //
        // Set the bit section according to bit start and length
        //
        AsmMsrBitFieldWrite64 (
          RegisterTableEntry->Index,
          RegisterTableEntry->ValidBitStart,
          RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
          RegisterTableEntry->Value
          );
        ReleaseSpinLock (MsrSpinLock);
      }
      break;
    //
    // MemoryMapped operations
    //
    case MemoryMapped:
      AcquireSpinLock (mMemoryMappedLock);
      MmioBitFieldWrite32 (
        (UINTN)(RegisterTableEntry->Index | LShiftU64 (RegisterTableEntry->HighIndex, 32)),
        RegisterTableEntry->ValidBitStart,
        RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
        (UINT32)RegisterTableEntry->Value
        );
      ReleaseSpinLock (mMemoryMappedLock);
      break;
    //
    // Enable or disable cache
    //
    case CacheControl:
      //
      // If value of the entry is 0, then disable cache.  Otherwise, enable cache.
      //
      if (RegisterTableEntry->Value == 0) {
        AsmDisableCache ();
      } else {
        AsmEnableCache ();
      }
      break;

    default:
      break;
    }
  }
}

/**
  AP initialization before then after SMBASE relocation in the S3 boot path.
**/
VOID
InitializeAp (
  VOID
  )
{
  UINTN                      TopOfStack;
  UINT8                      Stack[128];

  LoadMtrrData (mAcpiCpuData.MtrrTable);

  SetProcessorRegister ((CPU_REGISTER_TABLE *) (UINTN) mAcpiCpuData.PreSmmInitRegisterTable, mAcpiCpuData.NumberOfCpus);

  //
  // Count down the number with lock mechanism.
  //
  InterlockedDecrement (&mNumberToFinish);

  //
  // Wait for BSP to signal SMM Base relocation done.
  //
  while (!mInitApsAfterSmmBaseReloc) {
    CpuPause ();
  }

  ProgramVirtualWireMode ();
  DisableLvtInterrupts ();

  SetProcessorRegister ((CPU_REGISTER_TABLE *) (UINTN) mAcpiCpuData.RegisterTable, mAcpiCpuData.NumberOfCpus);

  //
  // Place AP into the safe code, count down the number with lock mechanism in the safe code.
  //
  TopOfStack  = (UINTN) Stack + sizeof (Stack);
  TopOfStack &= ~(UINTN) (CPU_STACK_ALIGNMENT - 1);
  CopyMem ((VOID *) (UINTN) mApHltLoopCode, mApHltLoopCodeTemplate, sizeof (mApHltLoopCodeTemplate));
  TransferApToSafeState ((UINTN)mApHltLoopCode, TopOfStack, (UINTN)&mNumberToFinish);
}

/**
  Prepares startup vector for APs.

  This function prepares startup vector for APs.

  @param  WorkingBuffer  The address of the work buffer.
**/
VOID
PrepareApStartupVector (
  EFI_PHYSICAL_ADDRESS  WorkingBuffer
  )
{
  EFI_PHYSICAL_ADDRESS                        StartupVector;
  MP_ASSEMBLY_ADDRESS_MAP                     AddressMap;

  //
  // Get the address map of startup code for AP,
  // including code size, and offset of long jump instructions to redirect.
  //
  ZeroMem (&AddressMap, sizeof (AddressMap));
  AsmGetAddressMap (&AddressMap);

  StartupVector = WorkingBuffer;

  //
  // Copy AP startup code to startup vector, and then redirect the long jump
  // instructions for mode switching.
  //
  CopyMem ((VOID *) (UINTN) StartupVector, AddressMap.RendezvousFunnelAddress, AddressMap.Size);
  *(UINT32 *) (UINTN) (StartupVector + AddressMap.FlatJumpOffset + 3) = (UINT32) (StartupVector + AddressMap.PModeEntryOffset);
  if (AddressMap.LongJumpOffset != 0) {
    *(UINT32 *) (UINTN) (StartupVector + AddressMap.LongJumpOffset + 2) = (UINT32) (StartupVector + AddressMap.LModeEntryOffset);
  }

  //
  // Get the start address of exchange data between BSP and AP.
  //
  mExchangeInfo = (MP_CPU_EXCHANGE_INFO *) (UINTN) (StartupVector + AddressMap.Size);
  ZeroMem ((VOID *) mExchangeInfo, sizeof (MP_CPU_EXCHANGE_INFO));

  CopyMem ((VOID *) (UINTN) &mExchangeInfo->GdtrProfile, (VOID *) (UINTN) mAcpiCpuData.GdtrProfile, sizeof (IA32_DESCRIPTOR));
  CopyMem ((VOID *) (UINTN) &mExchangeInfo->IdtrProfile, (VOID *) (UINTN) mAcpiCpuData.IdtrProfile, sizeof (IA32_DESCRIPTOR));

  //
  // Copy AP's GDT, IDT and Machine Check handler from SMRAM to ACPI NVS memory
  //
  CopyMem ((VOID *) mExchangeInfo->GdtrProfile.Base, mGdtForAp, mExchangeInfo->GdtrProfile.Limit + 1);
  CopyMem ((VOID *) mExchangeInfo->IdtrProfile.Base, mIdtForAp, mExchangeInfo->IdtrProfile.Limit + 1);
  CopyMem ((VOID *)(UINTN) mAcpiCpuData.ApMachineCheckHandlerBase, mMachineCheckHandlerForAp, mAcpiCpuData.ApMachineCheckHandlerSize);

  mExchangeInfo->StackStart  = (VOID *) (UINTN) mAcpiCpuData.StackAddress;
  mExchangeInfo->StackSize   = mAcpiCpuData.StackSize;
  mExchangeInfo->BufferStart = (UINT32) StartupVector;
  mExchangeInfo->Cr3         = (UINT32) (AsmReadCr3 ());
  mExchangeInfo->InitializeFloatingPointUnitsAddress = (UINTN)InitializeFloatingPointUnits;
}

/**
  The function is invoked before SMBASE relocation in S3 path to restores CPU status.

  The function is invoked before SMBASE relocation in S3 path. It does first time microcode load
  and restores MTRRs for both BSP and APs.

**/
VOID
InitializeCpuBeforeRebase (
  VOID
  )
{
  LoadMtrrData (mAcpiCpuData.MtrrTable);

  SetProcessorRegister ((CPU_REGISTER_TABLE *) (UINTN) mAcpiCpuData.PreSmmInitRegisterTable, mAcpiCpuData.NumberOfCpus);

  ProgramVirtualWireMode ();

  PrepareApStartupVector (mAcpiCpuData.StartupVector);

  mNumberToFinish = mAcpiCpuData.NumberOfCpus - 1;
  mExchangeInfo->ApFunction  = (VOID *) (UINTN) InitializeAp;

  //
  // Execute code for before SmmBaseReloc. Note: This flag is maintained across S3 boots.
  //
  mInitApsAfterSmmBaseReloc = FALSE;

  //
  // Send INIT IPI - SIPI to all APs
  //
  SendInitSipiSipiAllExcludingSelf ((UINT32)mAcpiCpuData.StartupVector);

  while (mNumberToFinish > 0) {
    CpuPause ();
  }
}

/**
  The function is invoked after SMBASE relocation in S3 path to restores CPU status.

  The function is invoked after SMBASE relocation in S3 path. It restores configuration according to
  data saved by normal boot path for both BSP and APs.

**/
VOID
InitializeCpuAfterRebase (
  VOID
  )
{
  SetProcessorRegister ((CPU_REGISTER_TABLE *) (UINTN) mAcpiCpuData.RegisterTable, mAcpiCpuData.NumberOfCpus);

  mNumberToFinish = mAcpiCpuData.NumberOfCpus - 1;

  //
  // Signal that SMM base relocation is complete and to continue initialization.
  //
  mInitApsAfterSmmBaseReloc = TRUE;

  while (mNumberToFinish > 0) {
    CpuPause ();
  }
}

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

  mSmmS3Flag = TRUE;

  InitializeSpinLock (mMemoryMappedLock);

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
    InitializeCpuBeforeRebase ();
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
    InitializeCpuAfterRebase ();
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
  Initialize SMM S3 resume state structure used during S3 Resume.

  @param[in] Cr3    The base address of the page tables to use in SMM.

**/
VOID
InitSmmS3ResumeState (
  IN UINT32  Cr3
  )
{
  VOID                       *GuidHob;
  EFI_SMRAM_DESCRIPTOR       *SmramDescriptor;
  SMM_S3_RESUME_STATE        *SmmS3ResumeState;
  EFI_PHYSICAL_ADDRESS       Address;
  EFI_STATUS                 Status;

  if (!mAcpiS3Enable) {
    return;
  }

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

    SmmS3ResumeState->SmmS3Cr0 = mSmmCr0;
    SmmS3ResumeState->SmmS3Cr3 = Cr3;
    SmmS3ResumeState->SmmS3Cr4 = mSmmCr4;

    if (sizeof (UINTN) == sizeof (UINT64)) {
      SmmS3ResumeState->Signature = SMM_S3_RESUME_SMM_64;
    }
    if (sizeof (UINTN) == sizeof (UINT32)) {
      SmmS3ResumeState->Signature = SMM_S3_RESUME_SMM_32;
    }
  }

  //
  // Patch SmmS3ResumeState->SmmS3Cr3
  //
  InitSmmS3Cr3 ();

  //
  // Allocate safe memory in ACPI NVS for AP to execute hlt loop in
  // protected mode on S3 path
  //
  Address = BASE_4GB - 1;
  Status  = gBS->AllocatePages (
                   AllocateMaxAddress,
                   EfiACPIMemoryNVS,
                   EFI_SIZE_TO_PAGES (sizeof (mApHltLoopCodeTemplate)),
                   &Address
                   );
  ASSERT_EFI_ERROR (Status);
  mApHltLoopCode = (UINT8 *) (UINTN) Address;
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
    if (DestinationRegisterTableList[Index].AllocatedSize != 0) {
      RegisterTableEntry = AllocateCopyPool (
        DestinationRegisterTableList[Index].AllocatedSize,
        (VOID *)(UINTN)SourceRegisterTableList[Index].RegisterTableEntry
        );
      ASSERT (RegisterTableEntry != NULL);
      DestinationRegisterTableList[Index].RegisterTableEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)RegisterTableEntry;
      //
      // Go though all MSRs in register table to initialize MSR spin lock
      //
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
}

/**
  Get ACPI CPU data.

**/
VOID
GetAcpiCpuData (
  VOID
  )
{
  ACPI_CPU_DATA              *AcpiCpuData;
  IA32_DESCRIPTOR            *Gdtr;
  IA32_DESCRIPTOR            *Idtr;

  if (!mAcpiS3Enable) {
    return;
  }

  //
  // Prevent use of mAcpiCpuData by initialize NumberOfCpus to 0
  //
  mAcpiCpuData.NumberOfCpus = 0;

  //
  // If PcdCpuS3DataAddress was never set, then do not copy CPU S3 Data into SMRAM
  //
  AcpiCpuData = (ACPI_CPU_DATA *)(UINTN)PcdGet64 (PcdCpuS3DataAddress);
  if (AcpiCpuData == 0) {
    return;
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
}

/**
  Get ACPI S3 enable flag.

**/
VOID
GetAcpiS3EnableFlag (
  VOID
  )
{
  mAcpiS3Enable = PcdGetBool (PcdAcpiS3Enable);
}
