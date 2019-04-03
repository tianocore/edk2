/** @file
SMM MP service implementation

Copyright (c) 2009 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuDxeSmm.h"

//
// Slots for all MTRR( FIXED MTRR + VARIABLE MTRR + MTRR_LIB_IA32_MTRR_DEF_TYPE)
//
MTRR_SETTINGS                               gSmiMtrrs;
UINT64                                      gPhyMask;
SMM_DISPATCHER_MP_SYNC_DATA                 *mSmmMpSyncData = NULL;
UINTN                                       mSmmMpSyncDataSize;
SMM_CPU_SEMAPHORES                          mSmmCpuSemaphores;
UINTN                                       mSemaphoreSize;
SPIN_LOCK                                   *mPFLock = NULL;
SMM_CPU_SYNC_MODE                           mCpuSmmSyncMode;
BOOLEAN                                     mMachineCheckSupported = FALSE;

/**
  Performs an atomic compare exchange operation to get semaphore.
  The compare exchange operation must be performed using
  MP safe mechanisms.

  @param      Sem        IN:  32-bit unsigned integer
                         OUT: original integer - 1
  @return     Original integer - 1

**/
UINT32
WaitForSemaphore (
  IN OUT  volatile UINT32           *Sem
  )
{
  UINT32                            Value;

  do {
    Value = *Sem;
  } while (Value == 0 ||
           InterlockedCompareExchange32 (
             (UINT32*)Sem,
             Value,
             Value - 1
             ) != Value);
  return Value - 1;
}


/**
  Performs an atomic compare exchange operation to release semaphore.
  The compare exchange operation must be performed using
  MP safe mechanisms.

  @param      Sem        IN:  32-bit unsigned integer
                         OUT: original integer + 1
  @return     Original integer + 1

**/
UINT32
ReleaseSemaphore (
  IN OUT  volatile UINT32           *Sem
  )
{
  UINT32                            Value;

  do {
    Value = *Sem;
  } while (Value + 1 != 0 &&
           InterlockedCompareExchange32 (
             (UINT32*)Sem,
             Value,
             Value + 1
             ) != Value);
  return Value + 1;
}

/**
  Performs an atomic compare exchange operation to lock semaphore.
  The compare exchange operation must be performed using
  MP safe mechanisms.

  @param      Sem        IN:  32-bit unsigned integer
                         OUT: -1
  @return     Original integer

**/
UINT32
LockdownSemaphore (
  IN OUT  volatile UINT32           *Sem
  )
{
  UINT32                            Value;

  do {
    Value = *Sem;
  } while (InterlockedCompareExchange32 (
             (UINT32*)Sem,
             Value, (UINT32)-1
             ) != Value);
  return Value;
}

/**
  Wait all APs to performs an atomic compare exchange operation to release semaphore.

  @param   NumberOfAPs      AP number

**/
VOID
WaitForAllAPs (
  IN      UINTN                     NumberOfAPs
  )
{
  UINTN                             BspIndex;

  BspIndex = mSmmMpSyncData->BspIndex;
  while (NumberOfAPs-- > 0) {
    WaitForSemaphore (mSmmMpSyncData->CpuData[BspIndex].Run);
  }
}

/**
  Performs an atomic compare exchange operation to release semaphore
  for each AP.

**/
VOID
ReleaseAllAPs (
  VOID
  )
{
  UINTN                             Index;
  UINTN                             BspIndex;

  BspIndex = mSmmMpSyncData->BspIndex;
  for (Index = mMaxNumberOfCpus; Index-- > 0;) {
    if (Index != BspIndex && *(mSmmMpSyncData->CpuData[Index].Present)) {
      ReleaseSemaphore (mSmmMpSyncData->CpuData[Index].Run);
    }
  }
}

/**
  Checks if all CPUs (with certain exceptions) have checked in for this SMI run

  @param   Exceptions     CPU Arrival exception flags.

  @retval   TRUE  if all CPUs the have checked in.
  @retval   FALSE  if at least one Normal AP hasn't checked in.

**/
BOOLEAN
AllCpusInSmmWithExceptions (
  SMM_CPU_ARRIVAL_EXCEPTIONS  Exceptions
  )
{
  UINTN                             Index;
  SMM_CPU_DATA_BLOCK                *CpuData;
  EFI_PROCESSOR_INFORMATION         *ProcessorInfo;

  ASSERT (*mSmmMpSyncData->Counter <= mNumberOfCpus);

  if (*mSmmMpSyncData->Counter == mNumberOfCpus) {
    return TRUE;
  }

  CpuData = mSmmMpSyncData->CpuData;
  ProcessorInfo = gSmmCpuPrivate->ProcessorInfo;
  for (Index = mMaxNumberOfCpus; Index-- > 0;) {
    if (!(*(CpuData[Index].Present)) && ProcessorInfo[Index].ProcessorId != INVALID_APIC_ID) {
      if (((Exceptions & ARRIVAL_EXCEPTION_DELAYED) != 0) && SmmCpuFeaturesGetSmmRegister (Index, SmmRegSmmDelayed) != 0) {
        continue;
      }
      if (((Exceptions & ARRIVAL_EXCEPTION_BLOCKED) != 0) && SmmCpuFeaturesGetSmmRegister (Index, SmmRegSmmBlocked) != 0) {
        continue;
      }
      if (((Exceptions & ARRIVAL_EXCEPTION_SMI_DISABLED) != 0) && SmmCpuFeaturesGetSmmRegister (Index, SmmRegSmmEnable) != 0) {
        continue;
      }
      return FALSE;
    }
  }


  return TRUE;
}

/**
  Has OS enabled Lmce in the MSR_IA32_MCG_EXT_CTL

  @retval TRUE     Os enable lmce.
  @retval FALSE    Os not enable lmce.

**/
BOOLEAN
IsLmceOsEnabled (
  VOID
  )
{
  MSR_IA32_MCG_CAP_REGISTER          McgCap;
  MSR_IA32_FEATURE_CONTROL_REGISTER  FeatureCtrl;
  MSR_IA32_MCG_EXT_CTL_REGISTER      McgExtCtrl;

  McgCap.Uint64 = AsmReadMsr64 (MSR_IA32_MCG_CAP);
  if (McgCap.Bits.MCG_LMCE_P == 0) {
    return FALSE;
  }

  FeatureCtrl.Uint64 = AsmReadMsr64 (MSR_IA32_FEATURE_CONTROL);
  if (FeatureCtrl.Bits.LmceOn == 0) {
    return FALSE;
  }

  McgExtCtrl.Uint64 = AsmReadMsr64 (MSR_IA32_MCG_EXT_CTL);
  return (BOOLEAN) (McgExtCtrl.Bits.LMCE_EN == 1);
}

/**
  Return if Local machine check exception signaled.

  Indicates (when set) that a local machine check exception was generated. This indicates that the current machine-check event was
  delivered to only the logical processor.

  @retval TRUE    LMCE was signaled.
  @retval FALSE   LMCE was not signaled.

**/
BOOLEAN
IsLmceSignaled (
  VOID
  )
{
  MSR_IA32_MCG_STATUS_REGISTER McgStatus;

  McgStatus.Uint64 = AsmReadMsr64 (MSR_IA32_MCG_STATUS);
  return (BOOLEAN) (McgStatus.Bits.LMCE_S == 1);
}

/**
  Given timeout constraint, wait for all APs to arrive, and insure when this function returns, no AP will execute normal mode code before
  entering SMM, except SMI disabled APs.

**/
VOID
SmmWaitForApArrival (
  VOID
  )
{
  UINT64                            Timer;
  UINTN                             Index;
  BOOLEAN                           LmceEn;
  BOOLEAN                           LmceSignal;

  ASSERT (*mSmmMpSyncData->Counter <= mNumberOfCpus);

  LmceEn     = FALSE;
  LmceSignal = FALSE;
  if (mMachineCheckSupported) {
    LmceEn     = IsLmceOsEnabled ();
    LmceSignal = IsLmceSignaled();
  }

  //
  // Platform implementor should choose a timeout value appropriately:
  // - The timeout value should balance the SMM time constrains and the likelihood that delayed CPUs are excluded in the SMM run. Note
  //   the SMI Handlers must ALWAYS take into account the cases that not all APs are available in an SMI run.
  // - The timeout value must, in the case of 2nd timeout, be at least long enough to give time for all APs to receive the SMI IPI
  //   and either enter SMM or buffer the SMI, to insure there is no CPU running normal mode code when SMI handling starts. This will
  //   be TRUE even if a blocked CPU is brought out of the blocked state by a normal mode CPU (before the normal mode CPU received the
  //   SMI IPI), because with a buffered SMI, and CPU will enter SMM immediately after it is brought out of the blocked state.
  // - The timeout value must be longer than longest possible IO operation in the system
  //

  //
  // Sync with APs 1st timeout
  //
  for (Timer = StartSyncTimer ();
       !IsSyncTimerTimeout (Timer) && !(LmceEn && LmceSignal) &&
       !AllCpusInSmmWithExceptions (ARRIVAL_EXCEPTION_BLOCKED | ARRIVAL_EXCEPTION_SMI_DISABLED );
       ) {
    CpuPause ();
  }

  //
  // Not all APs have arrived, so we need 2nd round of timeout. IPIs should be sent to ALL none present APs,
  // because:
  // a) Delayed AP may have just come out of the delayed state. Blocked AP may have just been brought out of blocked state by some AP running
  //    normal mode code. These APs need to be guaranteed to have an SMI pending to insure that once they are out of delayed / blocked state, they
  //    enter SMI immediately without executing instructions in normal mode. Note traditional flow requires there are no APs doing normal mode
  //    work while SMI handling is on-going.
  // b) As a consequence of SMI IPI sending, (spurious) SMI may occur after this SMM run.
  // c) ** NOTE **: Use SMI disabling feature VERY CAREFULLY (if at all) for traditional flow, because a processor in SMI-disabled state
  //    will execute normal mode code, which breaks the traditional SMI handlers' assumption that no APs are doing normal
  //    mode work while SMI handling is on-going.
  // d) We don't add code to check SMI disabling status to skip sending IPI to SMI disabled APs, because:
  //    - In traditional flow, SMI disabling is discouraged.
  //    - In relaxed flow, CheckApArrival() will check SMI disabling status before calling this function.
  //    In both cases, adding SMI-disabling checking code increases overhead.
  //
  if (*mSmmMpSyncData->Counter < mNumberOfCpus) {
    //
    // Send SMI IPIs to bring outside processors in
    //
    for (Index = mMaxNumberOfCpus; Index-- > 0;) {
      if (!(*(mSmmMpSyncData->CpuData[Index].Present)) && gSmmCpuPrivate->ProcessorInfo[Index].ProcessorId != INVALID_APIC_ID) {
        SendSmiIpi ((UINT32)gSmmCpuPrivate->ProcessorInfo[Index].ProcessorId);
      }
    }

    //
    // Sync with APs 2nd timeout.
    //
    for (Timer = StartSyncTimer ();
         !IsSyncTimerTimeout (Timer) &&
         !AllCpusInSmmWithExceptions (ARRIVAL_EXCEPTION_BLOCKED | ARRIVAL_EXCEPTION_SMI_DISABLED );
         ) {
      CpuPause ();
    }
  }

  return;
}


/**
  Replace OS MTRR's with SMI MTRR's.

  @param    CpuIndex             Processor Index

**/
VOID
ReplaceOSMtrrs (
  IN      UINTN                     CpuIndex
  )
{
  SmmCpuFeaturesDisableSmrr ();

  //
  // Replace all MTRRs registers
  //
  MtrrSetAllMtrrs (&gSmiMtrrs);
}

/**
  SMI handler for BSP.

  @param     CpuIndex         BSP processor Index
  @param     SyncMode         SMM MP sync mode

**/
VOID
BSPHandler (
  IN      UINTN                     CpuIndex,
  IN      SMM_CPU_SYNC_MODE         SyncMode
  )
{
  UINTN                             Index;
  MTRR_SETTINGS                     Mtrrs;
  UINTN                             ApCount;
  BOOLEAN                           ClearTopLevelSmiResult;
  UINTN                             PresentCount;

  ASSERT (CpuIndex == mSmmMpSyncData->BspIndex);
  ApCount = 0;

  //
  // Flag BSP's presence
  //
  *mSmmMpSyncData->InsideSmm = TRUE;

  //
  // Initialize Debug Agent to start source level debug in BSP handler
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_ENTER_SMI, NULL, NULL);

  //
  // Mark this processor's presence
  //
  *(mSmmMpSyncData->CpuData[CpuIndex].Present) = TRUE;

  //
  // Clear platform top level SMI status bit before calling SMI handlers. If
  // we cleared it after SMI handlers are run, we would miss the SMI that
  // occurs after SMI handlers are done and before SMI status bit is cleared.
  //
  ClearTopLevelSmiResult = ClearTopLevelSmiStatus();
  ASSERT (ClearTopLevelSmiResult == TRUE);

  //
  // Set running processor index
  //
  gSmmCpuPrivate->SmmCoreEntryContext.CurrentlyExecutingCpu = CpuIndex;

  //
  // If Traditional Sync Mode or need to configure MTRRs: gather all available APs.
  //
  if (SyncMode == SmmCpuSyncModeTradition || SmmCpuFeaturesNeedConfigureMtrrs()) {

    //
    // Wait for APs to arrive
    //
    SmmWaitForApArrival();

    //
    // Lock the counter down and retrieve the number of APs
    //
    *mSmmMpSyncData->AllCpusInSync = TRUE;
    ApCount = LockdownSemaphore (mSmmMpSyncData->Counter) - 1;

    //
    // Wait for all APs to get ready for programming MTRRs
    //
    WaitForAllAPs (ApCount);

    if (SmmCpuFeaturesNeedConfigureMtrrs()) {
      //
      // Signal all APs it's time for backup MTRRs
      //
      ReleaseAllAPs ();

      //
      // WaitForSemaphore() may wait for ever if an AP happens to enter SMM at
      // exactly this point. Please make sure PcdCpuSmmMaxSyncLoops has been set
      // to a large enough value to avoid this situation.
      // Note: For HT capable CPUs, threads within a core share the same set of MTRRs.
      // We do the backup first and then set MTRR to avoid race condition for threads
      // in the same core.
      //
      MtrrGetAllMtrrs(&Mtrrs);

      //
      // Wait for all APs to complete their MTRR saving
      //
      WaitForAllAPs (ApCount);

      //
      // Let all processors program SMM MTRRs together
      //
      ReleaseAllAPs ();

      //
      // WaitForSemaphore() may wait for ever if an AP happens to enter SMM at
      // exactly this point. Please make sure PcdCpuSmmMaxSyncLoops has been set
      // to a large enough value to avoid this situation.
      //
      ReplaceOSMtrrs (CpuIndex);

      //
      // Wait for all APs to complete their MTRR programming
      //
      WaitForAllAPs (ApCount);
    }
  }

  //
  // The BUSY lock is initialized to Acquired state
  //
  AcquireSpinLock (mSmmMpSyncData->CpuData[CpuIndex].Busy);

  //
  // Perform the pre tasks
  //
  PerformPreTasks ();

  //
  // Invoke SMM Foundation EntryPoint with the processor information context.
  //
  gSmmCpuPrivate->SmmCoreEntry (&gSmmCpuPrivate->SmmCoreEntryContext);

  //
  // Make sure all APs have completed their pending none-block tasks
  //
  for (Index = mMaxNumberOfCpus; Index-- > 0;) {
    if (Index != CpuIndex && *(mSmmMpSyncData->CpuData[Index].Present)) {
      AcquireSpinLock (mSmmMpSyncData->CpuData[Index].Busy);
      ReleaseSpinLock (mSmmMpSyncData->CpuData[Index].Busy);
    }
  }

  //
  // Perform the remaining tasks
  //
  PerformRemainingTasks ();

  //
  // If Relaxed-AP Sync Mode: gather all available APs after BSP SMM handlers are done, and
  // make those APs to exit SMI synchronously. APs which arrive later will be excluded and
  // will run through freely.
  //
  if (SyncMode != SmmCpuSyncModeTradition && !SmmCpuFeaturesNeedConfigureMtrrs()) {

    //
    // Lock the counter down and retrieve the number of APs
    //
    *mSmmMpSyncData->AllCpusInSync = TRUE;
    ApCount = LockdownSemaphore (mSmmMpSyncData->Counter) - 1;
    //
    // Make sure all APs have their Present flag set
    //
    while (TRUE) {
      PresentCount = 0;
      for (Index = mMaxNumberOfCpus; Index-- > 0;) {
        if (*(mSmmMpSyncData->CpuData[Index].Present)) {
          PresentCount ++;
        }
      }
      if (PresentCount > ApCount) {
        break;
      }
    }
  }

  //
  // Notify all APs to exit
  //
  *mSmmMpSyncData->InsideSmm = FALSE;
  ReleaseAllAPs ();

  //
  // Wait for all APs to complete their pending tasks
  //
  WaitForAllAPs (ApCount);

  if (SmmCpuFeaturesNeedConfigureMtrrs()) {
    //
    // Signal APs to restore MTRRs
    //
    ReleaseAllAPs ();

    //
    // Restore OS MTRRs
    //
    SmmCpuFeaturesReenableSmrr ();
    MtrrSetAllMtrrs(&Mtrrs);

    //
    // Wait for all APs to complete MTRR programming
    //
    WaitForAllAPs (ApCount);
  }

  //
  // Stop source level debug in BSP handler, the code below will not be
  // debugged.
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_EXIT_SMI, NULL, NULL);

  //
  // Signal APs to Reset states/semaphore for this processor
  //
  ReleaseAllAPs ();

  //
  // Perform pending operations for hot-plug
  //
  SmmCpuUpdate ();

  //
  // Clear the Present flag of BSP
  //
  *(mSmmMpSyncData->CpuData[CpuIndex].Present) = FALSE;

  //
  // Gather APs to exit SMM synchronously. Note the Present flag is cleared by now but
  // WaitForAllAps does not depend on the Present flag.
  //
  WaitForAllAPs (ApCount);

  //
  // Reset BspIndex to -1, meaning BSP has not been elected.
  //
  if (FeaturePcdGet (PcdCpuSmmEnableBspElection)) {
    mSmmMpSyncData->BspIndex = (UINT32)-1;
  }

  //
  // Allow APs to check in from this point on
  //
  *mSmmMpSyncData->Counter = 0;
  *mSmmMpSyncData->AllCpusInSync = FALSE;
}

/**
  SMI handler for AP.

  @param     CpuIndex         AP processor Index.
  @param     ValidSmi         Indicates that current SMI is a valid SMI or not.
  @param     SyncMode         SMM MP sync mode.

**/
VOID
APHandler (
  IN      UINTN                     CpuIndex,
  IN      BOOLEAN                   ValidSmi,
  IN      SMM_CPU_SYNC_MODE         SyncMode
  )
{
  UINT64                            Timer;
  UINTN                             BspIndex;
  MTRR_SETTINGS                     Mtrrs;

  //
  // Timeout BSP
  //
  for (Timer = StartSyncTimer ();
       !IsSyncTimerTimeout (Timer) &&
       !(*mSmmMpSyncData->InsideSmm);
       ) {
    CpuPause ();
  }

  if (!(*mSmmMpSyncData->InsideSmm)) {
    //
    // BSP timeout in the first round
    //
    if (mSmmMpSyncData->BspIndex != -1) {
      //
      // BSP Index is known
      //
      BspIndex = mSmmMpSyncData->BspIndex;
      ASSERT (CpuIndex != BspIndex);

      //
      // Send SMI IPI to bring BSP in
      //
      SendSmiIpi ((UINT32)gSmmCpuPrivate->ProcessorInfo[BspIndex].ProcessorId);

      //
      // Now clock BSP for the 2nd time
      //
      for (Timer = StartSyncTimer ();
           !IsSyncTimerTimeout (Timer) &&
           !(*mSmmMpSyncData->InsideSmm);
           ) {
        CpuPause ();
      }

      if (!(*mSmmMpSyncData->InsideSmm)) {
        //
        // Give up since BSP is unable to enter SMM
        // and signal the completion of this AP
        WaitForSemaphore (mSmmMpSyncData->Counter);
        return;
      }
    } else {
      //
      // Don't know BSP index. Give up without sending IPI to BSP.
      //
      WaitForSemaphore (mSmmMpSyncData->Counter);
      return;
    }
  }

  //
  // BSP is available
  //
  BspIndex = mSmmMpSyncData->BspIndex;
  ASSERT (CpuIndex != BspIndex);

  //
  // Mark this processor's presence
  //
  *(mSmmMpSyncData->CpuData[CpuIndex].Present) = TRUE;

  if (SyncMode == SmmCpuSyncModeTradition || SmmCpuFeaturesNeedConfigureMtrrs()) {
    //
    // Notify BSP of arrival at this point
    //
    ReleaseSemaphore (mSmmMpSyncData->CpuData[BspIndex].Run);
  }

  if (SmmCpuFeaturesNeedConfigureMtrrs()) {
    //
    // Wait for the signal from BSP to backup MTRRs
    //
    WaitForSemaphore (mSmmMpSyncData->CpuData[CpuIndex].Run);

    //
    // Backup OS MTRRs
    //
    MtrrGetAllMtrrs(&Mtrrs);

    //
    // Signal BSP the completion of this AP
    //
    ReleaseSemaphore (mSmmMpSyncData->CpuData[BspIndex].Run);

    //
    // Wait for BSP's signal to program MTRRs
    //
    WaitForSemaphore (mSmmMpSyncData->CpuData[CpuIndex].Run);

    //
    // Replace OS MTRRs with SMI MTRRs
    //
    ReplaceOSMtrrs (CpuIndex);

    //
    // Signal BSP the completion of this AP
    //
    ReleaseSemaphore (mSmmMpSyncData->CpuData[BspIndex].Run);
  }

  while (TRUE) {
    //
    // Wait for something to happen
    //
    WaitForSemaphore (mSmmMpSyncData->CpuData[CpuIndex].Run);

    //
    // Check if BSP wants to exit SMM
    //
    if (!(*mSmmMpSyncData->InsideSmm)) {
      break;
    }

    //
    // BUSY should be acquired by SmmStartupThisAp()
    //
    ASSERT (
      !AcquireSpinLockOrFail (mSmmMpSyncData->CpuData[CpuIndex].Busy)
      );

    //
    // Invoke the scheduled procedure
    //
    (*mSmmMpSyncData->CpuData[CpuIndex].Procedure) (
      (VOID*)mSmmMpSyncData->CpuData[CpuIndex].Parameter
      );

    //
    // Release BUSY
    //
    ReleaseSpinLock (mSmmMpSyncData->CpuData[CpuIndex].Busy);
  }

  if (SmmCpuFeaturesNeedConfigureMtrrs()) {
    //
    // Notify BSP the readiness of this AP to program MTRRs
    //
    ReleaseSemaphore (mSmmMpSyncData->CpuData[BspIndex].Run);

    //
    // Wait for the signal from BSP to program MTRRs
    //
    WaitForSemaphore (mSmmMpSyncData->CpuData[CpuIndex].Run);

    //
    // Restore OS MTRRs
    //
    SmmCpuFeaturesReenableSmrr ();
    MtrrSetAllMtrrs(&Mtrrs);
  }

  //
  // Notify BSP the readiness of this AP to Reset states/semaphore for this processor
  //
  ReleaseSemaphore (mSmmMpSyncData->CpuData[BspIndex].Run);

  //
  // Wait for the signal from BSP to Reset states/semaphore for this processor
  //
  WaitForSemaphore (mSmmMpSyncData->CpuData[CpuIndex].Run);

  //
  // Reset states/semaphore for this processor
  //
  *(mSmmMpSyncData->CpuData[CpuIndex].Present) = FALSE;

  //
  // Notify BSP the readiness of this AP to exit SMM
  //
  ReleaseSemaphore (mSmmMpSyncData->CpuData[BspIndex].Run);

}

/**
  Create 4G PageTable in SMRAM.

  @param[in]      Is32BitPageTable Whether the page table is 32-bit PAE
  @return         PageTable Address

**/
UINT32
Gen4GPageTable (
  IN      BOOLEAN                   Is32BitPageTable
  )
{
  VOID    *PageTable;
  UINTN   Index;
  UINT64  *Pte;
  UINTN   PagesNeeded;
  UINTN   Low2MBoundary;
  UINTN   High2MBoundary;
  UINTN   Pages;
  UINTN   GuardPage;
  UINT64  *Pdpte;
  UINTN   PageIndex;
  UINTN   PageAddress;

  Low2MBoundary = 0;
  High2MBoundary = 0;
  PagesNeeded = 0;
  if (FeaturePcdGet (PcdCpuSmmStackGuard)) {
    //
    // Add one more page for known good stack, then find the lower 2MB aligned address.
    //
    Low2MBoundary = (mSmmStackArrayBase + EFI_PAGE_SIZE) & ~(SIZE_2MB-1);
    //
    // Add two more pages for known good stack and stack guard page,
    // then find the lower 2MB aligned address.
    //
    High2MBoundary = (mSmmStackArrayEnd - mSmmStackSize + EFI_PAGE_SIZE * 2) & ~(SIZE_2MB-1);
    PagesNeeded = ((High2MBoundary - Low2MBoundary) / SIZE_2MB) + 1;
  }
  //
  // Allocate the page table
  //
  PageTable = AllocatePageTableMemory (5 + PagesNeeded);
  ASSERT (PageTable != NULL);

  PageTable = (VOID *)((UINTN)PageTable);
  Pte = (UINT64*)PageTable;

  //
  // Zero out all page table entries first
  //
  ZeroMem (Pte, EFI_PAGES_TO_SIZE (1));

  //
  // Set Page Directory Pointers
  //
  for (Index = 0; Index < 4; Index++) {
    Pte[Index] = ((UINTN)PageTable + EFI_PAGE_SIZE * (Index + 1)) | mAddressEncMask |
                   (Is32BitPageTable ? IA32_PAE_PDPTE_ATTRIBUTE_BITS : PAGE_ATTRIBUTE_BITS);
  }
  Pte += EFI_PAGE_SIZE / sizeof (*Pte);

  //
  // Fill in Page Directory Entries
  //
  for (Index = 0; Index < EFI_PAGE_SIZE * 4 / sizeof (*Pte); Index++) {
    Pte[Index] = (Index << 21) | mAddressEncMask | IA32_PG_PS | PAGE_ATTRIBUTE_BITS;
  }

  Pdpte = (UINT64*)PageTable;
  if (FeaturePcdGet (PcdCpuSmmStackGuard)) {
    Pages = (UINTN)PageTable + EFI_PAGES_TO_SIZE (5);
    GuardPage = mSmmStackArrayBase + EFI_PAGE_SIZE;
    for (PageIndex = Low2MBoundary; PageIndex <= High2MBoundary; PageIndex += SIZE_2MB) {
      Pte = (UINT64*)(UINTN)(Pdpte[BitFieldRead32 ((UINT32)PageIndex, 30, 31)] & ~mAddressEncMask & ~(EFI_PAGE_SIZE - 1));
      Pte[BitFieldRead32 ((UINT32)PageIndex, 21, 29)] = (UINT64)Pages | mAddressEncMask | PAGE_ATTRIBUTE_BITS;
      //
      // Fill in Page Table Entries
      //
      Pte = (UINT64*)Pages;
      PageAddress = PageIndex;
      for (Index = 0; Index < EFI_PAGE_SIZE / sizeof (*Pte); Index++) {
        if (PageAddress == GuardPage) {
          //
          // Mark the guard page as non-present
          //
          Pte[Index] = PageAddress | mAddressEncMask;
          GuardPage += mSmmStackSize;
          if (GuardPage > mSmmStackArrayEnd) {
            GuardPage = 0;
          }
        } else {
          Pte[Index] = PageAddress | mAddressEncMask | PAGE_ATTRIBUTE_BITS;
        }
        PageAddress+= EFI_PAGE_SIZE;
      }
      Pages += EFI_PAGE_SIZE;
    }
  }

  if ((PcdGet8 (PcdNullPointerDetectionPropertyMask) & BIT1) != 0) {
    Pte = (UINT64*)(UINTN)(Pdpte[0] & ~mAddressEncMask & ~(EFI_PAGE_SIZE - 1));
    if ((Pte[0] & IA32_PG_PS) == 0) {
      // 4K-page entries are already mapped. Just hide the first one anyway.
      Pte = (UINT64*)(UINTN)(Pte[0] & ~mAddressEncMask & ~(EFI_PAGE_SIZE - 1));
      Pte[0] &= ~(UINT64)IA32_PG_P; // Hide page 0
    } else {
      // Create 4K-page entries
      Pages = (UINTN)AllocatePageTableMemory (1);
      ASSERT (Pages != 0);

      Pte[0] = (UINT64)(Pages | mAddressEncMask | PAGE_ATTRIBUTE_BITS);

      Pte = (UINT64*)Pages;
      PageAddress = 0;
      Pte[0] = PageAddress | mAddressEncMask; // Hide page 0 but present left
      for (Index = 1; Index < EFI_PAGE_SIZE / sizeof (*Pte); Index++) {
        PageAddress += EFI_PAGE_SIZE;
        Pte[Index] = PageAddress | mAddressEncMask | PAGE_ATTRIBUTE_BITS;
      }
    }
  }

  return (UINT32)(UINTN)PageTable;
}

/**
  Schedule a procedure to run on the specified CPU.

  @param[in]       Procedure                The address of the procedure to run
  @param[in]       CpuIndex                 Target CPU Index
  @param[in, out]  ProcArguments            The parameter to pass to the procedure
  @param[in]       BlockingMode             Startup AP in blocking mode or not

  @retval EFI_INVALID_PARAMETER    CpuNumber not valid
  @retval EFI_INVALID_PARAMETER    CpuNumber specifying BSP
  @retval EFI_INVALID_PARAMETER    The AP specified by CpuNumber did not enter SMM
  @retval EFI_INVALID_PARAMETER    The AP specified by CpuNumber is busy
  @retval EFI_SUCCESS              The procedure has been successfully scheduled

**/
EFI_STATUS
InternalSmmStartupThisAp (
  IN      EFI_AP_PROCEDURE          Procedure,
  IN      UINTN                     CpuIndex,
  IN OUT  VOID                      *ProcArguments OPTIONAL,
  IN      BOOLEAN                   BlockingMode
  )
{
  if (CpuIndex >= gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus) {
    DEBUG((DEBUG_ERROR, "CpuIndex(%d) >= gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus(%d)\n", CpuIndex, gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus));
    return EFI_INVALID_PARAMETER;
  }
  if (CpuIndex == gSmmCpuPrivate->SmmCoreEntryContext.CurrentlyExecutingCpu) {
    DEBUG((DEBUG_ERROR, "CpuIndex(%d) == gSmmCpuPrivate->SmmCoreEntryContext.CurrentlyExecutingCpu\n", CpuIndex));
    return EFI_INVALID_PARAMETER;
  }
  if (gSmmCpuPrivate->ProcessorInfo[CpuIndex].ProcessorId == INVALID_APIC_ID) {
    return EFI_INVALID_PARAMETER;
  }
  if (!(*(mSmmMpSyncData->CpuData[CpuIndex].Present))) {
    if (mSmmMpSyncData->EffectiveSyncMode == SmmCpuSyncModeTradition) {
      DEBUG((DEBUG_ERROR, "!mSmmMpSyncData->CpuData[%d].Present\n", CpuIndex));
    }
    return EFI_INVALID_PARAMETER;
  }
  if (gSmmCpuPrivate->Operation[CpuIndex] == SmmCpuRemove) {
    if (!FeaturePcdGet (PcdCpuHotPlugSupport)) {
      DEBUG((DEBUG_ERROR, "gSmmCpuPrivate->Operation[%d] == SmmCpuRemove\n", CpuIndex));
    }
    return EFI_INVALID_PARAMETER;
  }

  if (BlockingMode) {
    AcquireSpinLock (mSmmMpSyncData->CpuData[CpuIndex].Busy);
  } else {
    if (!AcquireSpinLockOrFail (mSmmMpSyncData->CpuData[CpuIndex].Busy)) {
      DEBUG((DEBUG_ERROR, "mSmmMpSyncData->CpuData[%d].Busy\n", CpuIndex));
      return EFI_INVALID_PARAMETER;
    }
  }

  mSmmMpSyncData->CpuData[CpuIndex].Procedure = Procedure;
  mSmmMpSyncData->CpuData[CpuIndex].Parameter = ProcArguments;
  ReleaseSemaphore (mSmmMpSyncData->CpuData[CpuIndex].Run);

  if (BlockingMode) {
    AcquireSpinLock (mSmmMpSyncData->CpuData[CpuIndex].Busy);
    ReleaseSpinLock (mSmmMpSyncData->CpuData[CpuIndex].Busy);
  }
  return EFI_SUCCESS;
}

/**
  Schedule a procedure to run on the specified CPU in blocking mode.

  @param[in]       Procedure                The address of the procedure to run
  @param[in]       CpuIndex                 Target CPU Index
  @param[in, out]  ProcArguments            The parameter to pass to the procedure

  @retval EFI_INVALID_PARAMETER    CpuNumber not valid
  @retval EFI_INVALID_PARAMETER    CpuNumber specifying BSP
  @retval EFI_INVALID_PARAMETER    The AP specified by CpuNumber did not enter SMM
  @retval EFI_INVALID_PARAMETER    The AP specified by CpuNumber is busy
  @retval EFI_SUCCESS              The procedure has been successfully scheduled

**/
EFI_STATUS
EFIAPI
SmmBlockingStartupThisAp (
  IN      EFI_AP_PROCEDURE          Procedure,
  IN      UINTN                     CpuIndex,
  IN OUT  VOID                      *ProcArguments OPTIONAL
  )
{
  return InternalSmmStartupThisAp(Procedure, CpuIndex, ProcArguments, TRUE);
}

/**
  Schedule a procedure to run on the specified CPU.

  @param  Procedure                The address of the procedure to run
  @param  CpuIndex                 Target CPU Index
  @param  ProcArguments            The parameter to pass to the procedure

  @retval EFI_INVALID_PARAMETER    CpuNumber not valid
  @retval EFI_INVALID_PARAMETER    CpuNumber specifying BSP
  @retval EFI_INVALID_PARAMETER    The AP specified by CpuNumber did not enter SMM
  @retval EFI_INVALID_PARAMETER    The AP specified by CpuNumber is busy
  @retval EFI_SUCCESS              The procedure has been successfully scheduled

**/
EFI_STATUS
EFIAPI
SmmStartupThisAp (
  IN      EFI_AP_PROCEDURE          Procedure,
  IN      UINTN                     CpuIndex,
  IN OUT  VOID                      *ProcArguments OPTIONAL
  )
{
  return InternalSmmStartupThisAp(Procedure, CpuIndex, ProcArguments, FeaturePcdGet (PcdCpuSmmBlockStartupThisAp));
}

/**
  This function sets DR6 & DR7 according to SMM save state, before running SMM C code.
  They are useful when you want to enable hardware breakpoints in SMM without entry SMM mode.

  NOTE: It might not be appreciated in runtime since it might
        conflict with OS debugging facilities. Turn them off in RELEASE.

  @param    CpuIndex              CPU Index

**/
VOID
EFIAPI
CpuSmmDebugEntry (
  IN UINTN  CpuIndex
  )
{
  SMRAM_SAVE_STATE_MAP *CpuSaveState;

  if (FeaturePcdGet (PcdCpuSmmDebug)) {
    ASSERT(CpuIndex < mMaxNumberOfCpus);
    CpuSaveState = (SMRAM_SAVE_STATE_MAP *)gSmmCpuPrivate->CpuSaveState[CpuIndex];
    if (mSmmSaveStateRegisterLma == EFI_SMM_SAVE_STATE_REGISTER_LMA_32BIT) {
      AsmWriteDr6 (CpuSaveState->x86._DR6);
      AsmWriteDr7 (CpuSaveState->x86._DR7);
    } else {
      AsmWriteDr6 ((UINTN)CpuSaveState->x64._DR6);
      AsmWriteDr7 ((UINTN)CpuSaveState->x64._DR7);
    }
  }
}

/**
  This function restores DR6 & DR7 to SMM save state.

  NOTE: It might not be appreciated in runtime since it might
        conflict with OS debugging facilities. Turn them off in RELEASE.

  @param    CpuIndex              CPU Index

**/
VOID
EFIAPI
CpuSmmDebugExit (
  IN UINTN  CpuIndex
  )
{
  SMRAM_SAVE_STATE_MAP *CpuSaveState;

  if (FeaturePcdGet (PcdCpuSmmDebug)) {
    ASSERT(CpuIndex < mMaxNumberOfCpus);
    CpuSaveState = (SMRAM_SAVE_STATE_MAP *)gSmmCpuPrivate->CpuSaveState[CpuIndex];
    if (mSmmSaveStateRegisterLma == EFI_SMM_SAVE_STATE_REGISTER_LMA_32BIT) {
      CpuSaveState->x86._DR7 = (UINT32)AsmReadDr7 ();
      CpuSaveState->x86._DR6 = (UINT32)AsmReadDr6 ();
    } else {
      CpuSaveState->x64._DR7 = AsmReadDr7 ();
      CpuSaveState->x64._DR6 = AsmReadDr6 ();
    }
  }
}

/**
  C function for SMI entry, each processor comes here upon SMI trigger.

  @param    CpuIndex              CPU Index

**/
VOID
EFIAPI
SmiRendezvous (
  IN      UINTN                     CpuIndex
  )
{
  EFI_STATUS                     Status;
  BOOLEAN                        ValidSmi;
  BOOLEAN                        IsBsp;
  BOOLEAN                        BspInProgress;
  UINTN                          Index;
  UINTN                          Cr2;

  ASSERT(CpuIndex < mMaxNumberOfCpus);

  //
  // Save Cr2 because Page Fault exception in SMM may override its value,
  // when using on-demand paging for above 4G memory.
  //
  Cr2 = 0;
  SaveCr2 (&Cr2);

  //
  // Perform CPU specific entry hooks
  //
  SmmCpuFeaturesRendezvousEntry (CpuIndex);

  //
  // Determine if this is a valid SMI
  //
  ValidSmi = PlatformValidSmi();

  //
  // Determine if BSP has been already in progress. Note this must be checked after
  // ValidSmi because BSP may clear a valid SMI source after checking in.
  //
  BspInProgress = *mSmmMpSyncData->InsideSmm;

  if (!BspInProgress && !ValidSmi) {
    //
    // If we reach here, it means when we sampled the ValidSmi flag, SMI status had not
    // been cleared by BSP in a new SMI run (so we have a truly invalid SMI), or SMI
    // status had been cleared by BSP and an existing SMI run has almost ended. (Note
    // we sampled ValidSmi flag BEFORE judging BSP-in-progress status.) In both cases, there
    // is nothing we need to do.
    //
    goto Exit;
  } else {
    //
    // Signal presence of this processor
    //
    if (ReleaseSemaphore (mSmmMpSyncData->Counter) == 0) {
      //
      // BSP has already ended the synchronization, so QUIT!!!
      //

      //
      // Wait for BSP's signal to finish SMI
      //
      while (*mSmmMpSyncData->AllCpusInSync) {
        CpuPause ();
      }
      goto Exit;
    } else {

      //
      // The BUSY lock is initialized to Released state.
      // This needs to be done early enough to be ready for BSP's SmmStartupThisAp() call.
      // E.g., with Relaxed AP flow, SmmStartupThisAp() may be called immediately
      // after AP's present flag is detected.
      //
      InitializeSpinLock (mSmmMpSyncData->CpuData[CpuIndex].Busy);
    }

    if (FeaturePcdGet (PcdCpuSmmProfileEnable)) {
      ActivateSmmProfile (CpuIndex);
    }

    if (BspInProgress) {
      //
      // BSP has been elected. Follow AP path, regardless of ValidSmi flag
      // as BSP may have cleared the SMI status
      //
      APHandler (CpuIndex, ValidSmi, mSmmMpSyncData->EffectiveSyncMode);
    } else {
      //
      // We have a valid SMI
      //

      //
      // Elect BSP
      //
      IsBsp = FALSE;
      if (FeaturePcdGet (PcdCpuSmmEnableBspElection)) {
        if (!mSmmMpSyncData->SwitchBsp || mSmmMpSyncData->CandidateBsp[CpuIndex]) {
          //
          // Call platform hook to do BSP election
          //
          Status = PlatformSmmBspElection (&IsBsp);
          if (EFI_SUCCESS == Status) {
            //
            // Platform hook determines successfully
            //
            if (IsBsp) {
              mSmmMpSyncData->BspIndex = (UINT32)CpuIndex;
            }
          } else {
            //
            // Platform hook fails to determine, use default BSP election method
            //
            InterlockedCompareExchange32 (
              (UINT32*)&mSmmMpSyncData->BspIndex,
              (UINT32)-1,
              (UINT32)CpuIndex
              );
          }
        }
      }

      //
      // "mSmmMpSyncData->BspIndex == CpuIndex" means this is the BSP
      //
      if (mSmmMpSyncData->BspIndex == CpuIndex) {

        //
        // Clear last request for SwitchBsp.
        //
        if (mSmmMpSyncData->SwitchBsp) {
          mSmmMpSyncData->SwitchBsp = FALSE;
          for (Index = 0; Index < mMaxNumberOfCpus; Index++) {
            mSmmMpSyncData->CandidateBsp[Index] = FALSE;
          }
        }

        if (FeaturePcdGet (PcdCpuSmmProfileEnable)) {
          SmmProfileRecordSmiNum ();
        }

        //
        // BSP Handler is always called with a ValidSmi == TRUE
        //
        BSPHandler (CpuIndex, mSmmMpSyncData->EffectiveSyncMode);
      } else {
        APHandler (CpuIndex, ValidSmi, mSmmMpSyncData->EffectiveSyncMode);
      }
    }

    ASSERT (*mSmmMpSyncData->CpuData[CpuIndex].Run == 0);

    //
    // Wait for BSP's signal to exit SMI
    //
    while (*mSmmMpSyncData->AllCpusInSync) {
      CpuPause ();
    }
  }

Exit:
  SmmCpuFeaturesRendezvousExit (CpuIndex);

  //
  // Restore Cr2
  //
  RestoreCr2 (Cr2);
}

/**
  Allocate buffer for all semaphores and spin locks.

**/
VOID
InitializeSmmCpuSemaphores (
  VOID
  )
{
  UINTN                      ProcessorCount;
  UINTN                      TotalSize;
  UINTN                      GlobalSemaphoresSize;
  UINTN                      CpuSemaphoresSize;
  UINTN                      SemaphoreSize;
  UINTN                      Pages;
  UINTN                      *SemaphoreBlock;
  UINTN                      SemaphoreAddr;

  SemaphoreSize   = GetSpinLockProperties ();
  ProcessorCount = gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus;
  GlobalSemaphoresSize = (sizeof (SMM_CPU_SEMAPHORE_GLOBAL) / sizeof (VOID *)) * SemaphoreSize;
  CpuSemaphoresSize    = (sizeof (SMM_CPU_SEMAPHORE_CPU) / sizeof (VOID *)) * ProcessorCount * SemaphoreSize;
  TotalSize = GlobalSemaphoresSize + CpuSemaphoresSize;
  DEBUG((EFI_D_INFO, "One Semaphore Size    = 0x%x\n", SemaphoreSize));
  DEBUG((EFI_D_INFO, "Total Semaphores Size = 0x%x\n", TotalSize));
  Pages = EFI_SIZE_TO_PAGES (TotalSize);
  SemaphoreBlock = AllocatePages (Pages);
  ASSERT (SemaphoreBlock != NULL);
  ZeroMem (SemaphoreBlock, TotalSize);

  SemaphoreAddr = (UINTN)SemaphoreBlock;
  mSmmCpuSemaphores.SemaphoreGlobal.Counter       = (UINT32 *)SemaphoreAddr;
  SemaphoreAddr += SemaphoreSize;
  mSmmCpuSemaphores.SemaphoreGlobal.InsideSmm     = (BOOLEAN *)SemaphoreAddr;
  SemaphoreAddr += SemaphoreSize;
  mSmmCpuSemaphores.SemaphoreGlobal.AllCpusInSync = (BOOLEAN *)SemaphoreAddr;
  SemaphoreAddr += SemaphoreSize;
  mSmmCpuSemaphores.SemaphoreGlobal.PFLock        = (SPIN_LOCK *)SemaphoreAddr;
  SemaphoreAddr += SemaphoreSize;
  mSmmCpuSemaphores.SemaphoreGlobal.CodeAccessCheckLock
                                                  = (SPIN_LOCK *)SemaphoreAddr;
  SemaphoreAddr += SemaphoreSize;

  SemaphoreAddr = (UINTN)SemaphoreBlock + GlobalSemaphoresSize;
  mSmmCpuSemaphores.SemaphoreCpu.Busy    = (SPIN_LOCK *)SemaphoreAddr;
  SemaphoreAddr += ProcessorCount * SemaphoreSize;
  mSmmCpuSemaphores.SemaphoreCpu.Run     = (UINT32 *)SemaphoreAddr;
  SemaphoreAddr += ProcessorCount * SemaphoreSize;
  mSmmCpuSemaphores.SemaphoreCpu.Present = (BOOLEAN *)SemaphoreAddr;

  mPFLock                       = mSmmCpuSemaphores.SemaphoreGlobal.PFLock;
  mConfigSmmCodeAccessCheckLock = mSmmCpuSemaphores.SemaphoreGlobal.CodeAccessCheckLock;

  mSemaphoreSize = SemaphoreSize;
}

/**
  Initialize un-cacheable data.

**/
VOID
EFIAPI
InitializeMpSyncData (
  VOID
  )
{
  UINTN                      CpuIndex;

  if (mSmmMpSyncData != NULL) {
    //
    // mSmmMpSyncDataSize includes one structure of SMM_DISPATCHER_MP_SYNC_DATA, one
    // CpuData array of SMM_CPU_DATA_BLOCK and one CandidateBsp array of BOOLEAN.
    //
    ZeroMem (mSmmMpSyncData, mSmmMpSyncDataSize);
    mSmmMpSyncData->CpuData = (SMM_CPU_DATA_BLOCK *)((UINT8 *)mSmmMpSyncData + sizeof (SMM_DISPATCHER_MP_SYNC_DATA));
    mSmmMpSyncData->CandidateBsp = (BOOLEAN *)(mSmmMpSyncData->CpuData + gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus);
    if (FeaturePcdGet (PcdCpuSmmEnableBspElection)) {
      //
      // Enable BSP election by setting BspIndex to -1
      //
      mSmmMpSyncData->BspIndex = (UINT32)-1;
    }
    mSmmMpSyncData->EffectiveSyncMode = mCpuSmmSyncMode;

    mSmmMpSyncData->Counter       = mSmmCpuSemaphores.SemaphoreGlobal.Counter;
    mSmmMpSyncData->InsideSmm     = mSmmCpuSemaphores.SemaphoreGlobal.InsideSmm;
    mSmmMpSyncData->AllCpusInSync = mSmmCpuSemaphores.SemaphoreGlobal.AllCpusInSync;
    ASSERT (mSmmMpSyncData->Counter != NULL && mSmmMpSyncData->InsideSmm != NULL &&
            mSmmMpSyncData->AllCpusInSync != NULL);
    *mSmmMpSyncData->Counter       = 0;
    *mSmmMpSyncData->InsideSmm     = FALSE;
    *mSmmMpSyncData->AllCpusInSync = FALSE;

    for (CpuIndex = 0; CpuIndex < gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus; CpuIndex ++) {
      mSmmMpSyncData->CpuData[CpuIndex].Busy    =
        (SPIN_LOCK *)((UINTN)mSmmCpuSemaphores.SemaphoreCpu.Busy + mSemaphoreSize * CpuIndex);
      mSmmMpSyncData->CpuData[CpuIndex].Run     =
        (UINT32 *)((UINTN)mSmmCpuSemaphores.SemaphoreCpu.Run + mSemaphoreSize * CpuIndex);
      mSmmMpSyncData->CpuData[CpuIndex].Present =
        (BOOLEAN *)((UINTN)mSmmCpuSemaphores.SemaphoreCpu.Present + mSemaphoreSize * CpuIndex);
      *(mSmmMpSyncData->CpuData[CpuIndex].Busy)    = 0;
      *(mSmmMpSyncData->CpuData[CpuIndex].Run)     = 0;
      *(mSmmMpSyncData->CpuData[CpuIndex].Present) = FALSE;
    }
  }
}

/**
  Initialize global data for MP synchronization.

  @param Stacks             Base address of SMI stack buffer for all processors.
  @param StackSize          Stack size for each processor in SMM.
  @param ShadowStackSize    Shadow Stack size for each processor in SMM.

**/
UINT32
InitializeMpServiceData (
  IN VOID        *Stacks,
  IN UINTN       StackSize,
  IN UINTN       ShadowStackSize
  )
{
  UINT32                    Cr3;
  UINTN                     Index;
  UINT8                     *GdtTssTables;
  UINTN                     GdtTableStepSize;
  CPUID_VERSION_INFO_EDX    RegEdx;

  //
  // Determine if this CPU supports machine check
  //
  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, &RegEdx.Uint32);
  mMachineCheckSupported = (BOOLEAN)(RegEdx.Bits.MCA == 1);

  //
  // Allocate memory for all locks and semaphores
  //
  InitializeSmmCpuSemaphores ();

  //
  // Initialize mSmmMpSyncData
  //
  mSmmMpSyncDataSize = sizeof (SMM_DISPATCHER_MP_SYNC_DATA) +
                       (sizeof (SMM_CPU_DATA_BLOCK) + sizeof (BOOLEAN)) * gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus;
  mSmmMpSyncData = (SMM_DISPATCHER_MP_SYNC_DATA*) AllocatePages (EFI_SIZE_TO_PAGES (mSmmMpSyncDataSize));
  ASSERT (mSmmMpSyncData != NULL);
  mCpuSmmSyncMode = (SMM_CPU_SYNC_MODE)PcdGet8 (PcdCpuSmmSyncMode);
  InitializeMpSyncData ();

  //
  // Initialize physical address mask
  // NOTE: Physical memory above virtual address limit is not supported !!!
  //
  AsmCpuid (0x80000008, (UINT32*)&Index, NULL, NULL, NULL);
  gPhyMask = LShiftU64 (1, (UINT8)Index) - 1;
  gPhyMask &= (1ull << 48) - EFI_PAGE_SIZE;

  //
  // Create page tables
  //
  Cr3 = SmmInitPageTable ();

  GdtTssTables = InitGdt (Cr3, &GdtTableStepSize);

  //
  // Install SMI handler for each CPU
  //
  for (Index = 0; Index < mMaxNumberOfCpus; Index++) {
    InstallSmiHandler (
      Index,
      (UINT32)mCpuHotPlugData.SmBase[Index],
      (VOID*)((UINTN)Stacks + (StackSize + ShadowStackSize) * Index),
      StackSize,
      (UINTN)(GdtTssTables + GdtTableStepSize * Index),
      gcSmiGdtr.Limit + 1,
      gcSmiIdtr.Base,
      gcSmiIdtr.Limit + 1,
      Cr3
      );
  }

  //
  // Record current MTRR settings
  //
  ZeroMem (&gSmiMtrrs, sizeof (gSmiMtrrs));
  MtrrGetAllMtrrs (&gSmiMtrrs);

  return Cr3;
}

/**

  Register the SMM Foundation entry point.

  @param          This              Pointer to EFI_SMM_CONFIGURATION_PROTOCOL instance
  @param          SmmEntryPoint     SMM Foundation EntryPoint

  @retval         EFI_SUCCESS       Successfully to register SMM foundation entry point

**/
EFI_STATUS
EFIAPI
RegisterSmmEntry (
  IN CONST EFI_SMM_CONFIGURATION_PROTOCOL  *This,
  IN EFI_SMM_ENTRY_POINT                   SmmEntryPoint
  )
{
  //
  // Record SMM Foundation EntryPoint, later invoke it on SMI entry vector.
  //
  gSmmCpuPrivate->SmmCoreEntry = SmmEntryPoint;
  return EFI_SUCCESS;
}
