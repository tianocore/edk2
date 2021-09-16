/** @file
Code for Processor S3 restoration

Copyright (c) 2006 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

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
// Flags used when program the register.
//
typedef struct {
  volatile UINTN           MemoryMappedLock;        // Spinlock used to program mmio
  volatile UINT32          *CoreSemaphoreCount;     // Semaphore container used to program
                                                    // core level semaphore.
  volatile UINT32          *PackageSemaphoreCount;  // Semaphore container used to program
                                                    // package level semaphore.
} PROGRAM_CPU_REGISTER_FLAGS;

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

PROGRAM_CPU_REGISTER_FLAGS   mCpuFlags;
ACPI_CPU_DATA                mAcpiCpuData;
volatile UINT32              mNumberToFinish;
MP_CPU_EXCHANGE_INFO         *mExchangeInfo;
BOOLEAN                      mRestoreSmmConfigurationInS3 = FALSE;

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
  Increment semaphore by 1.

  @param      Sem            IN:  32-bit unsigned integer

**/
VOID
S3ReleaseSemaphore (
  IN OUT  volatile UINT32           *Sem
  )
{
  InterlockedIncrement (Sem);
}

/**
  Decrement the semaphore by 1 if it is not zero.

  Performs an atomic decrement operation for semaphore.
  The compare exchange operation must be performed using
  MP safe mechanisms.

  @param      Sem            IN:  32-bit unsigned integer

**/
VOID
S3WaitForSemaphore (
  IN OUT  volatile UINT32           *Sem
  )
{
  UINT32  Value;

  do {
    Value = *Sem;
  } while (Value == 0 ||
           InterlockedCompareExchange32 (
             Sem,
             Value,
             Value - 1
             ) != Value);
}

/**
  Read / write CR value.

  @param[in]      CrIndex         The CR index which need to read/write.
  @param[in]      Read            Read or write. TRUE is read.
  @param[in,out]  CrValue         CR value.

  @retval    EFI_SUCCESS means read/write success, else return EFI_UNSUPPORTED.
**/
UINTN
ReadWriteCr (
  IN     UINT32       CrIndex,
  IN     BOOLEAN      Read,
  IN OUT UINTN        *CrValue
  )
{
  switch (CrIndex) {
  case 0:
    if (Read) {
      *CrValue = AsmReadCr0 ();
    } else {
      AsmWriteCr0 (*CrValue);
    }
    break;
  case 2:
    if (Read) {
      *CrValue = AsmReadCr2 ();
    } else {
      AsmWriteCr2 (*CrValue);
    }
    break;
  case 3:
    if (Read) {
      *CrValue = AsmReadCr3 ();
    } else {
      AsmWriteCr3 (*CrValue);
    }
    break;
  case 4:
    if (Read) {
      *CrValue = AsmReadCr4 ();
    } else {
      AsmWriteCr4 (*CrValue);
    }
    break;
  default:
    return EFI_UNSUPPORTED;;
  }

  return EFI_SUCCESS;
}

/**
  Initialize the CPU registers from a register table.

  @param[in]  RegisterTable         The register table for this AP.
  @param[in]  ApLocation            AP location info for this ap.
  @param[in]  CpuStatus             CPU status info for this CPU.
  @param[in]  CpuFlags              Flags data structure used when program the register.

  @note This service could be called by BSP/APs.
**/
VOID
ProgramProcessorRegister (
  IN CPU_REGISTER_TABLE           *RegisterTable,
  IN EFI_CPU_PHYSICAL_LOCATION    *ApLocation,
  IN CPU_STATUS_INFORMATION       *CpuStatus,
  IN PROGRAM_CPU_REGISTER_FLAGS   *CpuFlags
  )
{
  CPU_REGISTER_TABLE_ENTRY  *RegisterTableEntry;
  UINTN                     Index;
  UINTN                     Value;
  CPU_REGISTER_TABLE_ENTRY  *RegisterTableEntryHead;
  volatile UINT32           *SemaphorePtr;
  UINT32                    FirstThread;
  UINT32                    CurrentThread;
  UINT32                    CurrentCore;
  UINTN                     ProcessorIndex;
  UINT32                    *ThreadCountPerPackage;
  UINT8                     *ThreadCountPerCore;
  EFI_STATUS                Status;
  UINT64                    CurrentValue;

  //
  // Traverse Register Table of this logical processor
  //
  RegisterTableEntryHead = (CPU_REGISTER_TABLE_ENTRY *) (UINTN) RegisterTable->RegisterTableEntry;

  for (Index = 0; Index < RegisterTable->TableLength; Index++) {

    RegisterTableEntry = &RegisterTableEntryHead[Index];

    //
    // Check the type of specified register
    //
    switch (RegisterTableEntry->RegisterType) {
    //
    // The specified register is Control Register
    //
    case ControlRegister:
      Status = ReadWriteCr (RegisterTableEntry->Index, TRUE, &Value);
      if (EFI_ERROR (Status)) {
        break;
      }
      if (RegisterTableEntry->TestThenWrite) {
        CurrentValue = BitFieldRead64 (
                         Value,
                         RegisterTableEntry->ValidBitStart,
                         RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1
                         );
        if (CurrentValue == RegisterTableEntry->Value) {
          break;
        }
      }
      Value = (UINTN) BitFieldWrite64 (
                        Value,
                        RegisterTableEntry->ValidBitStart,
                        RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
                        RegisterTableEntry->Value
                        );
      ReadWriteCr (RegisterTableEntry->Index, FALSE, &Value);
      break;
    //
    // The specified register is Model Specific Register
    //
    case Msr:
      if (RegisterTableEntry->TestThenWrite) {
        Value = (UINTN)AsmReadMsr64 (RegisterTableEntry->Index);
        if (RegisterTableEntry->ValidBitLength >= 64) {
          if (Value == RegisterTableEntry->Value) {
            break;
          }
        } else {
          CurrentValue = BitFieldRead64 (
                           Value,
                           RegisterTableEntry->ValidBitStart,
                           RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1
                           );
          if (CurrentValue == RegisterTableEntry->Value) {
            break;
          }
        }
      }

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
        // Set the bit section according to bit start and length
        //
        AsmMsrBitFieldWrite64 (
          RegisterTableEntry->Index,
          RegisterTableEntry->ValidBitStart,
          RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
          RegisterTableEntry->Value
          );
      }
      break;
    //
    // MemoryMapped operations
    //
    case MemoryMapped:
      AcquireSpinLock (&CpuFlags->MemoryMappedLock);
      MmioBitFieldWrite32 (
        (UINTN)(RegisterTableEntry->Index | LShiftU64 (RegisterTableEntry->HighIndex, 32)),
        RegisterTableEntry->ValidBitStart,
        RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
        (UINT32)RegisterTableEntry->Value
        );
      ReleaseSpinLock (&CpuFlags->MemoryMappedLock);
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

    case Semaphore:
      // Semaphore works logic like below:
      //
      //  V(x) = LibReleaseSemaphore (Semaphore[FirstThread + x]);
      //  P(x) = LibWaitForSemaphore (Semaphore[FirstThread + x]);
      //
      //  All threads (T0...Tn) waits in P() line and continues running
      //  together.
      //
      //
      //  T0             T1            ...           Tn
      //
      //  V(0...n)       V(0...n)      ...           V(0...n)
      //  n * P(0)       n * P(1)      ...           n * P(n)
      //
      ASSERT (
        (ApLocation != NULL) &&
        (CpuStatus->ThreadCountPerPackage != 0) &&
        (CpuStatus->ThreadCountPerCore != 0) &&
        (CpuFlags->CoreSemaphoreCount != NULL) &&
        (CpuFlags->PackageSemaphoreCount != NULL)
        );
      switch (RegisterTableEntry->Value) {
      case CoreDepType:
        SemaphorePtr = CpuFlags->CoreSemaphoreCount;
        ThreadCountPerCore = (UINT8 *)(UINTN)CpuStatus->ThreadCountPerCore;

        CurrentCore = ApLocation->Package * CpuStatus->MaxCoreCount + ApLocation->Core;
        //
        // Get Offset info for the first thread in the core which current thread belongs to.
        //
        FirstThread   = CurrentCore * CpuStatus->MaxThreadCount;
        CurrentThread = FirstThread + ApLocation->Thread;

        //
        // Different cores may have different valid threads in them. If driver maintail clearly
        // thread index in different cores, the logic will be much complicated.
        // Here driver just simply records the max thread number in all cores and use it as expect
        // thread number for all cores.
        // In below two steps logic, first current thread will Release semaphore for each thread
        // in current core. Maybe some threads are not valid in this core, but driver don't
        // care. Second, driver will let current thread wait semaphore for all valid threads in
        // current core. Because only the valid threads will do release semaphore for this
        // thread, driver here only need to wait the valid thread count.
        //

        //
        // First Notify ALL THREADs in current Core that this thread is ready.
        //
        for (ProcessorIndex = 0; ProcessorIndex < CpuStatus->MaxThreadCount; ProcessorIndex ++) {
          S3ReleaseSemaphore (&SemaphorePtr[FirstThread + ProcessorIndex]);
        }
        //
        // Second, check whether all VALID THREADs (not all threads) in current core are ready.
        //
        for (ProcessorIndex = 0; ProcessorIndex < ThreadCountPerCore[CurrentCore]; ProcessorIndex ++) {
          S3WaitForSemaphore (&SemaphorePtr[CurrentThread]);
        }
        break;

      case PackageDepType:
        SemaphorePtr = CpuFlags->PackageSemaphoreCount;
        ThreadCountPerPackage = (UINT32 *)(UINTN)CpuStatus->ThreadCountPerPackage;
        //
        // Get Offset info for the first thread in the package which current thread belongs to.
        //
        FirstThread = ApLocation->Package * CpuStatus->MaxCoreCount * CpuStatus->MaxThreadCount;
        //
        // Get the possible threads count for current package.
        //
        CurrentThread = FirstThread + CpuStatus->MaxThreadCount * ApLocation->Core + ApLocation->Thread;

        //
        // Different packages may have different valid threads in them. If driver maintail clearly
        // thread index in different packages, the logic will be much complicated.
        // Here driver just simply records the max thread number in all packages and use it as expect
        // thread number for all packages.
        // In below two steps logic, first current thread will Release semaphore for each thread
        // in current package. Maybe some threads are not valid in this package, but driver don't
        // care. Second, driver will let current thread wait semaphore for all valid threads in
        // current package. Because only the valid threads will do release semaphore for this
        // thread, driver here only need to wait the valid thread count.
        //

        //
        // First Notify ALL THREADS in current package that this thread is ready.
        //
        for (ProcessorIndex = 0; ProcessorIndex < CpuStatus->MaxThreadCount * CpuStatus->MaxCoreCount; ProcessorIndex ++) {
          S3ReleaseSemaphore (&SemaphorePtr[FirstThread + ProcessorIndex]);
        }
        //
        // Second, check whether VALID THREADS (not all threads) in current package are ready.
        //
        for (ProcessorIndex = 0; ProcessorIndex < ThreadCountPerPackage[ApLocation->Package]; ProcessorIndex ++) {
          S3WaitForSemaphore (&SemaphorePtr[CurrentThread]);
        }
        break;

      default:
        break;
      }
      break;

    default:
      break;
    }
  }
}

/**

  Set Processor register for one AP.

  @param     PreSmmRegisterTable     Use pre Smm register table or register table.

**/
VOID
SetRegister (
  IN BOOLEAN                 PreSmmRegisterTable
  )
{
  CPU_FEATURE_INIT_DATA     *FeatureInitData;
  CPU_REGISTER_TABLE        *RegisterTable;
  CPU_REGISTER_TABLE        *RegisterTables;
  UINT32                    InitApicId;
  UINTN                     ProcIndex;
  UINTN                     Index;

  FeatureInitData = &mAcpiCpuData.CpuFeatureInitData;

  if (PreSmmRegisterTable) {
    RegisterTables = (CPU_REGISTER_TABLE *)(UINTN)FeatureInitData->PreSmmInitRegisterTable;
  } else {
    RegisterTables = (CPU_REGISTER_TABLE *)(UINTN)FeatureInitData->RegisterTable;
  }
  if (RegisterTables == NULL) {
    return;
  }

  InitApicId = GetInitialApicId ();
  RegisterTable = NULL;
  ProcIndex = (UINTN)-1;
  for (Index = 0; Index < mAcpiCpuData.NumberOfCpus; Index++) {
    if (RegisterTables[Index].InitialApicId == InitApicId) {
      RegisterTable = &RegisterTables[Index];
      ProcIndex = Index;
      break;
    }
  }
  ASSERT (RegisterTable != NULL);

  if (FeatureInitData->ApLocation != 0) {
    ProgramProcessorRegister (
      RegisterTable,
      (EFI_CPU_PHYSICAL_LOCATION *)(UINTN)FeatureInitData->ApLocation + ProcIndex,
      &FeatureInitData->CpuStatus,
      &mCpuFlags
      );
  } else {
    ProgramProcessorRegister (
      RegisterTable,
      NULL,
      &FeatureInitData->CpuStatus,
      &mCpuFlags
      );
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

  SetRegister (TRUE);

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

  SetRegister (FALSE);

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

  SetRegister (TRUE);

  ProgramVirtualWireMode ();

  PrepareApStartupVector (mAcpiCpuData.StartupVector);

  if (FeaturePcdGet (PcdCpuHotPlugSupport)) {
    ASSERT (mNumberOfCpus <= mAcpiCpuData.NumberOfCpus);
  } else {
    ASSERT (mNumberOfCpus == mAcpiCpuData.NumberOfCpus);
  }
  mNumberToFinish = (UINT32)(mNumberOfCpus - 1);
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
  if (FeaturePcdGet (PcdCpuHotPlugSupport)) {
    ASSERT (mNumberOfCpus <= mAcpiCpuData.NumberOfCpus);
  } else {
    ASSERT (mNumberOfCpus == mAcpiCpuData.NumberOfCpus);
  }
  mNumberToFinish = (UINT32)(mNumberOfCpus - 1);

  //
  // Signal that SMM base relocation is complete and to continue initialization for all APs.
  //
  mInitApsAfterSmmBaseReloc = TRUE;

  //
  // Must begin set register after all APs have continue their initialization.
  // This is a requirement to support semaphore mechanism in register table.
  // Because if semaphore's dependence type is package type, semaphore will wait
  // for all Aps in one package finishing their tasks before set next register
  // for all APs. If the Aps not begin its task during BSP doing its task, the
  // BSP thread will hang because it is waiting for other Aps in the same
  // package finishing their task.
  //
  SetRegister (FALSE);

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
  if (GuidHob == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR:%a(): HOB(gEfiAcpiVariableGuid=%g) needed by S3 resume doesn't exist!\n",
      __FUNCTION__,
      &gEfiAcpiVariableGuid
    ));
    CpuDeadLoop ();
  } else {
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

    //
    // Patch SmmS3ResumeState->SmmS3Cr3
    //
    InitSmmS3Cr3 ();
  }

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
  Copy register table from non-SMRAM into SMRAM.

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
  CPU_REGISTER_TABLE_ENTRY   *RegisterTableEntry;

  CopyMem (DestinationRegisterTableList, SourceRegisterTableList, NumberOfCpus * sizeof (CPU_REGISTER_TABLE));
  for (Index = 0; Index < NumberOfCpus; Index++) {
    if (DestinationRegisterTableList[Index].TableLength != 0) {
      DestinationRegisterTableList[Index].AllocatedSize = DestinationRegisterTableList[Index].TableLength * sizeof (CPU_REGISTER_TABLE_ENTRY);
      RegisterTableEntry = AllocateCopyPool (
        DestinationRegisterTableList[Index].AllocatedSize,
        (VOID *)(UINTN)SourceRegisterTableList[Index].RegisterTableEntry
        );
      ASSERT (RegisterTableEntry != NULL);
      DestinationRegisterTableList[Index].RegisterTableEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)RegisterTableEntry;
    }
  }
}

/**
  Check whether the register table is empty or not.

  @param[in] RegisterTable  Point to the register table.
  @param[in] NumberOfCpus   Number of CPUs.

  @retval TRUE              The register table is empty.
  @retval FALSE             The register table is not empty.
**/
BOOLEAN
IsRegisterTableEmpty (
  IN CPU_REGISTER_TABLE     *RegisterTable,
  IN UINT32                 NumberOfCpus
  )
{
  UINTN                     Index;

  if (RegisterTable != NULL) {
    for (Index = 0; Index < NumberOfCpus; Index++) {
      if (RegisterTable[Index].TableLength != 0) {
        return FALSE;
      }
    }
  }

  return TRUE;
}

/**
  Copy the data used to initialize processor register into SMRAM.

  @param[in,out]  CpuFeatureInitDataDst   Pointer to the destination CPU_FEATURE_INIT_DATA structure.
  @param[in]      CpuFeatureInitDataSrc   Pointer to the source CPU_FEATURE_INIT_DATA structure.

**/
VOID
CopyCpuFeatureInitDatatoSmram (
  IN OUT CPU_FEATURE_INIT_DATA    *CpuFeatureInitDataDst,
  IN     CPU_FEATURE_INIT_DATA    *CpuFeatureInitDataSrc
  )
{
  CPU_STATUS_INFORMATION    *CpuStatus;

  if (!IsRegisterTableEmpty ((CPU_REGISTER_TABLE *)(UINTN)CpuFeatureInitDataSrc->PreSmmInitRegisterTable, mAcpiCpuData.NumberOfCpus)) {
    CpuFeatureInitDataDst->PreSmmInitRegisterTable = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocatePool (mAcpiCpuData.NumberOfCpus * sizeof (CPU_REGISTER_TABLE));
    ASSERT (CpuFeatureInitDataDst->PreSmmInitRegisterTable != 0);

    CopyRegisterTable (
      (CPU_REGISTER_TABLE *)(UINTN)CpuFeatureInitDataDst->PreSmmInitRegisterTable,
      (CPU_REGISTER_TABLE *)(UINTN)CpuFeatureInitDataSrc->PreSmmInitRegisterTable,
      mAcpiCpuData.NumberOfCpus
      );
  }

  if (!IsRegisterTableEmpty ((CPU_REGISTER_TABLE *)(UINTN)CpuFeatureInitDataSrc->RegisterTable, mAcpiCpuData.NumberOfCpus)) {
    CpuFeatureInitDataDst->RegisterTable = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocatePool (mAcpiCpuData.NumberOfCpus * sizeof (CPU_REGISTER_TABLE));
    ASSERT (CpuFeatureInitDataDst->RegisterTable != 0);

    CopyRegisterTable (
      (CPU_REGISTER_TABLE *)(UINTN)CpuFeatureInitDataDst->RegisterTable,
      (CPU_REGISTER_TABLE *)(UINTN)CpuFeatureInitDataSrc->RegisterTable,
      mAcpiCpuData.NumberOfCpus
      );
  }

  CpuStatus = &CpuFeatureInitDataDst->CpuStatus;
  CopyMem (CpuStatus, &CpuFeatureInitDataSrc->CpuStatus, sizeof (CPU_STATUS_INFORMATION));

  if (CpuFeatureInitDataSrc->CpuStatus.ThreadCountPerPackage != 0) {
    CpuStatus->ThreadCountPerPackage = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateCopyPool (
                                            sizeof (UINT32) * CpuStatus->PackageCount,
                                            (UINT32 *)(UINTN)CpuFeatureInitDataSrc->CpuStatus.ThreadCountPerPackage
                                            );
    ASSERT (CpuStatus->ThreadCountPerPackage != 0);
  }

  if (CpuFeatureInitDataSrc->CpuStatus.ThreadCountPerCore != 0) {
    CpuStatus->ThreadCountPerCore = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateCopyPool (
                                            sizeof (UINT8) * (CpuStatus->PackageCount * CpuStatus->MaxCoreCount),
                                            (UINT32 *)(UINTN)CpuFeatureInitDataSrc->CpuStatus.ThreadCountPerCore
                                            );
    ASSERT (CpuStatus->ThreadCountPerCore != 0);
  }

  if (CpuFeatureInitDataSrc->ApLocation != 0) {
    CpuFeatureInitDataDst->ApLocation = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateCopyPool (
                                mAcpiCpuData.NumberOfCpus * sizeof (EFI_CPU_PHYSICAL_LOCATION),
                                (EFI_CPU_PHYSICAL_LOCATION *)(UINTN)CpuFeatureInitDataSrc->ApLocation
                                );
    ASSERT (CpuFeatureInitDataDst->ApLocation != 0);
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
  VOID                       *GdtForAp;
  VOID                       *IdtForAp;
  VOID                       *MachineCheckHandlerForAp;
  CPU_STATUS_INFORMATION     *CpuStatus;

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

  //
  // Copy AP's GDT, IDT and Machine Check handler into SMRAM.
  //
  Gdtr = (IA32_DESCRIPTOR *)(UINTN)mAcpiCpuData.GdtrProfile;
  Idtr = (IA32_DESCRIPTOR *)(UINTN)mAcpiCpuData.IdtrProfile;

  GdtForAp = AllocatePool ((Gdtr->Limit + 1) + (Idtr->Limit + 1) + mAcpiCpuData.ApMachineCheckHandlerSize);
  ASSERT (GdtForAp != NULL);
  IdtForAp = (VOID *) ((UINTN)GdtForAp + (Gdtr->Limit + 1));
  MachineCheckHandlerForAp = (VOID *) ((UINTN)IdtForAp + (Idtr->Limit + 1));

  CopyMem (GdtForAp, (VOID *)Gdtr->Base, Gdtr->Limit + 1);
  CopyMem (IdtForAp, (VOID *)Idtr->Base, Idtr->Limit + 1);
  CopyMem (MachineCheckHandlerForAp, (VOID *)(UINTN)mAcpiCpuData.ApMachineCheckHandlerBase, mAcpiCpuData.ApMachineCheckHandlerSize);

  Gdtr->Base = (UINTN)GdtForAp;
  Idtr->Base = (UINTN)IdtForAp;
  mAcpiCpuData.ApMachineCheckHandlerBase = (EFI_PHYSICAL_ADDRESS)(UINTN)MachineCheckHandlerForAp;

  ZeroMem (&mAcpiCpuData.CpuFeatureInitData, sizeof (CPU_FEATURE_INIT_DATA));

  if (!PcdGetBool (PcdCpuFeaturesInitOnS3Resume)) {
    //
    // If the CPU features will not be initialized by CpuFeaturesPei module during
    // next ACPI S3 resume, copy the CPU features initialization data into SMRAM,
    // which will be consumed in SmmRestoreCpu during next S3 resume.
    //
    CopyCpuFeatureInitDatatoSmram (&mAcpiCpuData.CpuFeatureInitData, &AcpiCpuData->CpuFeatureInitData);

    CpuStatus = &mAcpiCpuData.CpuFeatureInitData.CpuStatus;

    mCpuFlags.CoreSemaphoreCount = AllocateZeroPool (
                                     sizeof (UINT32) * CpuStatus->PackageCount *
                                     CpuStatus->MaxCoreCount * CpuStatus->MaxThreadCount
                                     );
    ASSERT (mCpuFlags.CoreSemaphoreCount != NULL);

    mCpuFlags.PackageSemaphoreCount = AllocateZeroPool (
                                        sizeof (UINT32) * CpuStatus->PackageCount *
                                        CpuStatus->MaxCoreCount * CpuStatus->MaxThreadCount
                                        );
    ASSERT (mCpuFlags.PackageSemaphoreCount != NULL);

    InitializeSpinLock((SPIN_LOCK*) &mCpuFlags.MemoryMappedLock);
  }
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
