/** @file
Code for Processor S3 restoration

Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PiSmmCpuDxeSmm.h"

typedef struct {
  UINTN             Lock;
  VOID              *StackStart;
  UINTN             StackSize;
  VOID              *ApFunction;
  IA32_DESCRIPTOR   GdtrProfile;
  IA32_DESCRIPTOR   IdtrProfile;
  UINT32            BufferStart;
  UINT32            Cr3;
} MP_CPU_EXCHANGE_INFO;

typedef struct {
  UINT8 *RendezvousFunnelAddress;
  UINTN PModeEntryOffset;
  UINTN FlatJumpOffset;
  UINTN Size;
  UINTN LModeEntryOffset;
  UINTN LongJumpOffset;
} MP_ASSEMBLY_ADDRESS_MAP;

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
#define MSR_SPIN_LOCK_INIT_NUM 15

ACPI_CPU_DATA                mAcpiCpuData;
UINT32                       mNumberToFinish;
MP_CPU_EXCHANGE_INFO         *mExchangeInfo;
BOOLEAN                      mRestoreSmmConfigurationInS3 = FALSE;
VOID                         *mGdtForAp = NULL;
VOID                         *mIdtForAp = NULL;
VOID                         *mMachineCheckHandlerForAp = NULL;
MP_MSR_LOCK                  *mMsrSpinLocks = NULL;
UINTN                        mMsrSpinLockCount = MSR_SPIN_LOCK_INIT_NUM;
UINTN                        mMsrCount = 0;

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
      return &mMsrSpinLocks[Index].SpinLock;
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
  UINTN    NewMsrSpinLockCount;

  if (mMsrSpinLocks == NULL) {
    mMsrSpinLocks = (MP_MSR_LOCK *) AllocatePool (sizeof (MP_MSR_LOCK) * mMsrSpinLockCount);
    ASSERT (mMsrSpinLocks != NULL);
  }
  if (GetMsrSpinLockByIndex (MsrIndex) == NULL) {
    //
    // Initialize spin lock for MSR programming
    //
    mMsrSpinLocks[mMsrCount].MsrIndex = MsrIndex;
    InitializeSpinLock (&mMsrSpinLocks[mMsrCount].SpinLock);
    mMsrCount ++;
    if (mMsrCount == mMsrSpinLockCount) {
      //
      // If MSR spin lock buffer is full, enlarge it
      //
      NewMsrSpinLockCount = mMsrSpinLockCount + MSR_SPIN_LOCK_INIT_NUM;
      mMsrSpinLocks = ReallocatePool (
                        sizeof (MP_MSR_LOCK) * mMsrSpinLockCount,
                        sizeof (MP_MSR_LOCK) * NewMsrSpinLockCount,
                        mMsrSpinLocks
                        );
      mMsrSpinLockCount = NewMsrSpinLockCount;
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

  @param  RegisterTable Pointer to register table of the running processor.

**/
VOID
SetProcessorRegister (
  IN CPU_REGISTER_TABLE        *RegisterTable
  )
{
  CPU_REGISTER_TABLE_ENTRY  *RegisterTableEntry;
  UINTN                     Index;
  UINTN                     Value;
  SPIN_LOCK                 *MsrSpinLock;

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
  AP initialization before SMBASE relocation in the S3 boot path.
**/
VOID
EarlyMPRendezvousProcedure (
  VOID
  )
{
  CPU_REGISTER_TABLE         *RegisterTableList;
  UINT32                     InitApicId;
  UINTN                      Index;

  LoadMtrrData (mAcpiCpuData.MtrrTable);

  //
  // Find processor number for this CPU.
  //
  RegisterTableList = (CPU_REGISTER_TABLE *) (UINTN) mAcpiCpuData.PreSmmInitRegisterTable;
  InitApicId = GetInitialApicId ();
  for (Index = 0; Index < mAcpiCpuData.NumberOfCpus; Index++) {
    if (RegisterTableList[Index].InitialApicId == InitApicId) {
      SetProcessorRegister (&RegisterTableList[Index]);
      break;
    }
  }

  //
  // Count down the number with lock mechanism.
  //
  InterlockedDecrement (&mNumberToFinish);
}

/**
  AP initialization after SMBASE relocation in the S3 boot path.
**/
VOID
MPRendezvousProcedure (
  VOID
  )
{
  CPU_REGISTER_TABLE         *RegisterTableList;
  UINT32                     InitApicId;
  UINTN                      Index;

  ProgramVirtualWireMode ();
  DisableLvtInterrupts ();

  RegisterTableList = (CPU_REGISTER_TABLE *) (UINTN) mAcpiCpuData.RegisterTable;
  InitApicId = GetInitialApicId ();
  for (Index = 0; Index < mAcpiCpuData.NumberOfCpus; Index++) {
    if (RegisterTableList[Index].InitialApicId == InitApicId) {
      SetProcessorRegister (&RegisterTableList[Index]);
      break;
    }
  }

  //
  // Count down the number with lock mechanism.
  //
  InterlockedDecrement (&mNumberToFinish);
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
}

/**
  The function is invoked before SMBASE relocation in S3 path to restores CPU status.

  The function is invoked before SMBASE relocation in S3 path. It does first time microcode load
  and restores MTRRs for both BSP and APs.

**/
VOID
EarlyInitializeCpu (
  VOID
  )
{
  CPU_REGISTER_TABLE         *RegisterTableList;
  UINT32                     InitApicId;
  UINTN                      Index;

  LoadMtrrData (mAcpiCpuData.MtrrTable);

  //
  // Find processor number for this CPU.
  //
  RegisterTableList = (CPU_REGISTER_TABLE *) (UINTN) mAcpiCpuData.PreSmmInitRegisterTable;
  InitApicId = GetInitialApicId ();
  for (Index = 0; Index < mAcpiCpuData.NumberOfCpus; Index++) {
    if (RegisterTableList[Index].InitialApicId == InitApicId) {
      SetProcessorRegister (&RegisterTableList[Index]);
      break;
    }
  }

  ProgramVirtualWireMode ();

  PrepareApStartupVector (mAcpiCpuData.StartupVector);

  mNumberToFinish = mAcpiCpuData.NumberOfCpus - 1;
  mExchangeInfo->ApFunction  = (VOID *) (UINTN) EarlyMPRendezvousProcedure;

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
InitializeCpu (
  VOID
  )
{
  CPU_REGISTER_TABLE         *RegisterTableList;
  UINT32                     InitApicId;
  UINTN                      Index;

  RegisterTableList = (CPU_REGISTER_TABLE *) (UINTN) mAcpiCpuData.RegisterTable;
  InitApicId = GetInitialApicId ();
  for (Index = 0; Index < mAcpiCpuData.NumberOfCpus; Index++) {
    if (RegisterTableList[Index].InitialApicId == InitApicId) {
      SetProcessorRegister (&RegisterTableList[Index]);
      break;
    }
  }

  mNumberToFinish = mAcpiCpuData.NumberOfCpus - 1;
  //
  // StackStart was updated when APs were waken up in EarlyInitializeCpu.
  // Re-initialize StackAddress to original beginning address.
  //
  mExchangeInfo->StackStart  = (VOID *) (UINTN) mAcpiCpuData.StackAddress;
  mExchangeInfo->ApFunction  = (VOID *) (UINTN) MPRendezvousProcedure;

  //
  // Send INIT IPI - SIPI to all APs
  //
  SendInitSipiSipiAllExcludingSelf ((UINT32)mAcpiCpuData.StartupVector);

  while (mNumberToFinish > 0) {
    CpuPause ();
  }
}
