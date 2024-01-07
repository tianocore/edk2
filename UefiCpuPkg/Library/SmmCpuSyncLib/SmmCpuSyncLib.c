/** @file
  SMM CPU Sync lib implementation.

  The lib provides 3 sets of APIs:
  1. ContextInit/ContextDeinit/ContextReset:

    ContextInit() is called in driver's entrypoint to allocate and initialize the SMM CPU Sync context.
    ContextDeinit() is called in driver's unload function to deinitialize the SMM CPU Sync context.
    ContextReset() is called by one of CPUs after all CPUs are ready to exit SMI, which allows CPU to
    check into the next SMI from this point.

  2. GetArrivedCpuCount/CheckInCpu/CheckOutCpu/LockDoor:
    When SMI happens, all processors including BSP enter to SMM mode by calling CheckInCpu().
    CheckOutCpu() can be called in error handling flow for the CPU who calls CheckInCpu() earlier.
    The elected BSP calls LockDoor() so that CheckInCpu() and CheckOutCpu() will return the error code after that.
    GetArrivedCpuCount() returns the number of checked-in CPUs.

  3. WaitForAPs/ReleaseOneAp/WaitForBsp/ReleaseBsp
    WaitForAPs() & ReleaseOneAp() are called from BSP to wait the number of APs and release one specific AP.
    WaitForBsp() & ReleaseBsp() are called from APs to wait and release BSP.
    The 4 APIs are used to synchronize the running flow among BSP and APs.
    BSP and AP Sync flow can be easy understand as below:
    BSP: ReleaseOneAp  -->  AP: WaitForBsp
    BSP: WaitForAPs    <--  AP: ReleaseBsp

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SafeIntLib.h>
#include <Library/SmmCpuSyncLib.h>
#include <Library/SynchronizationLib.h>
#include <Uefi.h>

///
/// The implementation shall place one semaphore on exclusive cache line for good performance.
///
typedef volatile UINT32 SMM_CPU_SYNC_SEMAPHORE;

typedef struct {
  ///
  /// Used for control each CPU continue run or wait for signal
  ///
  SMM_CPU_SYNC_SEMAPHORE    *Run;
} SMM_CPU_SYNC_SEMAPHORE_FOR_EACH_CPU;

struct SMM_CPU_SYNC_CONTEXT  {
  ///
  /// Indicate all CPUs in the system.
  ///
  UINTN                                  NumberOfCpus;
  ///
  /// Address of semaphores.
  ///
  VOID                                   *SemBuffer;
  ///
  /// Size of semaphores.
  ///
  UINTN                                  SemBufferPages;
  ///
  /// Before the door is locked, CpuCount stores the arrived CPU count.
  /// After the door is locked, CpuCount is set to -1 indicating the door is locked.
  /// ArrivedCpuCountUponLock stores the arrived CPU count then.
  ///
  UINTN                                  ArrivedCpuCountUponLock;
  ///
  /// Indicate CPUs entered SMM before lock door.
  ///
  SMM_CPU_SYNC_SEMAPHORE                 *CpuCount;
  ///
  /// Define an array of structure for each CPU semaphore due to the size alignment
  /// requirement. With the array of structure for each CPU semaphore, it's easy to
  /// reach the specific CPU with CPU Index for its own semaphore access: CpuSem[CpuIndex].
  ///
  SMM_CPU_SYNC_SEMAPHORE_FOR_EACH_CPU    CpuSem[];
};

/**
  Performs an atomic compare exchange operation to get semaphore.
  The compare exchange operation must be performed using MP safe
  mechanisms.

  @param[in,out]  Sem    IN:  32-bit unsigned integer
                         OUT: original integer - 1 if Sem is not locked.
                         OUT: MAX_UINT32 if Sem is locked.

  @retval     Original integer - 1 if Sem is not locked.
              MAX_UINT32 if Sem is locked.

**/
STATIC
UINT32
InternalWaitForSemaphore (
  IN OUT  volatile UINT32  *Sem
  )
{
  UINT32  Value;

  for ( ; ;) {
    Value = *Sem;
    if (Value == MAX_UINT32) {
      return Value;
    }

    if ((Value != 0) &&
        (InterlockedCompareExchange32 (
           (UINT32 *)Sem,
           Value,
           Value - 1
           ) == Value))
    {
      break;
    }

    CpuPause ();
  }

  return Value - 1;
}

/**
  Performs an atomic compare exchange operation to release semaphore.
  The compare exchange operation must be performed using MP safe
  mechanisms.

  @param[in,out]  Sem    IN:  32-bit unsigned integer
                         OUT: original integer + 1 if Sem is not locked.
                         OUT: MAX_UINT32 if Sem is locked.

  @retval    Original integer + 1 if Sem is not locked.
             MAX_UINT32 if Sem is locked.

**/
STATIC
UINT32
InternalReleaseSemaphore (
  IN OUT  volatile UINT32  *Sem
  )
{
  UINT32  Value;

  do {
    Value = *Sem;
  } while (Value + 1 != 0 &&
           InterlockedCompareExchange32 (
             (UINT32 *)Sem,
             Value,
             Value + 1
             ) != Value);

  if (Value == MAX_UINT32) {
    return Value;
  }

  return Value + 1;
}

/**
  Performs an atomic compare exchange operation to lock semaphore.
  The compare exchange operation must be performed using MP safe
  mechanisms.

  @param[in,out]  Sem    IN:  32-bit unsigned integer
                         OUT: -1

  @retval    Original integer

**/
STATIC
UINT32
InternalLockdownSemaphore (
  IN OUT  volatile UINT32  *Sem
  )
{
  UINT32  Value;

  do {
    Value = *Sem;
  } while (InterlockedCompareExchange32 (
             (UINT32 *)Sem,
             Value,
             (UINT32)-1
             ) != Value);

  return Value;
}

/**
  Create and initialize the SMM CPU Sync context. It is to allocate and initialize the
  SMM CPU Sync context.

  If Context is NULL, then ASSERT().

  @param[in]  NumberOfCpus          The number of Logical Processors in the system.
  @param[out] Context               Pointer to the new created and initialized SMM CPU Sync context object.
                                    NULL will be returned if any error happen during init.

  @retval RETURN_SUCCESS            The SMM CPU Sync context was successful created and initialized.
  @retval RETURN_OUT_OF_RESOURCES   There are not enough resources available to create and initialize SMM CPU Sync context.
  @retval RETURN_BUFFER_TOO_SMALL   Overflow happen

**/
RETURN_STATUS
EFIAPI
SmmCpuSyncContextInit (
  IN   UINTN                 NumberOfCpus,
  OUT  SMM_CPU_SYNC_CONTEXT  **Context
  )
{
  RETURN_STATUS                        Status;
  UINTN                                ContextSize;
  UINTN                                OneSemSize;
  UINTN                                NumSem;
  UINTN                                TotalSemSize;
  UINTN                                SemAddr;
  UINTN                                CpuIndex;
  SMM_CPU_SYNC_SEMAPHORE_FOR_EACH_CPU  *CpuSem;

  ASSERT (Context != NULL);

  //
  // Calculate ContextSize
  //
  Status = SafeUintnMult (NumberOfCpus, sizeof (SMM_CPU_SYNC_SEMAPHORE_FOR_EACH_CPU), &ContextSize);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Status = SafeUintnAdd (ContextSize, sizeof (SMM_CPU_SYNC_CONTEXT), &ContextSize);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  //
  // Allocate Buffer for Context
  //
  *Context = AllocatePool (ContextSize);
  if (*Context == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  (*Context)->ArrivedCpuCountUponLock = 0;

  //
  // Save NumberOfCpus
  //
  (*Context)->NumberOfCpus = NumberOfCpus;

  //
  // Calculate total semaphore size
  //
  OneSemSize = GetSpinLockProperties ();
  ASSERT (sizeof (SMM_CPU_SYNC_SEMAPHORE) <= OneSemSize);

  Status = SafeUintnAdd (1, NumberOfCpus, &NumSem);
  if (RETURN_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = SafeUintnMult (NumSem, OneSemSize, &TotalSemSize);
  if (RETURN_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Allocate for Semaphores in the *Context
  //
  (*Context)->SemBufferPages = EFI_SIZE_TO_PAGES (TotalSemSize);
  (*Context)->SemBuffer      = AllocatePages ((*Context)->SemBufferPages);
  if ((*Context)->SemBuffer == NULL) {
    Status = RETURN_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  //
  // Assign Global Semaphore pointer
  //
  SemAddr               = (UINTN)(*Context)->SemBuffer;
  (*Context)->CpuCount  = (SMM_CPU_SYNC_SEMAPHORE *)SemAddr;
  *(*Context)->CpuCount = 0;

  SemAddr += OneSemSize;

  //
  // Assign CPU Semaphore pointer
  //
  CpuSem = (*Context)->CpuSem;
  for (CpuIndex = 0; CpuIndex < NumberOfCpus; CpuIndex++) {
    CpuSem->Run  = (SMM_CPU_SYNC_SEMAPHORE *)SemAddr;
    *CpuSem->Run = 0;

    CpuSem++;
    SemAddr += OneSemSize;
  }

  return RETURN_SUCCESS;

ON_ERROR:
  FreePool (*Context);
  return Status;
}

/**
  Deinit an allocated SMM CPU Sync context. The resources allocated in SmmCpuSyncContextInit() will
  be freed.

  If Context is NULL, then ASSERT().

  @param[in,out]  Context     Pointer to the SMM CPU Sync context object to be deinitialized.

**/
VOID
EFIAPI
SmmCpuSyncContextDeinit (
  IN OUT SMM_CPU_SYNC_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);

  FreePages (Context->SemBuffer, Context->SemBufferPages);

  FreePool (Context);
}

/**
  Reset SMM CPU Sync context. SMM CPU Sync context will be reset to the initialized state.

  This function is called by one of CPUs after all CPUs are ready to exit SMI, which allows CPU to
  check into the next SMI from this point.

  If Context is NULL, then ASSERT().

  @param[in,out]  Context     Pointer to the SMM CPU Sync context object to be reset.

**/
VOID
EFIAPI
SmmCpuSyncContextReset (
  IN OUT SMM_CPU_SYNC_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);

  Context->ArrivedCpuCountUponLock = 0;
  *Context->CpuCount               = 0;
}

/**
  Get current number of arrived CPU in SMI.

  BSP might need to know the current number of arrived CPU in SMI to make sure all APs
  in SMI. This API can be for that purpose.

  If Context is NULL, then ASSERT().

  @param[in]      Context     Pointer to the SMM CPU Sync context object.

  @retval    Current number of arrived CPU in SMI.

**/
UINTN
EFIAPI
SmmCpuSyncGetArrivedCpuCount (
  IN  SMM_CPU_SYNC_CONTEXT  *Context
  )
{
  UINT32  Value;

  ASSERT (Context != NULL);

  Value = *Context->CpuCount;

  if (Value == (UINT32)-1) {
    return Context->ArrivedCpuCountUponLock;
  }

  return Value;
}

/**
  Performs an atomic operation to check in CPU.

  When SMI happens, all processors including BSP enter to SMM mode by calling SmmCpuSyncCheckInCpu().

  If Context is NULL, then ASSERT().
  If CpuIndex exceeds the range of all CPUs in the system, then ASSERT().

  @param[in,out]  Context           Pointer to the SMM CPU Sync context object.
  @param[in]      CpuIndex          Check in CPU index.

  @retval RETURN_SUCCESS            Check in CPU (CpuIndex) successfully.
  @retval RETURN_ABORTED            Check in CPU failed due to SmmCpuSyncLockDoor() has been called by one elected CPU.

**/
RETURN_STATUS
EFIAPI
SmmCpuSyncCheckInCpu (
  IN OUT SMM_CPU_SYNC_CONTEXT  *Context,
  IN     UINTN                 CpuIndex
  )
{
  ASSERT (Context != NULL);

  ASSERT (CpuIndex < Context->NumberOfCpus);

  //
  // Check to return if CpuCount has already been locked.
  //
  if (InternalReleaseSemaphore (Context->CpuCount) == MAX_UINT32) {
    return RETURN_ABORTED;
  }

  return RETURN_SUCCESS;
}

/**
  Performs an atomic operation to check out CPU.

  This function can be called in error handling flow for the CPU who calls CheckInCpu() earlier.
  The caller shall make sure the CPU specified by CpuIndex has already checked-in.

  If Context is NULL, then ASSERT().
  If CpuIndex exceeds the range of all CPUs in the system, then ASSERT().

  @param[in,out]  Context           Pointer to the SMM CPU Sync context object.
  @param[in]      CpuIndex          Check out CPU index.

  @retval RETURN_SUCCESS            Check out CPU (CpuIndex) successfully.
  @retval RETURN_ABORTED            Check out CPU failed due to SmmCpuSyncLockDoor() has been called by one elected CPU.

**/
RETURN_STATUS
EFIAPI
SmmCpuSyncCheckOutCpu (
  IN OUT SMM_CPU_SYNC_CONTEXT  *Context,
  IN     UINTN                 CpuIndex
  )
{
  ASSERT (Context != NULL);

  ASSERT (CpuIndex < Context->NumberOfCpus);

  if (InternalWaitForSemaphore (Context->CpuCount) == MAX_UINT32) {
    return RETURN_ABORTED;
  }

  return RETURN_SUCCESS;
}

/**
  Performs an atomic operation lock door for CPU checkin and checkout. After this function:
  CPU can not check in via SmmCpuSyncCheckInCpu().
  CPU can not check out via SmmCpuSyncCheckOutCpu().

  The CPU specified by CpuIndex is elected to lock door. The caller shall make sure the CpuIndex
  is the actual CPU calling this function to avoid the undefined behavior.

  If Context is NULL, then ASSERT().
  If CpuCount is NULL, then ASSERT().
  If CpuIndex exceeds the range of all CPUs in the system, then ASSERT().

  @param[in,out]  Context           Pointer to the SMM CPU Sync context object.
  @param[in]      CpuIndex          Indicate which CPU to lock door.
  @param[out]     CpuCount          Number of arrived CPU in SMI after look door.

**/
VOID
EFIAPI
SmmCpuSyncLockDoor (
  IN OUT SMM_CPU_SYNC_CONTEXT  *Context,
  IN     UINTN                 CpuIndex,
  OUT UINTN                    *CpuCount
  )
{
  ASSERT (Context != NULL);

  ASSERT (CpuCount != NULL);

  ASSERT (CpuIndex < Context->NumberOfCpus);

  //
  // Temporarily record the CpuCount into the ArrivedCpuCountUponLock before lock door.
  // Recording before lock door is to avoid the Context->CpuCount is locked but possible
  // Context->ArrivedCpuCountUponLock is not updated.
  //
  Context->ArrivedCpuCountUponLock = *Context->CpuCount;

  //
  // Lock door operation
  //
  *CpuCount = InternalLockdownSemaphore (Context->CpuCount);

  //
  // Update the ArrivedCpuCountUponLock
  //
  Context->ArrivedCpuCountUponLock = *CpuCount;
}

/**
  Used by the BSP to wait for APs.

  The number of APs need to be waited is specified by NumberOfAPs. The BSP is specified by BspIndex.
  The caller shall make sure the BspIndex is the actual CPU calling this function to avoid the undefined behavior.
  The caller shall make sure the NumberOfAPs have already checked-in to avoid the undefined behavior.

  If Context is NULL, then ASSERT().
  If NumberOfAPs >= All CPUs in system, then ASSERT().
  If BspIndex exceeds the range of all CPUs in the system, then ASSERT().

  Note:
  This function is blocking mode, and it will return only after the number of APs released by
  calling SmmCpuSyncReleaseBsp():
  BSP: WaitForAPs    <--  AP: ReleaseBsp

  @param[in,out]  Context           Pointer to the SMM CPU Sync context object.
  @param[in]      NumberOfAPs       Number of APs need to be waited by BSP.
  @param[in]      BspIndex          The BSP Index to wait for APs.

**/
VOID
EFIAPI
SmmCpuSyncWaitForAPs (
  IN OUT SMM_CPU_SYNC_CONTEXT  *Context,
  IN     UINTN                 NumberOfAPs,
  IN     UINTN                 BspIndex
  )
{
  UINTN  Arrived;

  ASSERT (Context != NULL);

  ASSERT (NumberOfAPs < Context->NumberOfCpus);

  ASSERT (BspIndex < Context->NumberOfCpus);

  for (Arrived = 0; Arrived < NumberOfAPs; Arrived++) {
    InternalWaitForSemaphore (Context->CpuSem[BspIndex].Run);
  }
}

/**
  Used by the BSP to release one AP.

  The AP is specified by CpuIndex. The BSP is specified by BspIndex.
  The caller shall make sure the BspIndex is the actual CPU calling this function to avoid the undefined behavior.
  The caller shall make sure the CpuIndex has already checked-in to avoid the undefined behavior.

  If Context is NULL, then ASSERT().
  If CpuIndex == BspIndex, then ASSERT().
  If BspIndex or CpuIndex exceed the range of all CPUs in the system, then ASSERT().

  @param[in,out]  Context           Pointer to the SMM CPU Sync context object.
  @param[in]      CpuIndex          Indicate which AP need to be released.
  @param[in]      BspIndex          The BSP Index to release AP.

**/
VOID
EFIAPI
SmmCpuSyncReleaseOneAp   (
  IN OUT SMM_CPU_SYNC_CONTEXT  *Context,
  IN     UINTN                 CpuIndex,
  IN     UINTN                 BspIndex
  )
{
  ASSERT (Context != NULL);

  ASSERT (BspIndex != CpuIndex);

  ASSERT (CpuIndex < Context->NumberOfCpus);

  ASSERT (BspIndex < Context->NumberOfCpus);

  InternalReleaseSemaphore (Context->CpuSem[CpuIndex].Run);
}

/**
  Used by the AP to wait BSP.

  The AP is specified by CpuIndex.
  The caller shall make sure the CpuIndex is the actual CPU calling this function to avoid the undefined behavior.
  The BSP is specified by BspIndex.

  If Context is NULL, then ASSERT().
  If CpuIndex == BspIndex, then ASSERT().
  If BspIndex or CpuIndex exceed the range of all CPUs in the system, then ASSERT().

  Note:
  This function is blocking mode, and it will return only after the AP released by
  calling SmmCpuSyncReleaseOneAp():
  BSP: ReleaseOneAp  -->  AP: WaitForBsp

  @param[in,out]  Context          Pointer to the SMM CPU Sync context object.
  @param[in]      CpuIndex         Indicate which AP wait BSP.
  @param[in]      BspIndex         The BSP Index to be waited.

**/
VOID
EFIAPI
SmmCpuSyncWaitForBsp (
  IN OUT SMM_CPU_SYNC_CONTEXT  *Context,
  IN     UINTN                 CpuIndex,
  IN     UINTN                 BspIndex
  )
{
  ASSERT (Context != NULL);

  ASSERT (BspIndex != CpuIndex);

  ASSERT (CpuIndex < Context->NumberOfCpus);

  ASSERT (BspIndex < Context->NumberOfCpus);

  InternalWaitForSemaphore (Context->CpuSem[CpuIndex].Run);
}

/**
  Used by the AP to release BSP.

  The AP is specified by CpuIndex.
  The caller shall make sure the CpuIndex is the actual CPU calling this function to avoid the undefined behavior.
  The BSP is specified by BspIndex.

  If Context is NULL, then ASSERT().
  If CpuIndex == BspIndex, then ASSERT().
  If BspIndex or CpuIndex exceed the range of all CPUs in the system, then ASSERT().

  @param[in,out]  Context           Pointer to the SMM CPU Sync context object.
  @param[in]      CpuIndex          Indicate which AP release BSP.
  @param[in]      BspIndex          The BSP Index to be released.

**/
VOID
EFIAPI
SmmCpuSyncReleaseBsp (
  IN OUT SMM_CPU_SYNC_CONTEXT  *Context,
  IN     UINTN                 CpuIndex,
  IN     UINTN                 BspIndex
  )
{
  ASSERT (Context != NULL);

  ASSERT (BspIndex != CpuIndex);

  ASSERT (CpuIndex < Context->NumberOfCpus);

  ASSERT (BspIndex < Context->NumberOfCpus);

  InternalReleaseSemaphore (Context->CpuSem[BspIndex].Run);
}
